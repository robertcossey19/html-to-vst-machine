#pragma once
#include <JuceHeader.h>

class HtmlToVstAudioProcessor;

class HtmlToVstAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class BridgeBrowser : public juce::WebBrowserComponent
    {
    public:
        explicit BridgeBrowser (HtmlToVstAudioProcessorEditor& o)
        : juce::WebBrowserComponent (false), owner (o) {}

        bool pageAboutToLoad (const juce::String& newURL) override;
        void pageFinishedLoading (const juce::String& url) override;

    private:
        HtmlToVstAudioProcessorEditor& owner;
    };

    void timerCallback() override;
    void loadUiToTempFileAndNavigate();
    void injectBridgeJs();

    HtmlToVstAudioProcessor& processor;
    BridgeBrowser webView;

    juce::File tempHtmlFile;
    bool bridgeInjected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
