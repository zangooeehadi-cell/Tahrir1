#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SmartTahrirEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SmartTahrirEditor (SmartTahrirProcessor&);
    ~SmartTahrirEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SmartTahrirProcessor& processor;
    juce::Slider enhance, autoTahrir, hold, depth, speed;
    juce::ComboBox style;
    juce::Label title, subtitle, status, enhanceLabel, autoLabel, holdLabel, depthLabel, speedLabel, styleLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> enhanceAttachment, autoAttachment, holdAttachment, depthAttachment, speedAttachment;
    std::unique_ptr<ComboAttachment> styleAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartTahrirEditor)
};
