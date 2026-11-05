#pragma once
#include <JuceHeader.h>
#include <vector>

class SmartTahrirProcessor final : public juce::AudioProcessor
{
public:
    SmartTahrirProcessor();
    ~SmartTahrirProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Smart Tahrir Processor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    using Parameters = juce::AudioProcessorValueTreeState;
    Parameters parameters;

private:
    float readDelay (int channel, float delaySamples) const noexcept;
    float makeTahrirCurve (float phase, int style) const noexcept;

    double currentSampleRate = 44100.0;
    std::vector<std::vector<float>> delayMemory;
    int writePosition = 0;
    int delayCapacity = 0;
    double lfoPhase = 0.0;
    double voicedHoldSamples = 0.0;
    float gateEnvelope = 0.0f;
    int fixedLatencySamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartTahrirProcessor)
};
