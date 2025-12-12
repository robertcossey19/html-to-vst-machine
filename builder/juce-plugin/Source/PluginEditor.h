#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class HtmlToVstAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer,
                                           private juce::WebBrowserComponent::Listener
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    bool pageAboutToLoad (const juce::String& newURL) override;

    void ensureHtmlReady();
    void ensureBridgeInjected();

    HtmlToVstAudioProcessor& processor;
    juce::WebBrowserComponent webView;

    juce::File tempDir;
    juce::File tempHtmlFile;

    bool htmlReady = false;
    bool bridgeInjected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
