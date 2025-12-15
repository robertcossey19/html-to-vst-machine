#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class HtmlToVstPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // IMPORTANT: Don't name this 'processor' — JUCE now has AudioProcessorEditor::processor
    HtmlToVstPluginAudioProcessor& audioProcessor;

    juce::Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
