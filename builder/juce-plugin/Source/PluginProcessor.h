#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// Short aliases to match the JUCE plugin templates
using BusesProperties = juce::AudioProcessor::BusesProperties;
using BusesLayout     = juce::AudioProcessor::BusesLayout;

class HtmlToVstAudioProcessor : public juce::AudioProcessor
{
public:
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

    //==============================================================================
    // Mandatory AudioProcessor overrides
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // VU meter access for the HTML UI
    float getCurrentVUL() const noexcept { return currentVUL.load(); }
    float getCurrentVUR() const noexcept { return currentVUR.load(); }

private:
    void updateDSP();

    //==============================================================================
    double currentSampleRate = 44100.0;
    int    numChannels       = 2;

    // 16x oversampling: 2^4 = 16
    juce::dsp::Oversampling<float> oversampling;

    // Core gain / EQ stages (all 768 kHz in the oversampled domain)
    juce::dsp::Gain<float>        inputGain;
    juce::dsp::Gain<float>        fluxGain;
    juce::dsp::Gain<float>        headroomGain;
    juce::dsp::IIR::Filter<float> headBump;
    juce::dsp::IIR::Filter<float> reproHighShelf;
    juce::dsp::IIR::Filter<float> lowpassOut;
    juce::dsp::Gain<float>        outputGain;

    // Bias calibration (used in manual waveshaping)
    const float biasDb = 1.8f;   // over-bias around +1.8 dB (from the WebAudio version)

    // VU meter state (RMS in linear)
    std::atomic<float> currentVUL { 0.0f };
    std::atomic<float> currentVUR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
