#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

static float dbToGainSafe (float db)
{
    return juce::Decibels::decibelsToGain (db, -80.0f);
}

// IMPORTANT: non-capturing function (JUCE WaveShaper may require function pointer)
static float tanhShaper (float x)
{
    return std::tanh (x);
}

HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    // Use a plain function pointer (works with older JUCE)
    driveShaper.functionToUse = tanhShaper;
}

juce::AudioProcessorValueTreeState::ParameterLayout
HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<APF> (
        juce::ParameterID { "inGain", 1 },
        "Input Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f),
        0.0f));

    params.push_back (std::make_unique<APF> (
        juce::ParameterID { "drive", 1 },
        "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.0f));

    params.push_back (std::make_unique<APF> (
        juce::ParameterID { "outGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f),
        0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String HtmlToVstPluginAudioProcessor::getName() const { return JucePlugin_Name; }
bool HtmlToVstPluginAudioProcessor::acceptsMidi() const { return false; }
bool HtmlToVstPluginAudioProcessor::producesMidi() const { return false; }
bool HtmlToVstPluginAudioProcessor::isMidiEffect() const { return false; }
double HtmlToVstPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HtmlToVstPluginAudioProcessor::getNumPrograms() { return 1; }
int HtmlToVstPluginAudioProcessor::getCurrentProgram() { return 0; }
void HtmlToVstPluginAudioProcessor::setCurrentProgram (int) {}
const juce::String HtmlToVstPluginAudioProcessor::getProgramName (int) { return {}; }
void HtmlToVstPluginAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void HtmlToVstPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());

    inGain.prepare (spec);
    driveGain.prepare (spec);
    driveShaper.prepare (spec);
    outGain.prepare (spec);

    inGain.setRampDurationSeconds (0.01);
    driveGain.setRampDurationSeconds (0.01);
    outGain.setRampDurationSeconds (0.01);
}

void HtmlToVstPluginAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HtmlToVstPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == mainIn;
}
#endif

void HtmlToVstPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const float inDb   = apvts.getRawParameterValue ("inGain")->load();
    const float drive  = apvts.getRawParameterValue ("drive")->load();
    const float outDb  = apvts.getRawParameterValue ("outGain")->load();

    inGain.setGainLinear  (dbToGainSafe (inDb));
    outGain.setGainLinear (dbToGainSafe (outDb));

    // Drive scaling happens via a gain stage BEFORE the waveshaper (no capturing lambda!)
    const float k = 1.0f + 12.0f * juce::jlimit (0.0f, 1.0f, drive); // 1x..13x
    driveGain.setGainLinear (k);

    juce::dsp::AudioBlock<float> block (buffer);
    auto ctx = juce::dsp::ProcessContextReplacing<float> (block);

    inGain.process (ctx);
    driveGain.process (ctx);
    driveShaper.process (ctx);
    outGain.process (ctx);
}

//==============================================================================
bool HtmlToVstPluginAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* HtmlToVstPluginAudioProcessor::createEditor()
{
    return new HtmlToVstPluginAudioProcessorEditor (*this);
}

//==============================================================================
void HtmlToVstPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void HtmlToVstPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstPluginAudioProcessor();
}
