/*
  ==============================================================================

    PluginProcessor.cpp

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    static float parseBoolishTo01 (const juce::String& s, float fallback = 0.0f)
    {
        auto t = s.trim().toLowerCase();

        if (t == "1" || t == "true" || t == "yes" || t == "on")
            return 1.0f;

        if (t == "0" || t == "false" || t == "no" || t == "off")
            return 0.0f;

        return fallback;
    }

    // StringMap does NOT exist in your JUCE.
    // StringPairArray DOES exist across JUCE versions.
    static juce::StringPairArray parseQuery (const juce::String& query)
    {
        juce::StringPairArray out;

        auto q = query;
        if (q.startsWithChar ('?'))
            q = q.substring (1);

        for (auto part : juce::StringArray::fromTokens (q, "&", ""))
        {
            if (part.isEmpty())
                continue;

            auto key = part.upToFirstOccurrenceOf ("=", false, false);
            auto val = part.fromFirstOccurrenceOf ("=", false, false);

            key = juce::URL::removeEscapeChars (key);
            val = juce::URL::removeEscapeChars (val);

            if (key.isNotEmpty())
                out.set (key, val);
        }

        return out;
    }
}

HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("params"), createParameterLayout())
{
    parameters.addParameterListener ("gain", this);
    parameters.addParameterListener ("bypass", this);
}

HtmlToVstPluginAudioProcessor::~HtmlToVstPluginAudioProcessor()
{
    parameters.removeParameterListener ("gain", this);
    parameters.removeParameterListener ("bypass", this);
}

const juce::String HtmlToVstPluginAudioProcessor::getName() const
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

int HtmlToVstPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int HtmlToVstPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HtmlToVstPluginAudioProcessor::setCurrentProgram (int)
{
}

const juce::String HtmlToVstPluginAudioProcessor::getProgramName (int)
{
    return {};
}

void HtmlToVstPluginAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void HtmlToVstPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    outputMeterL = 0.0f;
    outputMeterR = 0.0f;
}

void HtmlToVstPluginAudioProcessor::releaseResources()
{
}

bool HtmlToVstPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void HtmlToVstPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto* gainParam   = parameters.getRawParameterValue ("gain");
    auto* bypassParam = parameters.getRawParameterValue ("bypass");

    const float gain   = gainParam != nullptr ? gainParam->load() : 1.0f;
    const bool bypass  = bypassParam != nullptr ? (bypassParam->load() > 0.5f) : false;

    if (! bypass)
        buffer.applyGain (gain);

    // crude peak meters (smoothed)
    const float peakL = buffer.getNumChannels() > 0 ? buffer.getMagnitude (0, 0, buffer.getNumSamples()) : 0.0f;
    const float peakR = buffer.getNumChannels() > 1 ? buffer.getMagnitude (1, 0, buffer.getNumSamples()) : peakL;

    outputMeterL = 0.95f * outputMeterL + 0.05f * peakL;
    outputMeterR = 0.95f * outputMeterR + 0.05f * peakR;
}

bool HtmlToVstPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* HtmlToVstPluginAudioProcessor::createEditor()
{
    return new HtmlToVstPluginAudioProcessorEditor (*this);
}

void HtmlToVstPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HtmlToVstPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr)
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "gain", "Gain",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f),
        1.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "bypass", "Bypass",
        false
    ));

    return { params.begin(), params.end() };
}

void HtmlToVstPluginAudioProcessor::parameterChanged (const juce::String&, float)
{
    // (optional) push parameter changes to UI later if desired
}

float HtmlToVstPluginAudioProcessor::getOutputMeterL() const { return outputMeterL; }
float HtmlToVstPluginAudioProcessor::getOutputMeterR() const { return outputMeterR; }

void HtmlToVstPluginAudioProcessor::setParamById (const juce::String& paramId, float value01)
{
    if (auto* p = parameters.getParameter (paramId))
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value01));
}

void HtmlToVstPluginAudioProcessor::handleUiMessageUrl (const juce::String& rawUrl)
{
    auto url = rawUrl.trim();

    // Expected forms:
    // juce://set?param=gain&value=0.7
    // juce://set?param=bypass&value=true

    auto qPos = url.indexOfChar ('?');
    auto query = (qPos >= 0) ? url.substring (qPos + 1) : juce::String();

    auto map = parseQuery (query);

    const auto action = map.getValue ("action", map.getValue ("a", "")).trim().toLowerCase();

    if (action == "set" || url.containsIgnoreCase ("://set"))
    {
        const auto param = map.getValue ("param", map.getValue ("p", "")).trim();
        const auto val   = map.getValue ("value", map.getValue ("v", "")).trim();

        if (param.isNotEmpty())
        {
            if (param == "bypass")
                setParamById (param, parseBoolishTo01 (val, 0.0f));
            else
                setParamById (param, (float) val.getDoubleValue());
        }
    }
}
