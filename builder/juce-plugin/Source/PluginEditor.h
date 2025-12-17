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
    class Browser final : public juce::WebBrowserComponent
    {
    public:
        explicit Browser (HtmlToVstPluginAudioProcessorEditor& ownerIn) : owner (ownerIn) {}
        bool pageAboutToLoad (const juce::String& newURL) override { return owner.handleBrowserURL (newURL); }

    private:
        HtmlToVstPluginAudioProcessorEditor& owner;
    };

    bool handleBrowserURL (const juce::String& url);

    void sendAllParamsToJS();
    void sendParamToJS (const juce::String& paramID, float value);

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    static juce::String getIndexHtml();

    HtmlToVstPluginAudioProcessor& audioProcessor;
    std::unique_ptr<Browser> browser;

    std::atomic<float> inGainValue  { 0.0f };
    std::atomic<float> driveValue   { 0.0f };
    std::atomic<float> outGainValue { 0.0f };
    std::atomic<bool>  dirty        { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
