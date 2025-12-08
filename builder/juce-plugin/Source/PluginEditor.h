#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class HtmlToVstAudioProcessor;

class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
