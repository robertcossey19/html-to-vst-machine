#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
// HtmlToVstAudioProcessor
// Core ATR-102 style DSP chain with 16x oversampling.
//==============================================================================

class HtmlToVstAudioProcessor : public juce::AudioProcessor
{
public:
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

    //==========================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==========================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==========================================================================
#if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    //==========================================================================
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // VU meter access for the editor
    float getCurrentVUL() const noexcept { return currentVUL; }
    float getCurrentVUR() const noexcept { return currentVUR; }

private:
    // Internal helper to set up EQ, gains, waveshapers, etc.
    void updateDSP();

    //==========================================================================
    double currentSampleRate = 0.0;
    int    numChannels       = 0;

    // 16x oversampling = 2^4
    juce::dsp::Oversampling<float> oversampling;

    // Gain structure
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> fluxGain;
    juce::dsp::Gain<float> headroomGain;
    juce::dsp::Gain<float> outputGain;

    // Non-linear and EQ stages
    juce::dsp::WaveShaper<float> biasShaper;
    juce::dsp::IIR::Filter<float> headBump;
    juce::dsp::IIR::Filter<float> reproHighShelf;
    juce::dsp::WaveShaper<float> transformerShape;
    juce::dsp::IIR::Filter<float> lowpassOut;

    // Meter state (smoothed RMS in dBFS)
    float currentVUL = -70.0f;
    float currentVUR = -70.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};

//==============================================================================
// Factory
//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
