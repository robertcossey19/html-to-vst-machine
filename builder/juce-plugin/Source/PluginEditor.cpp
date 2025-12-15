#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p)
{
    infoLabel.setText ("HTML to VST Plugin\n(UI placeholder)", juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (infoLabel);

    setSize (500, 350);
}

HtmlToVstPluginAudioProcessorEditor::~HtmlToVstPluginAudioProcessorEditor() = default;

//==============================================================================
void HtmlToVstPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void HtmlToVstPluginAudioProcessorEditor::resized()
{
    infoLabel.setBounds (getLocalBounds().reduced (20));
}
