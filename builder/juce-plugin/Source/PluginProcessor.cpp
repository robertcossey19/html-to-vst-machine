#include "PluginProcessor.h"
#include "PluginEditor.h"

HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

HtmlToVstPluginAudioProcessor::~HtmlToVstPluginAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    using namespace juce;

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "gain", 1 },
        "Gain",
        NormalisableRange<float> (0.0f, 2.0f, 0.0001f),
        1.0f
    ));

    return { params.begin(), params.end() };
}

const juce::String HtmlToVstPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HtmlToVstPluginAudioProcessor::acceptsMidi() const { return false; }
bool HtmlToVstPluginAudioProcessor::producesMidi() const { return false; }
bool HtmlToVstPluginAudioProcessor::isMidiEffect() const { return false; }
double HtmlToVstPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HtmlToVstPluginAudioProcessor::getNumPrograms() { return 1; }
int HtmlToVstPluginAudioProcessor::getCurrentProgram() { return 0; }
void HtmlToVstPluginAudioProcessor::setCurrentProgram (int) {}
const juce::String HtmlToVstPluginAudioProcessor::getProgramName (int) { return {}; }
void HtmlToVstPluginAudioProcessor::changeProgramName (int, const juce::String&) {}

void HtmlToVstPluginAudioProcessor::prepareToPlay (double, int) {}
void HtmlToVstPluginAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HtmlToVstPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void HtmlToVstPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const float gain = *apvts.getRawParameterValue ("gain");

    for (int ch = 0; ch < totalNumInputChannels; ++ch)
        buffer.applyGain (ch, 0, buffer.getNumSamples(), gain);
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
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr)
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

void HtmlToVstPluginAudioProcessor::setParamNormalized (const juce::String& paramID, float normalized01)
{
    if (auto* p = apvts.getParameter (paramID))
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, normalized01));
}

float HtmlToVstPluginAudioProcessor::getParamNormalized (const juce::String& paramID) const
{
    if (auto* p = apvts.getParameter (paramID))
        return p->getValue();
    return 0.0f;
}
