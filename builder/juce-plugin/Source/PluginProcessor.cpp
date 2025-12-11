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

const String HtmlToVstAudioProcessor::getName() const   { return JucePlugin_Name; }

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
    currentSampleRate = sampleRate;
    numChannels       = jmax (1, getTotalNumOutputChannels());

    dsp::ProcessSpec spec;
    spec.sampleRate       = currentSampleRate * (double) oversampling.getOversamplingFactor();
    spec.maximumBlockSize = (uint32) (samplesPerBlock * oversampling.getOversamplingFactor());
    spec.numChannels      = (uint32) numChannels;

    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);

    inputGain.prepare       (spec);
    fluxGain.prepare        (spec);
    headroomGain.prepare    (spec);
    headBump.prepare        (spec);
    reproHighShelf.prepare  (spec);
    biasShaper.prepare      (spec);
    transformerShape.prepare(spec);
    lowpassOut.prepare      (spec);
    outputGain.prepare      (spec);

    // Linear phase where possible
    headBump.reset();
    reproHighShelf.reset();
    lowpassOut.reset();

    updateDSP();
}

void HtmlToVstAudioProcessor::releaseResources()
{
}

//==============================================================================

void HtmlToVstAudioProcessor::updateDSP()
{
    if (currentSampleRate <= 0.0)
        return;

    const auto osFactor = (double) oversampling.getOversamplingFactor();
    const auto osRate   = currentSampleRate * osFactor;

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
    const float uiInputDb      = 0.0f;
    const float uiOutputDb     = 0.0f;
    const float inputOffsetDb  = -5.0f;   // GUI 0 dB == -5 dB at record path
    const float headroomDb     = -12.0f;
    const float fluxDb         = 2.6f;    // 250 nWb/m ~= +2.6 dB over ref
    const float biasDb         = 1.8f;    // over-bias calibration

    // Input + calibration
    const float inputDbEff = uiInputDb + inputOffsetDb;
    const float inGainLin  = Decibels::decibelsToGain (inputDbEff);
    const float fluxLin    = Decibels::decibelsToGain (fluxDb);
    const float headroomLin= Decibels::decibelsToGain (headroomDb);
    const float outMakeupDb = 1.0f; // transformer & flux compensation
    const float outDbEff   = uiOutputDb + outMakeupDb;

    inputGain.setGainLinear    (inGainLin);
    fluxGain.setGainLinear     (fluxLin);
    headroomGain.setGainLinear (headroomLin);
    outputGain.setGainDecibels (outDbEff);

    // --- Bias curves / asymmetry ---
    biasShaper.function = [biasDb] (float x)
    {
        // Slightly different curvature for positive vs negative to bias even/odd.
        const float baseK = 1.6f;
        const float asym  = 1.0f - jlimit (-0.25f, 0.25f, biasDb * 0.05f); // ±0.25
        const float kPos  = baseK;
        const float kNeg  = baseK / asym;

        return x >= 0.0f ? std::tanh (x * kPos)
                         : std::tanh (x * kNeg);
    };

    // --- Tape EQ: head bump + repro shelf (NAB @ 15 ips, 456) ---
    {
        const float bumpFreq = 80.0f;
        const float bumpQ    = 1.1f;
        const float bumpGain = 2.0f + 0.4f; // base + extra for 456

        using Coeff = dsp::IIR::Coefficients<float>;

        headBump.coefficients = Coeff::makePeakFilter (osRate,
                                                       bumpFreq,
                                                       bumpQ,
                                                       Decibels::decibelsToGain (bumpGain));
    }

    {
        // Repro high shelf: NAB @ 15 ips, slightly negative tilt
        const float shelfFreq = 3180.0f;
        const float shelfQ    = 0.707f;
        const float shelfDb   = -0.75f;

        using Coeff = dsp::IIR::Coefficients<float>;

        reproHighShelf.coefficients = Coeff::makeHighShelf (osRate,
                                                            shelfFreq,
                                                            shelfQ,
                                                            Decibels::decibelsToGain (shelfDb));
    }

    // --- Transformer output curve ---
    transformerShape.function = [] (float x)
    {
        // Moderate 3rd/5th harmonic emphasis, kept under unity
        constexpr float k    = 3.0f;
        constexpr float gain = 0.98f;
        return std::tanh (x * k) * gain;
    };

    // --- Output bandwidth limit ~ 22 kHz at 768 kHz OS ---
    {
        using Coeff = dsp::IIR::Coefficients<float>;
        const float lpFreq = 22000.0f;
        lowpassOut.coefficients = Coeff::makeLowPass (osRate, lpFreq);
    }

    // xfTrimTarget is left at 1.0 here – transformer loudness matching
    // (the WebAudio implementation tweaked this dynamically; we can extend
    // that in the future by analyzing RMS and nudging xfTrimTarget).
}

//==============================================================================

void HtmlToVstAudioProcessor::processBlock (AudioBuffer<float>& buffer,
                                            MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    ignoreUnused (midi);

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() == 0)
        return;

    dsp::AudioBlock<float> block (buffer);

    // --- 16x oversampling up ---
    auto osBlock = oversampling.processSamplesUp (block);
    dsp::ProcessContextReplacing<float> context (osBlock);

    // --- Core ATR-style chain at 768 kHz ---
    inputGain.process       (context);
    fluxGain.process        (context);
    headroomGain.process    (context);
    biasShaper.process      (context);
    headBump.process        (context);
    reproHighShelf.process  (context);
    transformerShape.process(context);
    lowpassOut.process      (context);
    outputGain.process      (context);

    // --- Downsample back to project rate ---
    oversampling.processSamplesDown (block);

    // --- Stereo crosstalk matrix (after downsampling) ---
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    if (right != nullptr)
    {
        const int n = buffer.getNumSamples();
        const float ct   = Decibels::decibelsToGain (-40.0f);   // ~ -40 dB crosstalk
        const float main = 1.0f - ct;                           // keep level ~ unity

        for (int i = 0; i < n; ++i)
        {
            const float l = left[i];
            const float r = right[i];

            left[i]  = main * l + ct * r;
            right[i] = main * r + ct * l;
        }
    }
}

//==============================================================================

bool HtmlToVstAudioProcessor::hasEditor() const                      { return true; }
AudioProcessorEditor* HtmlToVstAudioProcessor::createEditor()        { return new HtmlToVstAudioProcessorEditor (*this); }

//==============================================================================

void HtmlToVstAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // For now: no parameters, just placeholder blob.
    MemoryOutputStream (destData, true).writeInt (0x41545231); // "ATR1"
}

void HtmlToVstAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    ignoreUnused (data, sizeInBytes);
}

//==============================================================================

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstAudioProcessor();
}
