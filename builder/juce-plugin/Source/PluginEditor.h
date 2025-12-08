#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class HtmlToVstAudioProcessor;

/**
 * Basic JUCE editor for the HtmlToVst plugin.
 * For now this is just a placeholder with a black background and some text.
 */
class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
