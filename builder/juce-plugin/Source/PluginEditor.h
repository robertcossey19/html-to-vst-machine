#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HtmlToVstPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Browser final : public juce::WebBrowserComponent
    {
    public:
        using juce::WebBrowserComponent::WebBrowserComponent;

        void runJS (const juce::String& js)
        {
            evaluateJavascript (js, nullptr);
        }
    };

    void timerCallback() override;
    void loadUiFromBinaryData();

    HtmlToVstPluginAudioProcessor& audioProcessor;

    Browser webView;

    juce::File uiTempHtmlFile;
    bool uiLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
