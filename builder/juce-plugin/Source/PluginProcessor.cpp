#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

HtmlToVstAudioProcessor::HtmlToVstAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true))
{
}

HtmlToVstAudioProcessor::~HtmlToVstAudioProcessor() = default;

const String HtmlToVstAudioProcessor::getName() const { return JucePlugin_Name; }

bool   HtmlToVstAudioProcessor::acceptsMidi() const            { return false; }
bool   HtmlToVstAudioProcessor::producesMidi() const           { return false; }
bool   HtmlToVstAudioProcessor::isMidiEffect() const           { return false; }
double HtmlToVstAudioProcessor::getTailLengthSeconds() const   { return 0.0; }

int  HtmlToVstAudioProcessor::getNumPrograms()                 { return 1; }
int  HtmlToVstAudioProcessor::getCurrentProgram()              { return 0; }
void HtmlToVstAudioProcessor::setCurrentProgram (int)          {}
const String HtmlToVstAudioProcessor::getProgramName (int)     { return {}; }
void HtmlToVstAudioProcessor::changeProgramName (int, const String&) {}

#if ! JucePlugin_PreferredChannelConfigurations
bool HtmlToVstAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}
#endif

void HtmlToVstAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    numChannels = jmax (1, getTotalNumOutputChannels());

    const auto osFactor = (uint32) oversampling.getOversamplingFactor();

    dsp::ProcessSpec spec;
    spec.sampleRate       = currentSampleRate * (double) osFactor;
    spec.maximumBlockSize = (uint32) (samplesPerBlock * osFactor);
    spec.numChannels      = (uint32) numChannels;

    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);

    inputGain.prepare (spec);
    fluxGain.prepare (spec);
    headroomGain.prepare (spec);
    headBump.prepare (spec);
    reproHighShelf.prepare (spec);
    biasShaper.prepare (spec);
    transformerShape.prepare (spec);
    lowpassOut.prepare (spec);
    outputGain.prepare (spec);

    headBump.reset();
    reproHighShelf.reset();
    lowpassOut.reset();

    updateDSP();
    uiDirty.store (true);
}

void HtmlToVstAudioProcessor::releaseResources() {}

void HtmlToVstAudioProcessor::setUiParameters (float inDb, float outDb, float biasDb, bool transformerOn)
{
    // clamp to sane UI ranges (matches your HTML)
    uiInputDb.store  (jlimit (-12.0f,  12.0f, inDb));
    uiOutputDb.store (jlimit (-24.0f,   6.0f, outDb));
    uiBiasDb.store   (jlimit ( -5.0f,   5.0f, biasDb));
    uiTransformerOn.store (transformerOn ? 1 : 0);
    uiDirty.store (true);
}

void HtmlToVstAudioProcessor::updateDSP()
{
    if (currentSampleRate <= 0.0)
        return;

    const auto osFactor = (double) oversampling.getOversamplingFactor();
    const auto osRate   = currentSampleRate * osFactor;

    // ---- DEFAULT CALIBRATION (static parts) ----
    // Flux:    250 nWb/m  (~ +2.6 dB)
    // Headroom: -12 dB
    const float fluxDb     = 2.6f;
    const float headroomDb = -12.0f;

    fluxGain.setGainDecibels (fluxDb);
    headroomGain.setGainDecibels (headroomDb);

    // --- Tape EQ: head bump + repro shelf (NAB @ 15 ips, 456) ---
    using Coeff = dsp::IIR::Coefficients<float>;

    {
        const float bumpFreq = 80.0f;
        const float bumpQ    = 1.1f;
        const float bumpGainDb = 2.4f; // base + small extra
        headBump.coefficients = Coeff::makePeakFilter (osRate, bumpFreq, bumpQ,
                                                       Decibels::decibelsToGain (bumpGainDb));
    }

    {
        const float shelfFreq = 3180.0f;
        const float shelfQ    = 0.707f;
        const float shelfDb   = -0.75f;
        reproHighShelf.coefficients = Coeff::makeHighShelf (osRate, shelfFreq, shelfQ,
                                                            Decibels::decibelsToGain (shelfDb));
    }

    // --- Transformer output curve (always defined; we bypass it when "off") ---
    transformerShape.functionToUse = [] (float x)
    {
        constexpr float k    = 3.0f;
        constexpr float gain = 0.98f;
        return std::tanh (x * k) * gain;
    };

    // --- Output LPF ~22 kHz at oversampled rate ---
    lowpassOut.coefficients = Coeff::makeLowPass (osRate, 22000.0f);
}

void HtmlToVstAudioProcessor::applyUiToDSPIfNeeded()
{
    if (! uiDirty.exchange (false))
        return;

    // Web version behavior:
    // - input knob is "UI gain" but record path has an offset
    const float inputOffsetDb = -5.0f;

    const float inDb  = uiInputDb.load();
    const float outDb = uiOutputDb.load();
    const float biasDb = uiBiasDb.load();
    transformerEnabled = (uiTransformerOn.load() != 0);

    // Output makeup to roughly match your chain calibration
    const float outMakeupDb = 1.0f;

    inputGain.setGainDecibels  (inDb + inputOffsetDb);
    outputGain.setGainDecibels (outDb + outMakeupDb);

    // Bias curve / asymmetry (updated when UI changes)
    biasShaper.functionToUse = [biasDb] (float x)
    {
        const float baseK = 1.6f;
        const float asym  = 1.0f - juce::jlimit (-0.25f, 0.25f, biasDb * 0.05f);

        const float kPos = baseK;
        const float kNeg = baseK / asym;

        return x >= 0.0f ? std::tanh (x * kPos)
                         : std::tanh (x * kNeg);
    };
}

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

        auto rms = std::sqrt (sum / jmax (1, numSamples));
        return Decibels::gainToDecibels ((float) rms, -100.0f);
    };

    const float lDb = buffer.getNumChannels() > 0 ? calcDb (buffer.getReadPointer (0)) : -90.0f;
    const float rDb = buffer.getNumChannels() > 1 ? calcDb (buffer.getReadPointer (1)) : lDb;

    currentVUL.store (lDb);
    currentVUR.store (rDb);
}

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

    // Apply UI-driven DSP values (gain/bias/transformer) when needed
    applyUiToDSPIfNeeded();

    dsp::AudioBlock<float> block (buffer);

    // --- 16x oversampling up ---
    auto osBlock = oversampling.processSamplesUp (block);
    dsp::ProcessContextReplacing<float> context (osBlock);

    // --- Chain ---
    inputGain.process (context);
    fluxGain.process (context);
    headroomGain.process (context);
    biasShaper.process (context);
    headBump.process (context);
    reproHighShelf.process (context);

    if (transformerEnabled)
        transformerShape.process (context);

    lowpassOut.process (context);
    outputGain.process (context);

    // --- Downsample back to project rate ---
    oversampling.processSamplesDown (block);

    // --- Stereo crosstalk matrix (after downsampling) ---
    if (buffer.getNumChannels() > 1)
    {
        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);

        const int n = buffer.getNumSamples();
        const float ct   = Decibels::decibelsToGain (-40.0f); // ~ -40 dB
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

bool HtmlToVstAudioProcessor::hasEditor() const { return true; }

AudioProcessorEditor* HtmlToVstAudioProcessor::createEditor()
{
    return new HtmlToVstAudioProcessorEditor (*this);
}

void HtmlToVstAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    MemoryOutputStream (destData, true).writeInt (0x41545231); // "ATR1"
}

void HtmlToVstAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    ignoreUnused (data, sizeInBytes);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstAudioProcessor();
}
