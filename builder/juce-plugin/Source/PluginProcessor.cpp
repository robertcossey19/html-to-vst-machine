#include "PluginProcessor.h"
#include "PluginEditor.h"

static inline float dbToGain (float db) noexcept
{
    return std::pow (10.0f, db / 20.0f);
}

static inline float softClipTanh (float x) noexcept
{
    // gentle, stable saturation
    return std::tanh (x);
}

juce::AudioProcessorValueTreeState::ParameterLayout
HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    using namespace juce;

    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    // Input/Output in dB
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { kParamInputGain, 1 },
        "Input Gain",
        NormalisableRange<float> (-24.0f, 24.0f, 0.01f),
        0.0f));

    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { kParamOutputGain, 1 },
        "Output Gain",
        NormalisableRange<float> (-24.0f, 24.0f, 0.01f),
        0.0f));

    // Drive: 0..1
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { kParamDrive, 1 },
        "Drive",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.25f));

    // Transformer on/off
    p.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { kParamTransformer, 1 },
        "Transformer",
        true));

    return { p.begin(), p.end() };
}

HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
: AudioProcessor (BusesProperties()
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
  apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

void HtmlToVstPluginAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    sr = sampleRate;
    inEnv = outEnv = 0.0f;
    inputMeter.store (0.0f);
    outputMeter.store (0.0f);
}

void HtmlToVstPluginAudioProcessor::releaseResources() {}

bool HtmlToVstPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return (in == out) && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void HtmlToVstPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int numS  = buffer.getNumSamples();

    // Read params
    const float inDb   = apvts.getRawParameterValue (kParamInputGain)->load();
    const float outDb  = apvts.getRawParameterValue (kParamOutputGain)->load();
    const float drive  = apvts.getRawParameterValue (kParamDrive)->load();
    const bool  xfmrOn = (apvts.getRawParameterValue (kParamTransformer)->load() >= 0.5f);

    const float inGain  = dbToGain (inDb);
    const float outGain = dbToGain (outDb);

    // Meter ballistics
    const float attack  = 0.02f;  // fast
    const float release = 0.002f; // slower

    float inPeak  = 0.0f;
    float outPeak = 0.0f;

    for (int ch = 0; ch < numCh; ++ch)
    {
        float* x = buffer.getWritePointer (ch);

        for (int i = 0; i < numS; ++i)
        {
            float s = x[i];

            // input gain
            s *= inGain;

            // transformer stage (toggleable)
            if (xfmrOn)
            {
                // drive shapes saturation amount; keep stable and audible
                const float d = 1.0f + 12.0f * drive;     // 1..13
                s = softClipTanh (s * d) / softClipTanh (d); // normalize-ish
            }

            // output gain
            s *= outGain;

            // write back
            x[i] = s;

            // metering (peak)
            inPeak  = juce::jmax (inPeak,  std::abs (s / (outGain == 0.0f ? 1.0f : outGain))); // approx pre-out
            outPeak = juce::jmax (outPeak, std::abs (s));
        }
    }

    // Smooth meters
    auto smooth = [] (float env, float x, float a, float r)
    {
        const float coeff = (x > env) ? a : r;
        return env + coeff * (x - env);
    };

    inEnv  = smooth (inEnv,  inPeak,  attack, release);
    outEnv = smooth (outEnv, outPeak, attack, release);

    inputMeter.store  (juce::jlimit (0.0f, 1.0f, inEnv));
    outputMeter.store (juce::jlimit (0.0f, 1.0f, outEnv));
}

juce::AudioProcessorEditor* HtmlToVstPluginAudioProcessor::createEditor()
{
    return new HtmlToVstPluginAudioProcessorEditor (*this);
}

void HtmlToVstPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HtmlToVstPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// This is required by JUCE plugin client
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstPluginAudioProcessor();
}
