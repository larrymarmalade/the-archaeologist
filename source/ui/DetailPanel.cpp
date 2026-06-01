#include "DetailPanel.h"
#include "Theme.h"

namespace arch
{
void DetailPanel::update (const ArtifactView& v, bool hasFocus)
{
    v_ = v; has_ = hasFocus;
    repaint();
}

static juce::String ageString (float sec)
{
    if (sec < 0.0f) sec = 0.0f;
    const int m = (int) (sec / 60.0f);
    const int s = (int) sec % 60;
    return juce::String (m) + "m " + juce::String (s) + "s";
}

void DetailPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (theme::smoke);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (theme::brass.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

    auto area = getLocalBounds().reduced (14, 12);
    g.setColour (theme::amber);
    g.setFont (juce::Font (juce::FontOptions (13.0f)).withExtraKerningFactor (0.12f));
    g.drawText ("EXCAVATED", area.removeFromTop (18), juce::Justification::topLeft, true);
    area.removeFromTop (6);

    if (! has_)
    {
        g.setColour (theme::paper.withAlpha (0.45f));
        g.setFont (juce::Font (juce::FontOptions (14.0f)));
        g.drawText (juce::String::fromUTF8 ("the archive is quiet \xe2\x80\x94 nothing has surfaced yet"),
                    area, juce::Justification::topLeft, true);
        return;
    }

    auto row = [&] (const juce::String& key, const juce::String& val)
    {
        auto line = area.removeFromTop (22);
        g.setColour (theme::paper.withAlpha (0.5f));
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawText (key, line.removeFromLeft (96), juce::Justification::centredLeft, true);
        g.setColour (theme::paper);
        g.setFont (juce::Font (juce::FontOptions (14.0f)));
        g.drawText (val, line, juce::Justification::centredLeft, true);
    };

    row ("Age",        ageString (v_.ageSec));
    row ("Source",     familyName (v_.family));
    row ("Condition",  theme::conditionFor (v_.condition));
    row ("Brightness", juce::String (juce::roundToInt (juce::jlimit (0.0f, 1.0f, v_.brightness * 4.0f) * 100.0f)) + "%");
    row ("Stability",  juce::String (juce::roundToInt (v_.stability * 100.0f)) + "%");
    if (v_.pitchHz > 0.0f)
        row ("Pitch",  juce::String (v_.pitchHz, 1) + " Hz");
}
} // namespace arch
