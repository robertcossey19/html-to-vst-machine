#include "PluginEditor.h"

static void styleSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
}

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      proc (p)
{
    title.setText ("HTMLtoVST (Fallback UI)", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (title);

    styleSlider (inGain);
    styleSlider (drive);
    styleSlider (outGain);

    inGain.setName ("Input");
    drive.setName ("Drive");
    outGain.setName ("Output");

    addAndMakeVisible (inGain);
    addAndMakeVisible (drive);
    addAndMakeVisible (outGain);

    inGainAtt  = std::make_unique<SliderAttachment> (proc.apvts, "inGain",  inGain);
    driveAtt   = std::make_unique<SliderAttachment> (proc.apvts, "drive",   drive);
    outGainAtt = std::make_unique<SliderAttachment> (proc.apvts, "outGain", outGain);

    setSize (520, 260);
}

void HtmlToVstPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.92f));

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (14.0f);
    g.drawFittedText ("This build uses a minimal JUCE UI to ensure CI builds.\n"
                      "Next step: re-add the HTML UI safely once builds are green.",
                      getLocalBounds().reduced (12).removeFromBottom (60),
                      juce::Justification::centred, 3);
}

void HtmlToVstPluginAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (12);

    title.setBounds (r.removeFromTop (28));

    r.removeFromTop (10);

    auto row = r.removeFromTop (160);
    auto w = row.getWidth() / 3;

    inGain.setBounds (row.removeFromLeft (w).reduced (10));
    drive.setBounds  (row.removeFromLeft (w).reduced (10));
    outGain.setBounds(row.removeFromLeft (w).reduced (10));
}
