#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class HtmlToVstAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;
    juce::WebBrowserComponent web;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
