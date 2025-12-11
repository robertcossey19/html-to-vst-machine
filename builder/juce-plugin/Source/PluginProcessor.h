#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
//  HtmlToVstAudioProcessor
//  Core DSP: 16x oversampled ATR-style chain with head bump, repro EQ,
//  transformer non-linearity and stereo crosstalk.
//
//  NOTE: UI controls are not yet mapped – this uses a fixed default
//  calibration matching the WebAudio defaults (15 ips, 250 nWb, 456, stereo).
//==============================================================================

class HtmlToVstAudioProcessor : public juce::AudioProcessor
{
public:
    HtmlToVstAudioProcessor();
    ~HtmlToVstAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

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

private:
    // Recalculate gains / EQ when sample rate or calibration changes.
    void updateDSP();

    // 16x oversampling: 48k -> 768k
    juce::dsp::Oversampling<float> oversampling;

    // Core tape chain at oversampled rate
    juce::dsp::Gain<float>       inputGain;      // UI input + old -5 dB offset
    juce::dsp::Gain<float>       fluxGain;       // reference level (fluxivity)
    juce::dsp::Gain<float>       headroomGain;   // "headroom dB" pre-nonlinearity
    juce::dsp::IIR::Filter<float> headBump;      // LF head bump
    juce::dsp::IIR::Filter<float> reproHighShelf;// repro HF EQ
    juce::dsp::WaveShaper<float> biasShaper;     // subtle asymmetry / bias
    juce::dsp::WaveShaper<float> transformerShape;// output transformer curve
    juce::dsp::IIR::Filter<float> lowpassOut;    // HF bandwidth limitation
    juce::dsp::Gain<float>       outputGain;     // UI output + transformer makeup

    double currentSampleRate = 44100.0;
    float  xfTrimTarget      = 1.0f;             // transformer loudness trim
    int    numChannels       = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessor)
};
