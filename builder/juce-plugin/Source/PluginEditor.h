#pragma once
#include <JuceHeader.h>

class HtmlToVstPluginAudioProcessor;

class HtmlToVstPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    HtmlToVstPluginAudioProcessor& audioProcessor;

    juce::WebBrowserComponent web;

    void loadUiFromBinaryData();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
