#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class HtmlToVstAudioProcessor : public juce::AudioProcessor
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
    int getNumPrograms() const override;
    int getCurrentProgram() const override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) const override;
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

private:
    void updateDSP();

    double lastSampleRate { 44100.0 };

    // 16x oversampling (2x^4)
    juce::dsp::Oversampling<float> oversampling;

    // Core gain stages (match WebAudio graph)
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> fluxGain;
    juce::dsp::Gain<float> headroomGain;
    juce::dsp::Gain<float> repro;
    juce::dsp::Gain<float> monitorRe;
    juce::dsp::Gain<float> outGain;
    juce::dsp::Gain<float> xfTrim;

    // EQ / filters
    std::shared_ptr<juce::dsp::IIR::Coefficients<float>> biasTilt;
    juce::dsp::IIR::Filter<float> biasHF;
    juce::dsp::IIR::Filter<float> headBump;
    juce::dsp::IIR::Filter<float> deemph;
    juce::dsp::IIR::Filter<float> lpOut;

    // Non-linear blocks
    juce::dsp::WaveShaper<float> biasShaper;
    juce::dsp::WaveShaper<float> transformerShape;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
