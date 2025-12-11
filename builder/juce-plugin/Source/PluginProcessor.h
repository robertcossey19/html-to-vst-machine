#pragma once

#include <atomic>
#include "JuceHeader.h"

//==============================================================================

class HtmlToVstAudioProcessor  : public juce::AudioProcessor
{
public:
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
#if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /// Current VU values in dBFS (block-level RMS) for driving the HTML needles
    float getCurrentVUL() const noexcept { return currentVUL.load(); }
    float getCurrentVUR() const noexcept { return currentVUR.load(); }

    /// Rebuilds the DSP graph & calibration (15ips / 250nWb/m / 456 / −12dB headroom)
    void updateDSP();

private:
    //==============================================================================
    double currentSampleRate = 0.0;
    int    numChannels       = 0;

    // 16x oversampling (2^4)
    juce::dsp::Oversampling<float> oversampling;

    // Gain structure: input, flux, headroom, output
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> fluxGain;
    juce::dsp::Gain<float> headroomGain;
    juce::dsp::Gain<float> outputGain;

    // Tape EQ: head bump + repro shelf + output HF limit
    juce::dsp::IIR::Filter<float> headBump;
    juce::dsp::IIR::Filter<float> reproHighShelf;
    juce::dsp::IIR::Filter<float> lowpassOut;

    // Bias symmetry & transformer “iron” shaper
    juce::dsp::WaveShaper<float> biasShaper;
    juce::dsp::WaveShaper<float> transformerShape;

    // Block-level VU values
    std::atomic<float> currentVUL { -80.0f };
    std::atomic<float> currentVUR { -80.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
