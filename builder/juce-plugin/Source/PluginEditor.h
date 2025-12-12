#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class HtmlToVstPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // A WebBrowserComponent subclass so we can intercept juce:// messages
    class UiWebView : public juce::WebBrowserComponent
    {
    public:
        explicit UiWebView (HtmlToVstPluginAudioProcessorEditor& ownerIn)
        : owner (ownerIn) {}

        bool pageAboutToLoad (const juce::String& newURL) override
        {
            // Intercept and consume UI -> DSP messages
            if (newURL.startsWithIgnoreCase ("juce:"))
            {
                owner.processor.handleUiMessageUrl (newURL);
                return false; // prevent navigation
            }

            return true;
        }

    private:
        HtmlToVstPluginAudioProcessorEditor& owner;
    };

    void timerCallback() override;
    void loadHtmlUi();

    HtmlToVstPluginAudioProcessor& processor;

    UiWebView webView;

    juce::File uiTempDir;
    juce::File uiTempFile;
    bool uiLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
