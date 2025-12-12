#include "PluginProcessor.h"
#include "PluginEditor.h"

static float clamp01 (float x) noexcept
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static bool startsWithIgnoreCase (const juce::String& s, const juce::String& prefix)
{
    return s.substring (0, prefix.length()).compareIgnoreCase (prefix) == 0;
}

// Parses query like: key1=val1&key2=val2
static juce::StringPairArray parseQuery (const juce::String& query)
{
    juce::StringPairArray out;
    auto parts = juce::StringArray::fromTokens (query, "&", "");
    for (auto& p : parts)
    {
        auto eq = p.indexOfChar ('=');
        if (eq <= 0) continue;
        auto k = p.substring (0, eq).trim();
        auto v = p.substring (eq + 1).trim();
        out.set (k, v);
    }
    return out;
}

HtmlToVstPluginAudioProcessor::HtmlToVstPluginAudioProcessor()
: AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                  ),
  apvts (*this, nullptr, "STATE", createParameterLayout())
{
}

HtmlToVstPluginAudioProcessor::~HtmlToVstPluginAudioProcessor() = default;

const juce::String HtmlToVstPluginAudioProcessor::getName() const { return JucePlugin_Name; }

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
const juce::String HtmlToVstPluginAudioProcessor::getProgramName (int) { return {}; }
void HtmlToVstPluginAudioProcessor::changeProgramName (int, const juce::String&) {}

void HtmlToVstPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    inGainSm.reset (sampleRate, 0.02);
    outGainSm.reset (sampleRate, 0.02);
    inGainSm.setCurrentAndTargetValue (1.0f);
    outGainSm.setCurrentAndTargetValue (1.0f);

    meterL.store (0.0f);
    meterR.store (0.0f);
}

void HtmlToVstPluginAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HtmlToVstPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
}
#endif

void HtmlToVstPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Parameters (IDs we expose)
    const float inDb  = apvts.getRawParameterValue ("inputGainDb")->load();
    const float outDb = apvts.getRawParameterValue ("outputGainDb")->load();
    const bool xfmr   = (apvts.getRawParameterValue ("transformerOn")->load() >= 0.5f);

    inGainSm.setTargetValue (juce::Decibels::decibelsToGain (inDb));
    outGainSm.setTargetValue (juce::Decibels::decibelsToGain (outDb));

    float meterAccumL = 0.0f;
    float meterAccumR = 0.0f;

    // Simple DSP: input gain -> optional transformer saturation -> output gain
    for (int s = 0; s < numSamples; ++s)
    {
        const float gIn  = inGainSm.getNextValue();
        const float gOut = outGainSm.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float x = buffer.getSample (ch, s);

            // input gain
            x *= gIn;

            // transformer-ish saturation
            if (xfmr)
            {
                // light drive with a little asymmetry
                const float drive = 1.6f;
                const float asym  = 0.92f;
                const float y = (x >= 0.0f)
                                ? std::tanh (x * drive)
                                : std::tanh (x * drive / asym);
                x = y;
            }

            // output gain
            x *= gOut;

            buffer.setSample (ch, s, x);

            const float a = std::abs (x);
            if (ch == 0) meterAccumL = juce::jmax (meterAccumL, a);
            if (ch == 1) meterAccumR = juce::jmax (meterAccumR, a);
        }
    }

    // peak meters -> a bit of decay
    const float prevL = meterL.load();
    const float prevR = meterR.load();
    const float dec   = 0.92f;

    meterL.store (clamp01 (juce::jmax (meterAccumL, prevL * dec)));
    meterR.store (clamp01 (juce::jmax (meterAccumR, prevR * dec)));
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

juce::AudioProcessorValueTreeState::ParameterLayout HtmlToVstPluginAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<APF> (
        "inputGainDb", "Input Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<APF> (
        "outputGainDb", "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<APB> (
        "transformerOn", "Transformer", true));

    return { params.begin(), params.end() };
}

//==============================================================
// UI MESSAGE BRIDGE
//
// Supported messages from HTML:
//
// juce://set?param=inputGainDb&value=0.5&norm=1
// juce://set?param=inputGainDb&value=-6
// juce://set?param=transformerOn&value=0
//
// Also accepts aliases: gain -> inputGainDb, output -> outputGainDb, transformer -> transformerOn
//
void HtmlToVstPluginAudioProcessor::handleUiMessageUrl (const juce::String& url)
{
    // Expect: juce://set?... OR juce:set?... OR juce://param?...
    auto u = url.trim();

    auto stripScheme = [&]() -> juce::String
    {
        if (startsWithIgnoreCase (u, "juce://")) return u.fromFirstOccurrenceOf ("juce://", false, true);
        if (startsWithIgnoreCase (u, "juce:"))   return u.fromFirstOccurrenceOf ("juce:",   false, true);
        return {};
    };

    auto payload = stripScheme();
    if (payload.isEmpty())
        return;

    // payload example: "set?param=...&value=...&norm=1"
    auto path  = payload.upToFirstOccurrenceOf ("?", false, false);
    auto query = payload.fromFirstOccurrenceOf ("?", false, false);

    auto q = parseQuery (query);

    juce::String param = q.getValue ("param", q.getValue ("id", "")).trim();
    if (param.isEmpty())
        param = q.getValue ("name", "").trim();

    auto valueStr = q.getValue ("value", q.getValue ("v", "")).trim();
    if (param.isEmpty() || valueStr.isEmpty())
        return;

    // aliases to match whatever the HTML happens to send
    auto pLower = param.toLowerCase();
    if (pLower == "gain" || pLower == "inputgain" || pLower == "ingain") param = "inputGainDb";
    if (pLower == "output" || pLower == "outgain" || pLower == "outputgain") param = "outputGainDb";
    if (pLower == "transformer" || pLower == "xfmr") param = "transformerOn";

    auto* p = apvts.getParameter (param);
    if (p == nullptr)
        return;

    const bool norm = (q.getValue ("norm", "0").getIntValue() != 0);

    float v = (float) valueStr.getDoubleValue();

    if (norm)
    {
        v = clamp01 (v);
        p->setValueNotifyingHost (v);
        return;
    }

    // If it's a bool param, treat any >= 0.5 as on
    if (dynamic_cast<juce::AudioParameterBool*> (p) != nullptr)
    {
        p->setValueNotifyingHost (v >= 0.5f ? 1.0f : 0.0f);
        return;
    }

    // Otherwise interpret as "actual value" in the param's range
    if (auto* apf = dynamic_cast<juce::AudioParameterFloat*> (p))
    {
        auto range = apf->range;
        auto clamped = juce::jlimit (range.start, range.end, v);
        auto normed  = range.convertTo0to1 (clamped);
        p->setValueNotifyingHost (normed);
        return;
    }

    // Fallback: try normalized directly
    p->setValueNotifyingHost (clamp01 (v));
}
