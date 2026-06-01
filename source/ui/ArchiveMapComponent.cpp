#include "ArchiveMapComponent.h"
#include "Theme.h"

namespace arch
{
ArchiveMapComponent::ArchiveMapComponent()
{
    setOpaque (true);
}

void ArchiveMapComponent::update (const Snapshot& snap, float dt)
{
    clock_       += dt;
    aliveTotal_   = snap.aliveTotal;
    activeVoices_ = snap.activeVoices;
    frozen_       = snap.frozen;
    mode_         = snap.mode;
    archiveFill_  = snap.archiveFill;

    for (auto& kv : glyphs_) kv.second.present = false;

    const float maxAge = juce::jmax (8.0f, snap.sessionSeconds > 0 ? 60.0f : 30.0f);
    for (int i = 0; i < snap.viewCount; ++i)
    {
        const auto& v = snap.views[(size_t) i];
        auto& gph = glyphs_[v.id];
        gph.present = true;
        gph.family  = v.family;
        // x: age (old drifts left), y: brightness (bright = top)
        gph.tx = juce::jlimit (0.04f, 0.96f, 0.94f - juce::jlimit (0.0f, 1.0f, v.ageSec / maxAge) * 0.9f);
        gph.ty = juce::jlimit (0.06f, 0.94f, 0.9f - v.brightness * 4.0f);
        gph.size = juce::jlimit (0.2f, 1.0f, (v.loudnessDb + 48.0f) / 48.0f);
        if (gph.seen <= 0.0f) { gph.x = gph.tx; gph.y = gph.ty + 0.05f; }
        gph.seen = juce::jmin (1.0f, gph.seen + dt * 1.5f);

        const float glowTarget = v.excavating ? 1.0f : v.excitation;
        gph.glow += (glowTarget - gph.glow) * juce::jmin (1.0f, dt * 4.0f);
        if (v.excavating) gph.ty -= 0.04f;                 // surface upward
    }

    // ease positions; drop vanished glyphs
    for (auto it = glyphs_.begin(); it != glyphs_.end(); )
    {
        auto& gph = it->second;
        if (! gph.present)
        {
            gph.seen -= dt * 0.8f;
            if (gph.seen <= 0.0f) { it = glyphs_.erase (it); continue; }
        }
        gph.x += (gph.tx - gph.x) * juce::jmin (1.0f, dt * 1.2f);
        gph.y += (gph.ty - gph.y) * juce::jmin (1.0f, dt * 1.2f);
        ++it;
    }
}

void ArchiveMapComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // smoked-glass field over warm paper
    juce::ColourGradient grad (theme::smoke.darker (0.2f), r.getCentre(),
                               theme::smoke.brighter (0.05f), r.getTopLeft(), true);
    g.setGradientFill (grad);
    g.fillRect (r);

    // archaeological strata
    g.setColour (theme::brassDim.withAlpha (0.10f));
    for (int i = 1; i < 6; ++i)
    {
        const float y = r.getY() + r.getHeight() * (float) i / 6.0f;
        g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    }

    // faint slow "breathing" vignette so the field is alive at idle
    const float breathe = 0.5f + 0.5f * std::sin (clock_ * 0.35f);
    g.setColour (theme::amber.withAlpha (0.04f + 0.03f * breathe));
    g.fillEllipse (r.reduced (r.getWidth() * 0.18f, r.getHeight() * 0.2f));

    // glyphs
    for (const auto& kv : glyphs_)
    {
        const auto& gph = kv.second;
        const float px = r.getX() + gph.x * r.getWidth();
        const float py = r.getY() + gph.y * r.getHeight();
        const float baseR = 4.0f + gph.size * 14.0f;
        const float appear = juce::jlimit (0.0f, 1.0f, gph.seen);
        const auto col = theme::familyColour (gph.family);

        if (gph.glow > 0.01f)
        {
            g.setColour (theme::amber.withAlpha (0.18f * gph.glow));
            const float gr = baseR * (1.8f + gph.glow);
            g.fillEllipse (px - gr, py - gr, gr * 2.0f, gr * 2.0f);
        }

        g.setColour (col.withAlpha (0.85f * appear));
        g.fillEllipse (px - baseR * 0.5f, py - baseR * 0.5f, baseR, baseR);
        g.setColour (theme::paper.withAlpha (0.5f * appear));
        g.drawEllipse (px - baseR * 0.5f, py - baseR * 0.5f, baseR, baseR, 1.0f);
    }

    // header text
    g.setColour (theme::paper.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    juce::String hdr;
    const juce::String dot = juce::String::fromUTF8 ("   \xc2\xb7   ");
    hdr << (frozen_ ? "ARCHIVE FROZEN" : "LISTENING")
        << dot << aliveTotal_ << " artifacts"
        << dot << activeVoices_ << " surfacing";
    g.drawText (hdr, getLocalBounds().reduced (12, 8), juce::Justification::topLeft, true);

    g.setColour (theme::brass.withAlpha (0.4f));
    g.drawRect (getLocalBounds(), 1);
}
} // namespace arch
