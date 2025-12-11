#pragma once
#include "PluginProcessor.h"

class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void loadUiFromBinaryData();

    HtmlToVstAudioProcessor& processor;

    // JUCE 7+ WebBrowserComponent takes Options or default ctor (NOT bool)
    juce::WebBrowserComponent webView;

    bool uiLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
