#pragma once
#include <JuceHeader.h>
#include <vector>
class SmartTahrirProcessor final : public juce::AudioProcessor
{
public:
 SmartTahrirProcessor();
 ~SmartTahrirProcessor() override = default;
 void prepareToPlay(double sampleRate, int samplesPerBlock) override;
 void releaseResources() override;
 void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override { return true; }
 const juce::String getName() const override { return "HADI"; }
 bool acceptsMidi() const override { return false; }
 bool producesMidi() const override { return false; }
 bool isMidiEffect() const override { return false; }
 double getTailLengthSeconds() const override { return 0.05; }
 int getNumPrograms() override { return 1; }
 int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override { return {}; }
 void changeProgramName(int, const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*, int) override;
 using Parameters = juce::AudioProcessorValueTreeState;
 Parameters parameters;
private:
 float readDelay(int channel, float delaySamples) const noexcept;
 float makeTahrirCurve(double phase, int style) const noexcept;
 void analysePitch() noexcept;
 double currentSampleRate = 44100.0;
 std::vector<std::vector<float>> delayMemory;
 int writePosition = 0, delayCapacity = 0, baseDelaySamples = 0, pitchWindowSamples = 0;
 double phaseA = 0.0, phaseB = 0.5, offsetA = 0.0, offsetB = 0.0, tahrirPhase = 0.0;
 double voicedHoldSamples = 0.0;
 float gateEnvelope = 0.0f;
 std::vector<float> analysisBuffer;
 int analysisWrite = 0, analysisFill = 0, samplesUntilAnalysis = 0, decimationCounter = 0;
 float decimationAccumulator = 0.0f, detectedF0 = 0.0f, pitchConfidence = 0.0f, previousF0 = 0.0f;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartTahrirProcessor)
};
