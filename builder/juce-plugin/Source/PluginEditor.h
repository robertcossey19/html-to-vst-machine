#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
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
    HtmlToVstPluginAudioProcessor& processor;

    juce::Slider inGain, drive, outGain;
    juce::Label  inLbl, driveLbl, outLbl;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> inAtt, driveAtt, outAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
