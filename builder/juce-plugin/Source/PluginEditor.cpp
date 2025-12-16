#include "PluginEditor.h"

static void styleKnob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
}

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
: AudioProcessorEditor (&p), processor (p)
{
    styleKnob (inGain);
    styleKnob (drive);
    styleKnob (outGain);

    inLbl.setText ("IN", juce::dontSendNotification);
    driveLbl.setText ("DRIVE", juce::dontSendNotification);
    outLbl.setText ("OUT", juce::dontSendNotification);

    for (auto* l : { &inLbl, &driveLbl, &outLbl })
    {
        l->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*l);
    }

    addAndMakeVisible (inGain);
    addAndMakeVisible (drive);
    addAndMakeVisible (outGain);

    inAtt    = std::make_unique<Attachment> (processor.apvts, "inGain",  inGain);
    driveAtt = std::make_unique<Attachment> (processor.apvts, "drive",   drive);
    outAtt   = std::make_unique<Attachment> (processor.apvts, "outGain", outGain);

    setSize (520, 220);
}

void HtmlToVstPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff141414));
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (15.0f);
    g.drawText ("HTMLtoVST", 12, 10, getWidth() - 24, 20, juce::Justification::centredLeft);
}

void HtmlToVstPluginAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    r.removeFromTop (28);

    auto row = r.removeFromTop (170);

    auto colW = row.getWidth() / 3;
    auto a = row.removeFromLeft (colW).reduced (10);
    auto b = row.removeFromLeft (colW).reduced (10);
    auto c = row.removeFromLeft (colW).reduced (10);

    inLbl.setBounds    (a.removeFromTop (18));
    driveLbl.setBounds (b.removeFromTop (18));
    outLbl.setBounds   (c.removeFromTop (18));

    inGain.setBounds  (a);
    drive.setBounds   (b);
    outGain.setBounds (c);
}
