#pragma once

#include <JuceHeader.h>
#include <atomic>

//==============================================================================
class HtmlToVstPluginAudioProcessor  : public juce::AudioProcessor,
                                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    HtmlToVstPluginAudioProcessor();
    ~HtmlToVstPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Meter values for the UI thread (0..1 peak-ish)
    float getOutputMeterL() const noexcept { return outputMeterL.load(); }
    float getOutputMeterR() const noexcept { return outputMeterR.load(); }

    //==============================================================================
    // Keep APVTS public (typical JUCE pattern so the editor can attach to params).
    juce::AudioProcessorValueTreeState apvts;

    // Backwards-compatible alias: your PluginProcessor.cpp was written using "parameters".
    // This makes both names valid without duplicating state.
    juce::AudioProcessorValueTreeState& parameters = apvts;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // AudioProcessorValueTreeState::Listener
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void syncCachedParameters();

    // Cached params (thread-safe)
    std::atomic<float> cachedGain   { 1.0f };
    std::atomic<bool>  cachedBypass { false };

    // Meter values (written on audio thread, read on UI thread)
    std::atomic<float> outputMeterL { 0.0f };
    std::atomic<float> outputMeterR { 0.0f };

    // Smoothing state (audio thread only)
    float meterSmoothL = 0.0f;
    float meterSmoothR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessor)
};
