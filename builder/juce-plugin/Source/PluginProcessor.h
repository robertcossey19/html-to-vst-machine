#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>
#include <functional>

//==============================================================================
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
    float getVuLDb() const noexcept { return currentVUL.load(); }
    float getVuRDb() const noexcept { return currentVUR.load(); }

private:
    void updateDSP();
    void updateVuFromBuffer (juce::AudioBuffer<float>& buffer);

    double currentSampleRate = 0.0;
    int    numChannels       = 2;

    // 16x oversampling = 2^4 stages
    juce::dsp::Oversampling<float> oversampling { 2, 4, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };

    juce::dsp::Gain<float> inputGain, fluxGain, headroomGain, outputGain;
    juce::dsp::IIR::Filter<float> headBump, reproHighShelf, lowpassOut;

    using ShaperFn = std::function<float(float)>;
    juce::dsp::WaveShaper<float, ShaperFn> biasShaper;
    juce::dsp::WaveShaper<float, ShaperFn> transformerShape;

    std::atomic<float> currentVUL { -60.0f };
    std::atomic<float> currentVUR { -60.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
