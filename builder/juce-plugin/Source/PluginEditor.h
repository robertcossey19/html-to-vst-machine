#pragma once
#include "PluginProcessor.h"

class HtmlToVstPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // IMPORTANT: don't name this "processor" because AudioProcessorEditor already has a member called processor
    HtmlToVstPluginAudioProcessor& audioProcessor;

    juce::WebBrowserComponent webView;

    juce::File tempHtmlFile;
    bool uiLoaded = false;

    static juce::String urlDecodeToString (const juce::String& s);
    static bool looksLikeHtml (const juce::String& s);
    static juce::String makeMissingUiHtml (const juce::String& extra);

    void writeHtmlToTempAndLoad (const juce::String& html);
    void loadUiFromBinaryData();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
