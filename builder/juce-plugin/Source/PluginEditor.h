#pragma once
#include "PluginProcessor.h"

class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;
    juce::WebBrowserComponent webView;

    juce::File tempHtmlFile;
    bool uiLoaded = false;

    static juce::String urlDecodeToString (const juce::String& s);
    static bool looksLikeHtml (const juce::String& s);
    static juce::String makeMissingUiHtml (const juce::String& extra);

    void writeHtmlToTempAndLoad (const juce::String& html);
    void loadUiFromBinaryData();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
