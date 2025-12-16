#include "PluginProcessor.h"
#include "PluginEditor.h"

static float dbToGainSafe (float db) { return juce::Decibels::decibelsToGain (db, -80.0f); }

HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
: AudioProcessor (BusesProperties()
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
  apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("inGain", 1), "Input Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("drive", 1), "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("outGain", 1), "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

    return { p.begin(), p.end() };
}

const juce::String HtmlToVstPluginAudioProcessor::getName() const { return JucePlugin_Name; }

bool HtmlToVstPluginAudioProcessor::acceptsMidi() const  { return false; }
bool HtmlToVstPluginAudioProcessor::producesMidi() const { return false; }
bool HtmlToVstPluginAudioProcessor::isMidiEffect() const { return false; }
double HtmlToVstPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HtmlToVstPluginAudioProcessor::getNumPrograms() { return 1; }
int HtmlToVstPluginAudioProcessor::getCurrentProgram() { return 0; }
void HtmlToVstPluginAudioProcessor::setCurrentProgram (int) {}
const juce::String HtmlToVstPluginAudioProcessor::getProgramName (int) { return {}; }
void HtmlToVstPluginAudioProcessor::changeProgramName (int, const juce::String&) {}

void HtmlToVstPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    inGain.prepare (spec);
    outGain.prepare (spec);

    driveShaper.functionToUse = [] (float x) { return std::tanh (x); };
}

void HtmlToVstPluginAudioProcessor::releaseResources() {}

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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const float inDb   = apvts.getRawParameterValue ("inGain")->load();
    const float drive  = apvts.getRawParameterValue ("drive")->load();
    const float outDb  = apvts.getRawParameterValue ("outGain")->load();

    inGain.setGainLinear (dbToGainSafe (inDb));
    outGain.setGainLinear (dbToGainSafe (outDb));

    juce::dsp::AudioBlock<float> block (buffer);

    // Input gain
    inGain.process (juce::dsp::ProcessContextReplacing<float> (block));

    // Drive (simple tanh, scaled by drive)
    if (drive > 0.0001f)
    {
        const float k = 1.0f + 12.0f * drive; // up to ~13x
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                d[n] = std::tanh (d[n] * k);
        }
    }

    // Output gain
    outGain.process (juce::dsp::ProcessContextReplacing<float> (block));
}

bool HtmlToVstPluginAudioProcessor::hasEditor() const { return true; }

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
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}
