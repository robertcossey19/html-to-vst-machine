#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    static float parseBoolishTo01 (juce::String s)
    {
        s = s.trim().toLowerCase();
        if (s == "1" || s == "true" || s == "yes" || s == "on")  return 1.0f;
        if (s == "0" || s == "false" || s == "no"  || s == "off") return 0.0f;
        return s.getFloatValue();
    }

    static juce::String canonicalParamId (juce::String id)
    {
        id = id.trim();
        if (id.isEmpty()) return {};

        auto lower = id.toLowerCase();

        if (lower == "input" || lower == "ingain" || lower == "inputgain" || lower == "inputgaindb")
            return "inputGainDb";

        if (lower == "output" || lower == "outgain" || lower == "outputgain" || lower == "outputgaindb")
            return "outputGainDb";

        if (lower == "xfmr" || lower == "transformer" || lower == "transformeron" || lower == "transform")
            return "transformerOn";

        return id;
    }

    static juce::StringMap<juce::String> parseQuery (const juce::String& query)
    {
        juce::StringMap<juce::String> out;

        juce::String q = query;
        if (q.startsWithChar ('?')) q = q.substring (1);

        juce::StringArray parts;
        parts.addTokens (q, "&", "");
        for (auto& p : parts)
        {
            auto key = p.upToFirstOccurrenceOf ("=", false, false);
            auto val = p.fromFirstOccurrenceOf ("=", false, false);

            key = juce::URL::removeEscapeChars (key);
            val = juce::URL::removeEscapeChars (val);

            if (key.isNotEmpty())
                out.set (key, val);
        }

        return out;
    }

    static void setParamById (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float realValue)
    {
        if (auto* p = apvts.getParameter (id))
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p);
            if (ranged != nullptr)
            {
                const float norm = ranged->convertTo0to1 (realValue);
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
            }
        }
    }
}

HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
#endif
    , apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

HtmlToVstPluginAudioProcessor::~HtmlToVstPluginAudioProcessor() = default;

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
    spec.numChannels = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());

    inGain.prepare (spec);
    outGain.prepare (spec);

    inGain.setRampDurationSeconds (0.02);
    outGain.setRampDurationSeconds (0.02);

    updateDspFromParams();
}

void HtmlToVstPluginAudioProcessor::releaseResources()
{
}

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

void HtmlToVstPluginAudioProcessor::updateDspFromParams()
{
    const float inDb  = apvts.getRawParameterValue ("inputGainDb")->load();
    const float outDb = apvts.getRawParameterValue ("outputGainDb")->load();
    transformerOn      = apvts.getRawParameterValue ("transformerOn")->load() >= 0.5f;

    inGain.setGainDecibels (inDb);
    outGain.setGainDecibels (outDb);
}

void HtmlToVstPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);

    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateDspFromParams();

    // in gain
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        inGain.process (ctx);
    }

    // simple "transformer" saturation (no captured lambdas, no JUCE internals)
    if (transformerOn)
    {
        const float drive = 2.2f;
        const float norm  = 1.0f / std::tanh (drive);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                const float x = d[n];
                d[n] = std::tanh (x * drive) * norm;
            }
        }
    }

    // out gain
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        outGain.process (ctx);
    }

    // meters (peak, with gentle release)
    const float peakL = buffer.getNumChannels() > 0 ? buffer.getMagnitude (0, 0, buffer.getNumSamples()) : 0.0f;
    const float peakR = buffer.getNumChannels() > 1 ? buffer.getMagnitude (1, 0, buffer.getNumSamples()) : peakL;

    const float release = 0.90f;

    const float prevL = meterL.load (std::memory_order_relaxed);
    const float prevR = meterR.load (std::memory_order_relaxed);

    meterL.store (juce::jmax (peakL, prevL * release), std::memory_order_relaxed);
    meterR.store (juce::jmax (peakR, prevR * release), std::memory_order_relaxed);
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
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<APF> (
        "inputGainDb", "Input Gain (dB)",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f),
        0.0f));

    params.push_back (std::make_unique<APF> (
        "outputGainDb", "Output Gain (dB)",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f),
        0.0f));

    params.push_back (std::make_unique<APB> (
        "transformerOn", "Transformer",
        true));

    return { params.begin(), params.end() };
}

void HtmlToVstPluginAudioProcessor::handleUiMessageUrl (const juce::String& url)
{
    // Accept patterns like:
    //   juce:set?param=inputGainDb&value=-6
    //   juce://inputGainDb?value=-6
    //   juce://set?name=transformer&value=0
    juce::String s = url.trim();

    if (s.startsWithIgnoreCase ("juce:"))
        s = s.fromFirstOccurrenceOf ("juce:", false, false);

    if (s.startsWith ("//"))
        s = s.substring (2);

    const auto head  = s.upToFirstOccurrenceOf ("?", false, false);
    const auto query = s.fromFirstOccurrenceOf ("?", false, false);

    const auto map = parseQuery (query);

    juce::String param = map.getWithDefault ("param", {});
    if (param.isEmpty()) param = map.getWithDefault ("name", {});
    if (param.isEmpty()) param = map.getWithDefault ("id", {});
    if (param.isEmpty()) param = head; // e.g. juce://inputGainDb?value=...

    param = canonicalParamId (param);
    if (param.isEmpty())
        return;

    juce::String valueStr = map.getWithDefault ("value", {});
    if (valueStr.isEmpty()) valueStr = map.getWithDefault ("v", {});
    if (valueStr.isEmpty())
        return;

    const float v = parseBoolishTo01 (valueStr);

    setParamById (apvts, param, v);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstPluginAudioProcessor();
}
