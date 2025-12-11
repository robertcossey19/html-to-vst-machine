#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class HtmlToVstAudioProcessor : public juce::AudioProcessor
{
public:
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

    //==============================================================================
    // NOTE: These signatures must match the headless AudioProcessor API

    // getName is non-const in the headless JUCE API
    const juce::String getName() override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

   #if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // VU meters exposed to the editor (in dB)
    std::atomic<float> currentVUL { 0.0f };
    std::atomic<float> currentVUR { 0.0f };

    // Convenience getters the editor can call
    float getCurrentVUL() const noexcept { return currentVUL.load(); }
    float getCurrentVUR() const noexcept { return currentVUR.load(); }

private:
    void updateDSP();

    double lastSampleRate { 44100.0 };

    // 16× oversampling core
    juce::dsp::Oversampling<float> oversampling;

    // Gain stages (mirror WebAudio graph)
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> fluxGain;
    juce::dsp::Gain<float> headroomGain;
    juce::dsp::Gain<float> repro;
    juce::dsp::Gain<float> monitorRe;
    juce::dsp::Gain<float> outGain;
    juce::dsp::Gain<float> xfTrim;

    // Filters / EQ
    juce::dsp::IIR::Filter<float> biasHF;    // bias high-shelf tilt
    juce::dsp::IIR::Filter<float> headBump;  // LF bump
    juce::dsp::IIR::Filter<float> deemph;    // repro high shelf
    juce::dsp::IIR::Filter<float> lpOut;     // output lowpass

    // Non-linear stages
    juce::dsp::WaveShaper<float> biasShaper;       // bias asymmetry
    juce::dsp::WaveShaper<float> transformerShape; // output transformer sim

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
