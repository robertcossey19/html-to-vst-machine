#pragma once

#include <atomic>
#include <cmath>

#if __has_include(<JuceHeader.h>)
  #include <JuceHeader.h>
#else
  #include <juce_audio_processors/juce_audio_processors.h>
  #include <juce_dsp/juce_dsp.h>
  #include <juce_gui_basics/juce_gui_basics.h>
  #include <juce_gui_extra/juce_gui_extra.h>
#endif

class HtmlToVstPluginAudioProcessor : public juce::AudioProcessor
{
public:
    HtmlToVstPluginAudioProcessor();
    ~HtmlToVstPluginAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

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

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // HTML UI -> DSP bridge (called by editor when JS triggers a juce: URL)
    void handleUiMessageUrl (const juce::String& url);

    float getOutputMeterL() const noexcept { return meterL.load (std::memory_order_relaxed); }
    float getOutputMeterR() const noexcept { return meterR.load (std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void updateDspFromParams();

    juce::dsp::Gain<float> inGain;
    juce::dsp::Gain<float> outGain;

    bool transformerOn = true;

    std::atomic<float> meterL { 0.0f };
    std::atomic<float> meterR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessor)
};
