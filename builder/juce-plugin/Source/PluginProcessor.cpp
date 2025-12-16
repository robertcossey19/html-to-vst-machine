#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static inline float linToDbSafe (float lin)
{
    if (lin <= 0.0000001f)
        return -100.0f;
    return Decibels::gainToDecibels (lin, -100.0f);
}

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

    // Normalized [0..1] floats (we’ll map to dB etc in processBlock)
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "inputGain",  1 }, "Input Gain",
                                                             NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.5f));

    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "outputGain", 1 }, "Output Gain",
                                                             NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.5f));

    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix",        1 }, "Mix",
                                                             NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 1.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive",      1 }, "Drive",
                                                             NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.0f));

    params.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "transformer", 1 }, "Transformer", false));
    params.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "bypass",      1 }, "Bypass",      false));

    return { params.begin(), params.end() };
}

const String HtmlToVstPluginAudioProcessor::getName() const { return JucePlugin_Name; }

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

double HtmlToVstPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

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

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Read params
    const float in01   = apvts.getRawParameterValue ("inputGain")->load();
    const float out01  = apvts.getRawParameterValue ("outputGain")->load();
    const float mix01  = apvts.getRawParameterValue ("mix")->load();
    const float drive01= apvts.getRawParameterValue ("drive")->load();
    const bool  xfm    = (apvts.getRawParameterValue ("transformer")->load() > 0.5f);
    const bool  byp    = (apvts.getRawParameterValue ("bypass")->load() > 0.5f);

    // Map normalized [0..1] -> dB ranges
    // Center (0.5) = 0 dB, left = -24 dB, right = +24 dB
    const float inDb  = jmap (in01,  -24.0f, 24.0f);
    const float outDb = jmap (out01, -24.0f, 24.0f);

    const float inLin  = Decibels::decibelsToGain (inDb);
    const float outLin = Decibels::decibelsToGain (outDb);

    // Drive amount (subtle -> heavier)
    const float driveAmt = jmap (drive01, 1.0f, 10.0f);

    // Snapshot dry for mix/bypass
    AudioBuffer<float> dry;
    dry.makeCopyOf (buffer, true);

    // Meter input (pre anything)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            peak = jmax (peak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
        inputMeterDb.store (linToDbSafe (peak));
    }

    if (! byp)
    {
        // Input gain
        buffer.applyGain (inLin);

        // “Transformer” coloration (simple but audible)
        if (xfm || drive01 > 0.0001f)
        {
            const float sat = (xfm ? 1.25f : 1.0f) * driveAmt;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* p = buffer.getWritePointer (ch);
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                {
                    const float x = p[n] * sat;
                    // tanh soft clip
                    p[n] = std::tanh (x);
                }
            }
        }

        // Output gain
        buffer.applyGain (outLin);

        // Mix
        const float wet = mix01;
        const float dryAmtMix = 1.0f - wet;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wetPtr = buffer.getWritePointer (ch);
            auto* dryPtr = dry.getReadPointer (ch);

            for (int n = 0; n < buffer.getNumSamples(); ++n)
                wetPtr[n] = dryPtr[n] * dryAmtMix + wetPtr[n] * wet;
        }
    }
    else
    {
        // hard bypass -> original audio
        buffer.makeCopyOf (dry, true);
    }

    // Meter output
    {
        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            peak = jmax (peak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
        outputMeterDb.store (linToDbSafe (peak));
    }
}

bool HtmlToVstPluginAudioProcessor::hasEditor() const { return true; }

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

String HtmlToVstPluginAudioProcessor::canonicalParamId (String in)
{
    in = in.trim().toLowerCase();

    // Common aliases from HTML UIs
    if (in == "in" || in == "input" || in == "inputgain" || in == "ingain" || in == "input_gain")
        return "inputGain";

    if (in == "out" || in == "output" || in == "outputgain" || in == "outgain" || in == "output_gain")
        return "outputGain";

    if (in == "xfmr" || in == "xformer" || in == "transform" || in == "transformer" || in == "tx")
        return "transformer";

    if (in == "sat" || in == "saturation" || in == "drive")
        return "drive";

    if (in == "blend")
        return "mix";

    return in; // already canonical or unknown
}

void HtmlToVstPluginAudioProcessor::setParamNormalized (const String& paramID, float normalized01)
{
    const auto id = canonicalParamId (paramID);
    normalized01 = jlimit (0.0f, 1.0f, normalized01);

    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (normalized01);
}

float HtmlToVstPluginAudioProcessor::getParamNormalized (const String& paramID) const
{
    const auto id = canonicalParamId (paramID);

    if (auto* p = apvts.getParameter (id))
        return p->getValue();

    return 0.0f;
}

// JUCE required factory symbol
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstPluginAudioProcessor();
}
