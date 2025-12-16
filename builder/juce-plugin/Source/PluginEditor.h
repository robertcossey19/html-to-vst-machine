#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HtmlToVstPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Small WebBrowser subclass so we can intercept navigation and inject JS
    class Browser final : public juce::WebBrowserComponent
    {
    public:
        Browser();

        std::function<bool (const juce::String&)> onAboutToLoad;
        std::function<void (const juce::String&)> onFinished;

    private:
        bool pageAboutToLoad (const juce::String& newURL) override;
        void pageFinishedLoading (const juce::String& url) override;
    };

    HtmlToVstPluginAudioProcessor& audioProcessor;
    Browser webView;

    juce::File tempHtmlFile;
    bool uiLoaded = false;

    void timerCallback() override;

    // UI loading
    void loadUiFromBinaryData();
    void writeHtmlToTempAndLoad (const juce::String& html);

    // Bridge helpers
    void injectBridgeJavascript();
    void handleJuceUrl (const juce::String& urlString);

    // Simple URL query parsing without relying on JUCE URL helpers that differ across versions
    static juce::String getQueryParam (const juce::String& url, const juce::String& key);

    static juce::String urlDecode (const juce::String& s);
    static bool looksLikeHtml (const juce::String& s);
    static juce::String makeMissingUiHtml (const juce::String& extra);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
