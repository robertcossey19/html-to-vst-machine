#pragma once

#include "PluginProcessor.h"

class HtmlToVstAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Loads embedded HTML into the WebView correctly (data: URL)
    void loadUI();

    // Try a few common BinaryData names; fallback to a minimal page
    juce::String getEmbeddedHtml() const;

    HtmlToVstAudioProcessor& processor;

    // JUCE 7+ uses Options or default ctor
    juce::WebBrowserComponent webView;

    bool loadedOnce = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
