#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Editor
class HtmlToVstPluginAudioProcessorEditor final
    : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Loads index.html from BinaryData and navigates the embedded browser to it.
    void loadUiFromBinaryData();

    HtmlToVstPluginAudioProcessor& processor;

    juce::WebBrowserComponent webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
