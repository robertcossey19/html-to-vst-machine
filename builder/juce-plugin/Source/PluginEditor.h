#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HtmlToVstPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstPluginAudioProcessor& audioProcessor;

    class UiWebView : public juce::WebBrowserComponent
    {
    public:
        explicit UiWebView (HtmlToVstPluginAudioProcessor& p) : audioProcessor (p) {}
        bool pageAboutToLoad (const juce::String& newURL) override;

    private:
        HtmlToVstPluginAudioProcessor& audioProcessor;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UiWebView)
    };

    UiWebView webView;

    juce::File tempHtmlFile;
    bool uiLoaded = false;

    static juce::String urlDecodeToString (const juce::String& s);
    static bool looksLikeHtml (const juce::String& s);
    static juce::String makeMissingUiHtml (const juce::String& extra);

    // JUCE-version-safe query parsing (works even if URL::getParameterValue is missing)
    static juce::String getQueryParam (const juce::String& fullUrl, const juce::String& key);

    void writeHtmlToTempAndLoad (const juce::String& html);
    void loadUiFromBinaryData();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
