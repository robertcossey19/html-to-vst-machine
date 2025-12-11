#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
class HtmlToVstAudioProcessor : public juce::AudioProcessor
{
public:
    //==========================================================================
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

    //==========================================================================
    // Required AudioProcessor overrides (JUCE headless / modular API)

    const juce::String getName() const override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // MIDI / tail
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    // Programs (we just expose a single program)
    int getNumPrograms() override;                    // must be > 0
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    // Editor
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    // State
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // VU meter access for PluginEditor
    float getCurrentVUL() const noexcept { return currentVUL.load(); }
    float getCurrentVUR() const noexcept { return currentVUR.load(); }

private:
    //==========================================================================
    void updateDSP();

    // Core audio config
    double currentSampleRate = 44100.0;
    int    numChannels       = 2;

    // 16x oversampling (2^4)
    juce::dsp::Oversampling<float> oversampling;

    // Gain stages
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> fluxGain;
    juce::dsp::Gain<float> headroomGain;
    juce::dsp::Gain<float> outputGain;

    // EQ / filters
    juce::dsp::IIR::Filter<float> headBump;
    juce::dsp::IIR::Filter<float> reproHighShelf;
    juce::dsp::IIR::Filter<float> lowpassOut;

    // Simple non-linear bias & transformer are applied sample-wise in processBlock
    float biasDb        = 1.8f;   // default over-bias calibration
    float transformerOn = 1.0f;   // 1 = on, 0 = off (UI later)

    // VU meters (atomics so editor can poll safely)
    std::atomic<float> currentVUL { 0.0f };
    std::atomic<float> currentVUR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
