#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class HtmlToVstAudioProcessor : public juce::AudioProcessor
{
public:
    using BusesLayout = juce::AudioProcessor::BusesLayout;

    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

    // Basic plugin info
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    // Programs (we just expose a single program)
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    // Audio processing
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Editor
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    // State (no parameters yet, we just remember a dummy tag)
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // VU meter values in dB (approx. -80..+12) for the HTML UI
    float getCurrentVUL() const noexcept { return currentVUL.load(); }
    float getCurrentVUR() const noexcept { return currentVUR.load(); }

private:
    void updateDSP();

    double currentSampleRate = 44100.0;
    int    numChannels       = 2;

    // 16x oversampling ATR-style chain
    juce::dsp::Oversampling<float> oversampling;

    juce::dsp::Gain<float>         inputGain;
    juce::dsp::Gain<float>         fluxGain;
    juce::dsp::Gain<float>         headroomGain;
    juce::dsp::IIR::Filter<float>  headBump;
    juce::dsp::IIR::Filter<float>  reproHighShelf;
    juce::dsp::WaveShaper<float>   biasShaper;
    juce::dsp::WaveShaper<float>   transformerShape;
    juce::dsp::IIR::Filter<float>  lowpassOut;
    juce::dsp::Gain<float>         outputGain;

    // Block RMS (dB) for the VU meters
    std::atomic<float> currentVUL { 0.0f };
    std::atomic<float> currentVUR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
