#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

HtmlToVstPluginAudioProcessor::~HtmlToVstPluginAudioProcessor() = default;

AudioProcessorValueTreeState::ParameterLayout HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // Normalized parameters 0..1 (UI sends 0..1)
    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "gain", 1 }, "Gain",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        1.0f)); // default unity

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        1.0f)); // default fully wet

    // Optional “drive” example param (won’t hurt if UI never uses it)
    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "drive", 1 }, "Drive",
        NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.0f));

    return { params.begin(), params.end() };
}

const String HtmlToVstPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HtmlToVstPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HtmlToVstPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HtmlToVstPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HtmlToVstPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HtmlToVstPluginAudioProcessor::getNumPrograms() { return 1; }
int HtmlToVstPluginAudioProcessor::getCurrentProgram() { return 0; }
void HtmlToVstPluginAudioProcessor::setCurrentProgram (int) {}
const String HtmlToVstPluginAudioProcessor::getProgramName (int) { return {}; }
void HtmlToVstPluginAudioProcessor::changeProgramName (int, const String&) {}

void HtmlToVstPluginAudioProcessor::prepareToPlay (double, int) {}
void HtmlToVstPluginAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HtmlToVstPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return (in == AudioChannelSet::mono() || in == AudioChannelSet::stereo());
}
#endif

void HtmlToVstPluginAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Read normalized params
    const float gain01  = apvts.getRawParameterValue ("gain")->load();   // 0..1
    const float mix01   = apvts.getRawParameterValue ("mix")->load();    // 0..1
    const float drive01 = apvts.getRawParameterValue ("drive")->load();  // 0..1

    // Map gain01 to a useful dB range (so the UI actually *feels* like a volume knob)
    // -60 dB .. +12 dB
    const float gainDb  = jmap (gain01, -60.0f, 12.0f);
    const float gainLin = Decibels::decibelsToGain (gainDb);

    // Simple soft clip “drive” (optional)
    const float driveAmt = jmap (drive01, 1.0f, 8.0f);

    AudioBuffer<float> dry;
    dry.makeCopyOf (buffer, true);

    // Wet path = gain + optional drive
    buffer.applyGain (gainLin);

    if (drive01 > 0.0001f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* p = buffer.getWritePointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                const float x = p[n] * driveAmt;
                // tanh soft clip
                p[n] = std::tanh (x);
            }
        }
    }

    // Mix: output = dry*(1-mix) + wet*mix
    const float dryAmt = 1.0f - mix01;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wetPtr = buffer.getWritePointer (ch);
        auto* dryPtr = dry.getReadPointer (ch);

        for (int n = 0; n < buffer.getNumSamples(); ++n)
            wetPtr[n] = dryPtr[n] * dryAmt + wetPtr[n] * mix01;
    }
}

bool HtmlToVstPluginAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* HtmlToVstPluginAudioProcessor::createEditor()
{
    return new HtmlToVstPluginAudioProcessorEditor (*this);
}

void HtmlToVstPluginAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HtmlToVstPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (ValueTree::fromXml (*xml));
}

void HtmlToVstPluginAudioProcessor::setParamNormalized (const String& paramID, float normalized01)
{
    normalized01 = jlimit (0.0f, 1.0f, normalized01);
    if (auto* p = apvts.getParameter (paramID))
        p->setValueNotifyingHost (normalized01);
}

float HtmlToVstPluginAudioProcessor::getParamNormalized (const String& paramID) const
{
    if (auto* p = apvts.getParameter (paramID))
        return p->getValue();
    return 0.0f;
}

//==============================================================================
// JUCE needs this symbol in the final binary:
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstPluginAudioProcessor();
}
