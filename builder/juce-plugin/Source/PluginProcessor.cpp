#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <memory>
#include <vector>

//==============================================================================
HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       #if ! JucePlugin_IsMidiEffect
                        #if ! JucePlugin_IsSynth
                         .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        #endif
                         .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       #endif
                       ),
       apvts (*this, nullptr, juce::Identifier ("params"), createParameterLayout())
#else
     : apvts (*this, nullptr, juce::Identifier ("params"), createParameterLayout())
#endif
{
    // Listener hooks (matches your build log intent)
    parameters.addParameterListener ("gain",   this);
    parameters.addParameterListener ("bypass", this);

    // Initialize cached values
    syncCachedParameters();
}

HtmlToVstPluginAudioProcessor::~HtmlToVstPluginAudioProcessor()
{
    parameters.removeParameterListener ("gain",   this);
    parameters.removeParameterListener ("bypass", this);
}

//==============================================================================
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
    return 1;   // NB: some hosts don't cope well if you return 0 here
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

//==============================================================================
void HtmlToVstPluginAudioProcessor::prepareToPlay (double, int)
{
    meterSmoothL = 0.0f;
    meterSmoothR = 0.0f;
    outputMeterL.store (0.0f);
    outputMeterR.store (0.0f);
}

void HtmlToVstPluginAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HtmlToVstPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    const auto mainOut = layouts.getMainOutputChannelSet();

    // Only allow mono or stereo
    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    // Input must match output
    if (mainOut != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void HtmlToVstPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples             = buffer.getNumSamples();

    // Clear any channels that don't have input data
    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    const bool  bypassed = cachedBypass.load();
    const float gain     = cachedGain.load();

    if (! bypassed)
        buffer.applyGain (gain);

    // --- Simple peak meters (0..1), smoothed ---
    float peakL = 0.0f;
    float peakR = 0.0f;

    if (totalNumOutputChannels > 0)
    {
        const float* data = buffer.getReadPointer (0);
        for (int i = 0; i < numSamples; ++i)
            peakL = juce::jmax (peakL, std::abs (data[i]));
    }

    if (totalNumOutputChannels > 1)
    {
        const float* data = buffer.getReadPointer (1);
        for (int i = 0; i < numSamples; ++i)
            peakR = juce::jmax (peakR, std::abs (data[i]));
    }
    else
    {
        peakR = peakL;
    }

    // Match the smoothing style from your failing code (0.95/0.05)
    meterSmoothL = 0.95f * meterSmoothL + 0.05f * peakL;
    meterSmoothR = 0.95f * meterSmoothR + 0.05f * peakR;

    outputMeterL.store (meterSmoothL);
    outputMeterR.store (meterSmoothR);
}

//==============================================================================
bool HtmlToVstPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* HtmlToVstPluginAudioProcessor::createEditor()
{
    return new HtmlToVstPluginAudioProcessorEditor (*this);
}

//==============================================================================
void HtmlToVstPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());

    juce::copyXmlToBinary (*xml, destData);
}

void HtmlToVstPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (juce::getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
    {
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
        syncCachedParameters();
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.reserve (2);

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "gain",
        "Gain",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f),
        1.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "bypass",
        "Bypass",
        false));

    return { params.begin(), params.end() };
}

//==============================================================================
void HtmlToVstPluginAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "gain")
    {
        cachedGain.store (newValue);
    }
    else if (parameterID == "bypass")
    {
        cachedBypass.store (newValue >= 0.5f);
    }
}

void HtmlToVstPluginAudioProcessor::syncCachedParameters()
{
    if (auto* g = parameters.getRawParameterValue ("gain"))
        cachedGain.store (g->load());

    if (auto* b = parameters.getRawParameterValue ("bypass"))
        cachedBypass.store (b->load() >= 0.5f);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstPluginAudioProcessor();
}
