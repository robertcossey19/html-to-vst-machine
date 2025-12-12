#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class HtmlToVstAudioProcessor final : public juce::AudioProcessor
{
public:
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
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

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // UI bridge entrypoint (called from PluginEditor when web UI sends updates)
    void setParameterFromUI (const juce::String& paramIdOrAlias, float value);

    // Meters for UI
    float getVUL() const noexcept { return currentVUL.load(); }
    float getVUR() const noexcept { return currentVUR.load(); }

    // Access APVTS for editor polling (thread-safe reads only)
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

private:
    //==============================================================================
    void updateFixedFilters();
    void updateVuFromBuffer (juce::AudioBuffer<float>& buffer);

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    juce::dsp::Oversampling<float> oversampling { 2, 4,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };

    double currentSampleRate = 0.0;
    int numChannels = 2;

    // DSP chain
    juce::dsp::Gain<float> inputGain, fluxGain, headroomGain, outputGain;
    juce::dsp::WaveShaper<float> biasShaper;
    juce::dsp::IIR::Filter<float> headBump, reproHighShelf, lowpassOut;
    juce::dsp::WaveShaper<float> transformerShape;

    // meters
    std::atomic<float> currentVUL { -90.0f };
    std::atomic<float> currentVUR { -90.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
