#pragma once
#include <JuceHeader.h>

class HtmlToVstPluginAudioProcessor : public juce::AudioProcessor
{
public:
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
    juce::AudioProcessorValueTreeState apvts;

    // UI bridge
    void handleUiMessageUrl (const juce::String& url);

    // meters (linear 0..1)
    float getMeterL() const noexcept { return meterL.load(); }
    float getMeterR() const noexcept { return meterR.load(); }

private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float> meterL { 0.0f };
    std::atomic<float> meterR { 0.0f };

    // smoothed gain
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inGainSm;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outGainSm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessor)
};
