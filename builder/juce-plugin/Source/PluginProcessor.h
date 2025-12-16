#pragma once

#include <JuceHeader.h>

class HtmlToVstPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    HtmlToVstPluginAudioProcessor();
    ~HtmlToVstPluginAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                                { return true; }

    //==============================================================================
    const juce::String getName() const override                     { return JucePlugin_Name; }
    bool acceptsMidi() const override                               { return false; }
    bool producesMidi() const override                              { return false; }
    bool isMidiEffect() const override                              { return false; }
    double getTailLengthSeconds() const override                    { return 0.0; }

    //==============================================================================
    int getNumPrograms() override                                   { return 1; }
    int getCurrentProgram() override                                { return 0; }
    void setCurrentProgram (int) override                           {}
    const juce::String getProgramName (int) override                { return {}; }
    void changeProgramName (int, const juce::String&) override      {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    // Meter values (linear 0..1 RMS-ish)
    float getInputMeter()  const noexcept { return inputMeter.load(); }
    float getOutputMeter() const noexcept { return outputMeter.load(); }

    // Parameter IDs (must match UI injection expectations)
    static constexpr const char* kParamInputGain   = "inputGain";
    static constexpr const char* kParamOutputGain  = "outputGain";
    static constexpr const char* kParamDrive       = "drive";
    static constexpr const char* kParamTransformer = "transformer";

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    std::atomic<float> inputMeter  { 0.0f };
    std::atomic<float> outputMeter { 0.0f };

    double sr = 44100.0;

    // Simple smoothing for meters
    float inEnv  = 0.0f;
    float outEnv = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessor)
};
