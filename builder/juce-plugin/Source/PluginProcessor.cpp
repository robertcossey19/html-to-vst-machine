#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================

HtmlToVstAudioProcessor::HtmlToVstAudioProcessor()
    :
#if ! JucePlugin_PreferredChannelConfigurations
      AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
#else
      AudioProcessor (BusesProperties()),
#endif
      // 16x oversampling = 4 stages of 2x
      oversampling (2, 4,
                    dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                    true)
{
}

HtmlToVstAudioProcessor::~HtmlToVstAudioProcessor() = default;

//==============================================================================

const String HtmlToVstAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HtmlToVstAudioProcessor::acceptsMidi() const       { return false; }
bool HtmlToVstAudioProcessor::producesMidi() const      { return false; }
bool HtmlToVstAudioProcessor::isMidiEffect() const      { return false; }
double HtmlToVstAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HtmlToVstAudioProcessor::getNumPrograms()           { return 1; }
int HtmlToVstAudioProcessor::getCurrentProgram()        { return 0; }
void HtmlToVstAudioProcessor::setCurrentProgram (int)   {}
const String HtmlToVstAudioProcessor::getProgramName (int) { return {}; }
void HtmlToVstAudioProcessor::changeProgramName (int, const String&) {}

//==============================================================================

#if ! JucePlugin_PreferredChannelConfigurations
bool HtmlToVstAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // We support only stereo <-> stereo
    if (layouts.getMainInputChannelSet()  != AudioChannelSet::stereo()
     || layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

//==============================================================================

void HtmlToVstAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;

    // 16× oversampling
    oversampling.reset();
    oversampling.initProcessing (static_cast<size_t> (samplesPerBlock));

    dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate * static_cast<double> (oversampling.getOversamplingFactor());
    spec.maximumBlockSize = static_cast<uint32> (samplesPerBlock * oversampling.getOversamplingFactor());
    spec.numChannels      = 2; // stereo

    inputGain.reset();
    fluxGain.reset();
    headroomGain.reset();
    repro.reset();
    monitorRe.reset();
    outGain.reset();
    xfTrim.reset();

    headBump.reset();
    deemph.reset();
    biasHF.reset();
    lpOut.reset();
    biasShaper.reset();
    transformerShape.reset();

    inputGain.prepare (spec);
    fluxGain.prepare (spec);
    headroomGain.prepare (spec);
    repro.prepare (spec);
    monitorRe.prepare (spec);
    outGain.prepare (spec);
    xfTrim.prepare (spec);

    headBump.prepare (spec);
    deemph.prepare (spec);
    biasHF.prepare (spec);
    lpOut.prepare (spec);
    biasShaper.prepare (spec);
    transformerShape.prepare (spec);

    currentVUL.store (0.0f);
    currentVUR.store (0.0f);

    updateDSP();
}

void HtmlToVstAudioProcessor::releaseResources()
{
}

//==============================================================================

void HtmlToVstAudioProcessor::updateDSP()
{
    const auto fs = lastSampleRate * static_cast<double> (oversampling.getOversamplingFactor());
    if (fs <= 0.0)
        return;

    // ---- DEFAULT CALIBRATION TO MATCH WEB VERSION ----
    //
    // Speed: 15 ips
    // Flux:  250 nWb/m
    // Tape:  456
    // Headroom: -12 dB
    // Input UI = 0 dB, but original UI had -5 dB offset in record path
    // Output UI = 0 dB
    // Transformer: ON
    //
    constexpr float inputDbOffset   = -5.0f;   // UI 0 dB = -5 dB into record path
    constexpr float defaultInDb     = 0.0f;
    constexpr float defaultOutDb    = 0.0f;
    constexpr float defaultFluxNwb  = 250.0f;
    constexpr float defaultHeadroom = -12.0f;
    constexpr float defaultBiasDb   = 1.8f;
    constexpr float defaultSpeedIps = 15.0f;
    constexpr float defaultTapeType = 456.0f;  // 456 base

    // Flux level → dB above reference (185 / 250 / 370 nWb/m)
    const float fluxDb = [&]()
    {
        if (defaultFluxNwb <= 190.0f) return 0.0f;
        if (defaultFluxNwb <= 260.0f) return 2.6f;   // ~250 nWb/m
        return 6.0f;                                 // ~370 nWb/m
    }();

    const float inDbEff   = defaultInDb  + inputDbOffset;
    const float outDbEff  = defaultOutDb;
    const float headroom  = defaultHeadroom;

    const float inGain  = std::pow (10.0f, inDbEff  / 20.0f);
    float       flGain  = std::pow (10.0f, fluxDb   / 20.0f);
    const float hrGain  = std::pow (10.0f, headroom / 20.0f);

    float outMakeupDb = 0.0f;
    const bool transformerOn = true;

    if (transformerOn)
    {
        flGain      *= std::pow (10.0f, -1.0f / 20.0f);  // ~ -1 dB before xf trim
        outMakeupDb += 1.0f;
    }

    inputGain.setGainLinear (inGain);
    fluxGain.setGainLinear  (flGain);
    headroomGain.setGainLinear (hrGain);

    repro.setGainLinear (1.0f);
    monitorRe.setGainLinear (1.0f);
    outGain.setGainDecibels (outDbEff + outMakeupDb);
    xfTrim.setGainLinear (1.0f);

    // --- Bias curve / asymmetry (matches WebAudio createBiasCurve) ---
    const float biasDb = defaultBiasDb;

    biasShaper.functionToUse = [biasDb] (float x)
    {
        const float baseK = 1.6f;
        const float asym  = 1.0f - juce::jlimit (-0.25f, 0.25f, biasDb * 0.05f);
        const float kPos  = baseK;
        const float kNeg  = baseK / asym;

        return x >= 0.0f ? std::tanh (x * kPos)
                         : std::tanh (x * kNeg);
    };

    // Bias high-shelf tilt (~±3 dB @ 8 kHz)
    const float biasTiltDb = juce::jlimit (-3.0f, 3.0f, -0.6f * biasDb);

    using Coeff = dsp::IIR::Coefficients<float>;

    auto biasCoeffs = Coeff::makeHighShelf (fs,
                                            8000.0,
                                            0.707,
                                            std::pow (10.0f, biasTiltDb / 20.0f));
    biasHF.state = *biasCoeffs;

    // --- Tape EQ: NAB vs IEC curves from WebAudio getEQ() ---
    const bool useIEC = (defaultSpeedIps >= 30.0f); // 30 ips → IEC, 15 ips default NAB

    struct EqDef { float hfFreq; float hfDb; float bumpFreq; float bumpDb; };
    EqDef eq {};

    if (! useIEC)
    {
        if (defaultSpeedIps <= 8.0f)
            eq = { 3180.0f, -1.25f, 60.0f,  2.4f };  // 7.5 ips NAB
        else if (defaultSpeedIps < 20.0f)
            eq = { 3180.0f, -0.75f, 80.0f,  1.8f };  // 15 ips NAB
        else
            eq = { 3180.0f, -0.50f,100.0f,  1.4f };  // 30 ips NAB
    }
    else
    {
        if (defaultSpeedIps <= 8.0f)
            eq = { 2270.0f, -0.75f, 55.0f,  1.8f };
        else if (defaultSpeedIps < 20.0f)
            eq = { 4550.0f, -0.25f, 75.0f,  1.6f };
        else
            eq = { 9100.0f,  0.00f, 95.0f,  0.7f };
    }

    // Tape formula extra bump (456, 499, GP9, SM900, SM911)
    const float tapeBumpExtra = [=]()
    {
        if (defaultTapeType < 410.0f)                       return 0.3f; // 406
        if (defaultTapeType > 440.0f && defaultTapeType < 470.0f) return 0.4f; // 456
        if (std::abs (defaultTapeType - 499.0f) < 0.5f)     return 0.2f;
        if (std::abs (defaultTapeType - 900.0f) < 5.0f)     return -0.1f; // SM900
        if (std::abs (defaultTapeType - 911.0f) < 0.5f)     return 0.1f;
        return 0.0f;
    }();

    const float bumpDb = eq.bumpDb + tapeBumpExtra;

    auto bumpCoeffs = Coeff::makePeakFilter (fs,
                                             eq.bumpFreq,
                                             1.1f,
                                             std::pow (10.0f, bumpDb / 20.0f));
    headBump.state = *bumpCoeffs;

    auto deemphCoeffs = Coeff::makeHighShelf (fs,
                                              eq.hfFreq,
                                              0.707f,
                                              std::pow (10.0f, eq.hfDb / 20.0f));
    deemph.state = *deemphCoeffs;

    // --- Transformer output curve (matches WebAudio createTransformerOutCurve) ---
    transformerShape.functionToUse = [] (float x)
    {
        constexpr float k    = 3.0f;
        constexpr float gain = 0.98f;
        return std::tanh (x * k) * gain;
    };

    // --- Output bandwidth limit ~ 22 kHz at 768 kHz OS ---
    const double lpHz = transformerOn ? 22000.0 : 22050.0;
    auto lpCoeffs = Coeff::makeLowPass (fs, lpHz);
    lpOut.state = *lpCoeffs;
}

//==============================================================================

void HtmlToVstAudioProcessor::processBlock (AudioBuffer<float>& buffer,
                                            MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    ignoreUnused (midi);

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() == 0)
        return;

    dsp::AudioBlock<float> block (buffer);

    // --- 16x oversampling up ---
    auto oversampledBlock = oversampling.processSamplesUp (block);
    dsp::AudioBlock<float> procBlock { oversampledBlock };
    dsp::ProcessContextReplacing<float> context { procBlock };

    // --- Core ATR-style chain at 768 kHz ---
    inputGain.process        (context);
    fluxGain.process         (context);
    headroomGain.process     (context);
    biasShaper.process       (context);
    biasHF.process           (context);
    headBump.process         (context);
    repro.process            (context);
    deemph.process           (context);
    monitorRe.process        (context);
    outGain.process          (context);
    xfTrim.process           (context);
    transformerShape.process (context);
    lpOut.process            (context);

    // --- Downsample back to project rate ---
    oversampling.processSamplesDown (block);

    // --- VU metering on post-processing signal ---
    float sumL = 0.0f, sumR = 0.0f;
    const auto* left  = buffer.getReadPointer (0);
    const auto* right = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr;

    if (right != nullptr)
    {
        const int n = buffer.getNumSamples();

        for (int i = 0; i < n; ++i)
        {
            const float l = left [i];
            const float r = right[i];

            sumL += l * l;
            sumR += r * r;
        }

        const float rmsL = std::sqrt (sumL / static_cast<float> (n) + 1.0e-12f);
        const float rmsR = std::sqrt (sumR / static_cast<float> (n) + 1.0e-12f);

        const float dbL = 20.0f * std::log10 (rmsL);
        const float dbR = 20.0f * std::log10 (rmsR);

        const float vuL = juce::jlimit (-40.0f, 6.0f, dbL);
        const float vuR = juce::jlimit (-40.0f, 6.0f, dbR);

        currentVUL.store (vuL);
        currentVUR.store (vuR);
    }
}

//==============================================================================

bool HtmlToVstAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* HtmlToVstAudioProcessor::createEditor()
{
    return new HtmlToVstAudioProcessorEditor (*this);
}

//==============================================================================

void HtmlToVstAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    MemoryOutputStream stream (destData, true);
    stream.writeFloat (0.0f);
}

void HtmlToVstAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstAudioProcessor();
}
