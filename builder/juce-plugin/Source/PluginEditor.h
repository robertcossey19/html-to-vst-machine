#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class HtmlToVstPluginAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::AudioProcessorValueTreeState::Listener,
      private juce::Timer
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // A tiny WebBrowser subclass so we can intercept custom "juce://" URLs.
    class Browser final : public juce::WebBrowserComponent
    {
    public:
        explicit Browser (HtmlToVstPluginAudioProcessorEditor& ownerIn) : owner (ownerIn) {}
        bool pageAboutToLoad (const juce::String& newURL) override { return owner.handleBrowserURL (newURL); }

    private:
        HtmlToVstPluginAudioProcessorEditor& owner;
    };

    // URL handler for messages from the HTML UI
    bool handleBrowserURL (const juce::String& url);

    // JS send helpers (called on message thread)
    void sendAllParamsToJS();
    void sendParamToJS (const juce::String& paramID, float value);

    // APVTS listener
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // Timer (message thread) to safely push param changes -> JS
    void timerCallback() override;

    // Loads HTML either from BinaryData (preferred) or embedded fallback
    static juce::String getIndexHtml();

    HtmlToVstPluginAudioProcessor& audioProcessor;

    std::unique_ptr<Browser> browser;

    // atomics updated on audio thread, pushed to UI on timer
    std::atomic<float> inGainValue  { 0.0f };
    std::atomic<float> driveValue   { 0.0f };
    std::atomic<float> outGainValue { 0.0f };
    std::atomic<bool>  dirty        { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
