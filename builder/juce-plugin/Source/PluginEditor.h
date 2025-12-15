/*
  ==============================================================================

    PluginEditor.h

  ==============================================================================
*/

#pragma once

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
    // Use the classic JUCE hook that exists across older/newer JUCE revisions.
    class InterceptingBrowser : public juce::WebBrowserComponent
    {
    public:
        explicit InterceptingBrowser (HtmlToVstPluginAudioProcessor& p);
        bool pageAboutToLoad (const juce::String& newURL) override;

    private:
        HtmlToVstPluginAudioProcessor& processor;
    };

    void timerCallback() override;
    void loadHtmlUi();
    void pushMetersToUi();

    HtmlToVstPluginAudioProcessor& processor;
    InterceptingBrowser webView;

    bool uiLoaded = false;
    int uiRetryCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
