#pragma once

#include <atomic>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace arch
{
/**
    Stereo (or mono) circular capture buffer.

    Threading contract:
      - exactly ONE writer: the audio thread (write()).
      - exactly ONE reader: the background analysis thread (copyRange()).
    The reader only ever touches samples strictly OLDER than the current write
    head, so writer and reader never address the same cell concurrently.

    Absolute sample positions are monotonic 64-bit counters; the analysis
    thread and artifact metadata speak in absolute positions, which the buffer
    maps onto its physical ring. A position older than (written - capacity) has
    been overwritten and is no longer recoverable.

    All storage is allocated in prepare(); write()/copyRange() never allocate.
*/
class RingBuffer
{
public:
    void prepare (int numChannels, int capacitySamples)
    {
        channels_ = std::max (1, numChannels);
        capacity_ = std::max (1, capacitySamples);
        data_.assign (static_cast<size_t> (channels_) * static_cast<size_t> (capacity_), 0.0f);
        written_.store (0, std::memory_order_relaxed);
        writePos_ = 0;
    }

    /** O(1) reset — does NOT zero memory (safe on the audio thread). Old
        samples become unreachable because totalWritten resets to 0 and all
        reads are bounds-checked against it. */
    void clear() noexcept
    {
        written_.store (0, std::memory_order_release);
        writePos_ = 0;
    }

    int  channels()  const noexcept { return channels_; }
    int  capacity()  const noexcept { return capacity_; }
    /** Total samples ever written (monotonic). Safe to read from any thread. */
    int64_t totalWritten() const noexcept { return written_.load (std::memory_order_acquire); }

    /** Audio-thread only. Append `n` frames from interleaved-by-channel src[ch][i]. */
    void write (const float* const* src, int numSrcChannels, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const int phys = writePos_;
            for (int ch = 0; ch < channels_; ++ch)
            {
                const int srcCh = (ch < numSrcChannels) ? ch : (numSrcChannels - 1);
                data_[static_cast<size_t> (ch) * capacity_ + phys] = src[srcCh][i];
            }
            writePos_ = (writePos_ + 1 == capacity_) ? 0 : writePos_ + 1;
        }
        written_.fetch_add (n, std::memory_order_release);
    }

    /** Background-thread read of an absolute range into dst[ch] (length n).
        Returns false if any part of the range has been overwritten. */
    bool copyRange (int64_t startAbs, int n, float* const* dst, int numDstChannels) const noexcept
    {
        const int64_t written = written_.load (std::memory_order_acquire);
        if (startAbs < 0 || startAbs + n > written)
            return false;
        if (written - startAbs > capacity_)
            return false;                                   // eroded

        for (int i = 0; i < n; ++i)
        {
            const int phys = static_cast<int> ((startAbs + i) % capacity_);
            for (int ch = 0; ch < numDstChannels; ++ch)
            {
                const int srcCh = (ch < channels_) ? ch : (channels_ - 1);
                dst[ch][i] = data_[static_cast<size_t> (srcCh) * capacity_ + phys];
            }
        }
        return true;
    }

    /** Single-sample absolute read (background thread). Returns 0 if eroded. */
    float at (int64_t abs, int ch) const noexcept
    {
        const int64_t written = written_.load (std::memory_order_acquire);
        if (abs < 0 || abs >= written || written - abs > capacity_)
            return 0.0f;
        const int phys = static_cast<int> (abs % capacity_);
        const int c = (ch < channels_) ? ch : (channels_ - 1);
        return data_[static_cast<size_t> (c) * capacity_ + phys];
    }

private:
    std::vector<float> data_;
    int channels_ = 0;
    int capacity_ = 0;
    int writePos_ = 0;                       // audio thread only
    std::atomic<int64_t> written_ { 0 };
};
} // namespace arch
