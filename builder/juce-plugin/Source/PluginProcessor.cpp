#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    // Add parameters here later if you need them.
    // Example:
    // std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    // params.push_back (std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f, 1.0f, 0.5f));
    // return { params.begin(), params.end() };

    return {};
}

//==============================================================================
HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : juce::AudioProcessor (BusesProperties()
                          #if ! JucePlugin_IsMidiEffect
                           #if ! JucePlugin_IsSynth
                            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                           #endif
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          #endif
                            ),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
#else
    : apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
}

HtmlToVstPluginAudioProcessor::~HtmlToVstPluginAudioProcessor() = default;

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
    return 1;
}

int HtmlToVstPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HtmlToVstPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String HtmlToVstPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void HtmlToVstPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void HtmlToVstPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
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

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
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
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Passthrough (no DSP yet)
}

void HtmlToVstPluginAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer,
                                                 juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Passthrough (no DSP yet)
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
    // IMPORTANT FIX:
    // In your JUCE version, these helpers are NOT in the juce:: namespace.
    // They are available via AudioProcessor, so call them unqualified here.
    auto state = apvts.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void HtmlToVstPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // IMPORTANT FIX:
    // Call getXmlFromBinary unqualified (AudioProcessor helper).
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstPluginAudioProcessor();
}
