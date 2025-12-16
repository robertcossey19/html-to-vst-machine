#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

class HtmlToVstPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstPluginAudioProcessor& proc;

    // Simple fallback UI (always available)
    juce::Label title;
    juce::Slider inGain, drive, outGain;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> inGainAtt, driveAtt, outGainAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
