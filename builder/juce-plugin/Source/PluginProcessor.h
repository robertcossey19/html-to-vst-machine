#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "GeneratedParams.h"

//==============================================================================

class HtmlToVstAudioProcessor : public juce::AudioProcessor
{
public:
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override = default;

    //==============================================================================
    const juce::String getName() const override { return "HtmlToVstPlugin"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    // MIDI capabilities
    bool acceptsMidi() const override    { return false; }
    bool producesMidi() const override   { return false; }
    bool isMidiEffect() const override   { return false; }

    //==============================================================================
    bool hasEditor() const override         { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override               { return 1; }
    int getCurrentProgram() override            { return 0; }
    void setCurrentProgram (int) override       {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Access to the parameter state
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
