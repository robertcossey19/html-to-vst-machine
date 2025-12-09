#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

HtmlToVstAudioProcessor::HtmlToVstAudioProcessor()
  : juce::AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout HtmlToVstAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (std::size_t i = 0; i < kNumHtmlToVstParams; ++i)
    {
        const auto& p = kHtmlToVstParams[i];

        const juce::String paramID   (p.id);
        const juce::String paramName (p.label[0] != '\0' ? p.label : p.id);
        const juce::String type      (p.type);

        if (type == "knob" || type == "slider")
        {
            auto range = juce::NormalisableRange<float> ((float)p.minValue, (float)p.maxValue);
            params.push_back (std::make_unique<juce::AudioParameterFloat> (
                paramID,
                paramName,
                range,
                (float)p.defaultValue
            ));
        }
        else if (type == "bool")
        {
            params.push_back (std::make_unique<juce::AudioParameterBool> (
                paramID,
                paramName,
                (p.defaultValue > 0.5)
            ));
        }
        else if (type == "enum")
        {
            // For now map enums to index space 0..N-1; GUI/engine will interpret indices
            int numChoices = (int)p.maxValue + 1; // because we mapped maxValue = N-1 in generator
            juce::StringArray choices;
            for (int j = 0; j < numChoices; ++j)
                choices.add (juce::String (j));

            params.push_back (std::make_unique<juce::AudioParameterChoice> (
                paramID,
                paramName,
                choices,
                (int)p.defaultValue
            ));
        }
        else
        {
            // fallback: treat like float
            auto range = juce::NormalisableRange<float> ((float)p.minValue, (float)p.maxValue);
            params.push_back (std::make_unique<juce::AudioParameterFloat> (
                paramID,
                paramName,
                range,
                (float)p.defaultValue
            ));
        }
    }

    return { params.begin(), params.end() };
}

//==============================================================================

void HtmlToVstAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

bool HtmlToVstAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Only support stereo in/out for now
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo()
     || layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

//==============================================================================

void HtmlToVstAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // -----------------------------------------------------------------
    // Retrieve parameters from APVTS
    // -----------------------------------------------------------------
    auto getFloatParam = [this] (const juce::String& id, float fallback) -> float
    {
        if (auto* p = apvts.getRawParameterValue (id))
            return p->load();
        return fallback;
    };

    // These IDs must match your currentSpec.json / GeneratedParams.h
    const float inDb   = getFloatParam ("inDb",   0.0f);
    const float outDb  = getFloatParam ("outDb",  0.0f);
    const float bias   = getFloatParam ("bias",   0.0f);

    // For now we won't use the others yet; they'll drive the full Ampex DSP later
    // const float speedIndex     = getFloatParam ("speed", 0.0f);
    // const float fluxIndex      = getFloatParam ("flux",  1.0f); // etc...
    // const float eqIndex        = getFloatParam ("eq",    0.0f);
    // const float tapeTypeIndex  = getFloatParam ("tapeType", 1.0f);
    // const float xfOn           = getFloatParam ("transformer", 1.0f);
    // const float autoCalOn      = getFloatParam ("autoCal", 1.0f);
    // const float headblockIndex = getFloatParam ("headblock", 0.0f);

    // -----------------------------------------------------------------
    // Simple DSP: input gain + output gain (dB)
    // This is the placeholder until we port the entire Ampex engine.
    // -----------------------------------------------------------------

    const float inGain  = juce::Decibels::decibelsToGain (inDb);
    const float outGain = juce::Decibels::decibelsToGain (outDb);

    // Apply input gain
    for (int ch = 0; ch < numChannels; ++ch)
        buffer.applyGain (ch, 0, numSamples, inGain);

    // TODO: place full Ampex 102 DSP here, using bias, speed, flux, eq, etc.

    // Apply output gain
    for (int ch = 0; ch < numChannels; ++ch)
        buffer.applyGain (ch, 0, numSamples, outGain);
}

//==============================================================================

void HtmlToVstAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Let APVTS handle state
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HtmlToVstAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

//==============================================================================

juce::AudioProcessorEditor* HtmlToVstAudioProcessor::createEditor()
{
    return new HtmlToVstAudioProcessorEditor (*this);
}
