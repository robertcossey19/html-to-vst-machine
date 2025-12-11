#pragma once

// In this project we don’t have JuceHeader.h, we include the modules directly.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================

class HtmlToVstAudioProcessor : public juce::AudioProcessor
{
public:
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

    //==============================================================================
    // AudioProcessor basic info
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    // We don’t actually use programs, but AudioProcessor requires these.
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
    // VU values for the HTML UI (in dB, RMS, post-processing, post-crosstalk).
    float getCurrentVUL() const noexcept { return currentVUL; }
    float getCurrentVUR() const noexcept { return currentVUR; }

private:
    void updateDSP();

    //==============================================================================
    double currentSampleRate = 44100.0;
    int    numChannels       = 2;

    // 16x oversampling: 2^4 = 16
    juce::dsp::Oversampling<float> oversampling;

    // Core gain stages
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> fluxGain;
    juce::dsp::Gain<float> headroomGain;
    juce::dsp::Gain<float> outputGain;

    // Tape non-linearity and EQ
    juce::dsp::WaveShaper<float> biasShaper;         // bias symmetry / odd-even balance
    juce::dsp::IIR::Filter<float> headBump;          // LF head bump
    juce::dsp::IIR::Filter<float> reproHighShelf;    // repro HF shelf (NAB / 15 ips)
    juce::dsp::WaveShaper<float> transformerShape;   // output transformer saturation
    juce::dsp::IIR::Filter<float> lowpassOut;        // final bandwidth limit

    // VU state (in dB)
    float currentVUL = -80.0f;
    float currentVUR = -80.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
