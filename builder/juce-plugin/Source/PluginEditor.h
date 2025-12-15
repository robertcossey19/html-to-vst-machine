#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class HtmlToVstPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    HtmlToVstPluginAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
