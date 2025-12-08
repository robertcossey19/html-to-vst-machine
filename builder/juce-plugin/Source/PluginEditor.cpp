#include "PluginEditor.h"
#include "PluginProcessor.h"

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
  : juce::AudioProcessorEditor (&p), processor (p)
{
    setSize (600, 400);
}

void HtmlToVstAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawFittedText ("HTML → VST (JUCE template)", getLocalBounds(), juce::Justification::centred, 1);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    // Layout any child components here (none yet).
}
