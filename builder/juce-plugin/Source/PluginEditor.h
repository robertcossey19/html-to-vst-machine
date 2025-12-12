#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
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
    class InterceptingWebView : public juce::WebBrowserComponent
    {
    public:
        explicit InterceptingWebView (HtmlToVstAudioProcessorEditor& owner)
            : juce::WebBrowserComponent(), editor (owner) {}

        bool pageAboutToLoad (const juce::String& newURL) override;

    private:
        HtmlToVstAudioProcessorEditor& editor;
    };

    void timerCallback() override;

    void loadUi();
    juce::String getEmbeddedHtmlOrFallback (juce::String& debugInfoOut) const;
    static juce::String makeMissingUiHtml (const juce::String& extra);

    void handleJuceUrl (const juce::String& urlString);

    HtmlToVstAudioProcessor& processor;
    InterceptingWebView webView;

    juce::File tempDir;
    juce::File tempHtmlFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
