#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <vector>
#include <cmath>

using namespace juce;

// Normalize loose UI ids to real parameter IDs
static String normaliseParamId (String s)
{
    s = s.trim().toLowerCase();

    const bool hasGain = s.contains ("gain");
    const bool hasIn   = s.contains ("in")  || s.contains ("input")  || s.contains ("pre");
    const bool hasOut  = s.contains ("out") || s.contains ("output") || s.contains ("post");

    if (hasGain && hasIn)  return "inGainDb";
    if (hasGain && hasOut) return "outGainDb";

    if (s.contains ("transform")) return "transformerOn";

    // Already a real ID?
    if (s == "ingaindb" || s == "outgaindb" || s == "transformeron")
        return s;

    return s;
}

AudioProcessorValueTreeState::ParameterLayout HtmlToVstAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> (
        "inGainDb", "Input Gain",
        NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

    p.push_back (std::make_unique<AudioParameterFloat> (
        "outGainDb", "Output Gain",
        NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

    p.push_back (std::make_unique<AudioParameterBool> (
        "transformerOn", "Transformer", true));

    return { p.begin(), p.end() };
}

//==============================================================================
HtmlToVstAudioProcessor::HtmlToVstAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  AudioChannelSet::stereo(), true)
        .withOutput ("Output", AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "STATE", createParameterLayout())
{
}

HtmlToVstAudioProcessor::~HtmlToVstAudioProcessor() = default;

//==============================================================================
const String HtmlToVstAudioProcessor::getName() const { return JucePlugin_Name; }
bool   HtmlToVstAudioProcessor::acceptsMidi() const { return false; }
bool   HtmlToVstAudioProcessor::producesMidi() const { return false; }
bool   HtmlToVstAudioProcessor::isMidiEffect() const { return false; }
double HtmlToVstAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HtmlToVstAudioProcessor::getNumPrograms() { return 1; }
int HtmlToVstAudioProcessor::getCurrentProgram() { return 0; }
void HtmlToVstAudioProcessor::setCurrentProgram (int) {}
const String HtmlToVstAudioProcessor::getProgramName (int) { return {}; }
void HtmlToVstAudioProcessor::changeProgramName (int, const String&) {}

//==============================================================================
#if ! JucePlugin_PreferredChannelConfigurations
bool HtmlToVstAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet()  != AudioChannelSet::stereo()
     || layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

//==============================================================================
void HtmlToVstAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    numChannels = jmax (1, getTotalNumOutputChannels());

    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);

    const auto osFactor = (double) oversampling.getOversamplingFactor();

    dsp::ProcessSpec spec;
    spec.sampleRate       = currentSampleRate * osFactor;
    spec.maximumBlockSize = (uint32) (samplesPerBlock * osFactor);
    spec.numChannels      = (uint32) numChannels;

    inputGain.prepare (spec);
    fluxGain.prepare (spec);
    headroomGain.prepare (spec);
    biasShaper.prepare (spec);
    headBump.prepare (spec);
    reproHighShelf.prepare (spec);
    transformerShape.prepare (spec);
    lowpassOut.prepare (spec);
    outputGain.prepare (spec);

    headBump.reset();
    reproHighShelf.reset();
    lowpassOut.reset();

    // Non-capturing shapers (works even if your JUCE WaveShaper expects float(*)(float))
    biasShaper.functionToUse = [] (float x) noexcept
    {
        constexpr float k = 1.6f;
        return std::tanh (x * k);
    };

    transformerShape.functionToUse = [] (float x) noexcept
    {
        constexpr float k = 3.0f;
        constexpr float gain = 0.98f;
        return std::tanh (x * k) * gain;
    };

    updateFixedFilters();
}

void HtmlToVstAudioProcessor::releaseResources() {}

//==============================================================================
void HtmlToVstAudioProcessor::updateFixedFilters()
{
    if (currentSampleRate <= 0.0)
        return;

    const auto osRate = currentSampleRate * (double) oversampling.getOversamplingFactor();
    using Coeff = dsp::IIR::Coefficients<float>;

    // Head bump
    {
        const float bumpFreq   = 80.0f;
        const float bumpQ      = 1.1f;
        const float bumpGainDb = 2.4f;
        *headBump.state = *Coeff::makePeakFilter (osRate, bumpFreq, bumpQ,
                                                  Decibels::decibelsToGain (bumpGainDb));
    }

    // Repro shelf
    {
        const float shelfFreq = 3180.0f;
        const float shelfQ    = 0.707f;
        const float shelfDb   = -0.75f;
        *reproHighShelf.state = *Coeff::makeHighShelf (osRate, shelfFreq, shelfQ,
                                                       Decibels::decibelsToGain (shelfDb));
    }

    // Output LPF
    {
        const float lpFreq = 22000.0f;
        *lowpassOut.state = *Coeff::makeLowPass (osRate, lpFreq);
    }
}

//==============================================================================
void HtmlToVstAudioProcessor::setParameterFromUI (const String& paramIdOrAlias, float value)
{
    const auto id = normaliseParamId (paramIdOrAlias);

    if (auto* p = apvts.getParameter (id))
    {
        const float v01 = jlimit (0.0f, 1.0f, p->convertTo0to1 (value));

        p->beginChangeGesture();
        p->setValueNotifyingHost (v01);
        p->endChangeGesture();
    }
}

//==============================================================================
void HtmlToVstAudioProcessor::updateVuFromBuffer (AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    auto calcDb = [numSamples] (const float* data) -> float
    {
        double sum = 0.0;
        for (int i = 0; i < numSamples; ++i)
            sum += (double) data[i] * (double) data[i];

        const auto rms = std::sqrt (sum / (double) jmax (1, numSamples));
        return Decibels::gainToDecibels ((float) rms, -100.0f);
    };

    const float lDb = buffer.getNumChannels() > 0 ? calcDb (buffer.getReadPointer (0)) : -90.0f;
    const float rDb = buffer.getNumChannels() > 1 ? calcDb (buffer.getReadPointer (1)) : lDb;

    currentVUL.store (lDb);
    currentVUR.store (rDb);
}

//==============================================================================
void HtmlToVstAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    ignoreUnused (midi);

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() == 0)
        return;

    // --- Read params (audio-thread safe) ---
    const float inDbUI  = apvts.getRawParameterValue ("inGainDb")->load();
    const float outDbUI = apvts.getRawParameterValue ("outGainDb")->load();
    const bool transformerOn = apvts.getRawParameterValue ("transformerOn")->load() > 0.5f;

    // --- Calibration ---
    const float inputOffsetDb = -5.0f;
    const float outMakeupDb   =  1.0f;

    inputGain.setGainDecibels  (inDbUI  + inputOffsetDb);
    outputGain.setGainDecibels (outDbUI + outMakeupDb);

    // Fixed “flux/headroom”
    fluxGain.setGainDecibels (2.6f);
    headroomGain.setGainDecibels (-12.0f);

    dsp::AudioBlock<float> block (buffer);

    // Up-sample
    auto osBlock = oversampling.processSamplesUp (block);
    dsp::ProcessContextReplacing<float> ctx (osBlock);

    // DSP chain
    inputGain.process (ctx);
    fluxGain.process (ctx);
    headroomGain.process (ctx);

    biasShaper.process (ctx);
    headBump.process (ctx);
    reproHighShelf.process (ctx);

    if (transformerOn)
        transformerShape.process (ctx);

    lowpassOut.process (ctx);
    outputGain.process (ctx);

    // Down-sample
    oversampling.processSamplesDown (block);

    // Simple stereo crosstalk (post)
    if (buffer.getNumChannels() > 1)
    {
        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);
        const int n = buffer.getNumSamples();

        const float ct = Decibels::decibelsToGain (-40.0f);
        const float main = 1.0f - ct;

        for (int i = 0; i < n; ++i)
        {
            const float l = left[i];
            const float r = right[i];
            left[i]  = main * l + ct * r;
            right[i] = main * r + ct * l;
        }
    }

    updateVuFromBuffer (buffer);
}

//==============================================================================
bool HtmlToVstAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* HtmlToVstAudioProcessor::createEditor()
{
    return new HtmlToVstAudioProcessorEditor (*this);
}

//==============================================================================
void HtmlToVstAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HtmlToVstAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (ValueTree::fromXml (*xml));
}

//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstAudioProcessor();
}
