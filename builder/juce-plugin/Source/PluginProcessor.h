#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class HtmlToVstAudioProcessor : public juce::AudioProcessor
{
public:
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

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

    // --- UI -> DSP bridge (called by editor when the web UI changes) ---
    void setUiParameters (float inDb, float outDb, float biasDb, bool transformerOn);

    // --- VU accessors (dB RMS) ---
    float getCurrentVUL() const noexcept { return currentVUL.load(); }
    float getCurrentVUR() const noexcept { return currentVUR.load(); }

private:
    void updateDSP(); // static filters + fixed calibration parts
    void applyUiToDSPIfNeeded(); // gains/bias/transformer from UI
    void updateVuFromBuffer (juce::AudioBuffer<float>& buffer);

    double currentSampleRate = 44100.0;
    int numChannels = 2;

    // 16x oversampling = 4 stages of 2x
    juce::dsp::Oversampling<float> oversampling { 2, 4, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };

    // DSP blocks
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> fluxGain;
    juce::dsp::Gain<float> headroomGain;
    juce::dsp::IIR::Filter<float> headBump;
    juce::dsp::IIR::Filter<float> reproHighShelf;
    juce::dsp::WaveShaper<float> biasShaper;
    juce::dsp::WaveShaper<float> transformerShape;
    juce::dsp::IIR::Filter<float> lowpassOut;
    juce::dsp::Gain<float> outputGain;

    // --- UI state (atomic because web UI thread -> audio thread) ---
    std::atomic<float> uiInputDb { 0.0f };
    std::atomic<float> uiOutputDb { 0.0f };
    std::atomic<float> uiBiasDb { 0.0f };
    std::atomic<int>   uiTransformerOn { 1 };
    std::atomic<bool>  uiDirty { true };

    bool transformerEnabled = true; // cached on audio thread

    // VU metering (post-downsample, includes crosstalk)
    std::atomic<float> currentVUL { -60.0f };
    std::atomic<float> currentVUR { -60.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
