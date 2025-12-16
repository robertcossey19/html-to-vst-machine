#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HtmlToVstPluginAudioProcessorEditor final
    : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstPluginAudioProcessor& processor;

    // Web view subclass to intercept juce://set?param=...&value=...
    class Browser final : public juce::WebBrowserComponent
    {
    public:
        explicit Browser (HtmlToVstPluginAudioProcessorEditor& ownerEditor);

        bool pageAboutToLoad (const juce::String& newURL) override;

        // JUCE uses evaluateJavascript in this version
        void runJS (const juce::String& js)
        {
            evaluateJavascript (js);
        }

    private:
        HtmlToVstPluginAudioProcessorEditor& owner;
    };

    Browser webView;

    juce::String makeUiDataUrl();
    void injectBridgeJS();
    void handleJuceUrl (const juce::String& url);

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
