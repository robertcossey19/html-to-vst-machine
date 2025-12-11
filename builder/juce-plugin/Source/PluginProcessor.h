#pragma once

#include <JuceHeader.h>
#include <atomic>

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
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // VU meter access (linear RMS 0–1)
    float getCurrentVUL() const noexcept { return currentVUL.load(); }
    float getCurrentVUR() const noexcept { return currentVUR.load(); }

private:
    void updateDSP();

    double currentSampleRate = 0.0;
    int    numChannels       = 0;

    juce::dsp::Oversampling<float> oversampling;

    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> fluxGain;
    juce::dsp::Gain<float> headroomGain;
    juce::dsp::Gain<float> outputGain;

    juce::dsp::IIR::Filter<float> headBump;
    juce::dsp::IIR::Filter<float> reproHighShelf;
    juce::dsp::IIR::Filter<float> lowpassOut;

    juce::dsp::WaveShaper<float> biasShaper;
    juce::dsp::WaveShaper<float> transformerShape;

    std::atomic<float> currentVUL { 0.0f };
    std::atomic<float> currentVUR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};

// Factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
