#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
class TahrirCurveEditor final : public juce::Component {
public:
 TahrirCurveEditor();
 void paint(juce::Graphics&) override; void mouseDown(const juce::MouseEvent&) override; void mouseDrag(const juce::MouseEvent&) override;
 void setCurve(const std::array<float,SmartTahrirProcessor::curveSize>&);
 const std::array<float,SmartTahrirProcessor::curveSize>& getCurve()const noexcept{return curve;}
 std::function<void(const std::array<float,SmartTahrirProcessor::curveSize>&)> onCurveChanged;
private:
 void editAt(juce::Point<float>); std::array<float,SmartTahrirProcessor::curveSize> curve{};
};
class SmartTahrirEditor final : public juce::AudioProcessorEditor {
public:
 explicit SmartTahrirEditor(SmartTahrirProcessor&);
 ~SmartTahrirEditor() override=default;
 void paint(juce::Graphics&) override; void resized() override;
private:
 void importSample();
 SmartTahrirProcessor& processor;
 juce::Slider enhance,autoTahrir,hold,depth,speed;
 juce::ComboBox style;
 juce::Label title,subtitle,status,enhanceLabel,autoLabel,holdLabel,depthLabel,speedLabel,styleLabel,curveLabel;
 juce::TextButton importButton{"Load Tahrir WAV"},drawButton{"Use Drawn Curve"},resetButton{"Reset"};
 TahrirCurveEditor curveEditor;
 std::unique_ptr<juce::FileChooser> chooser;
 using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
 using ComboAttachment=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
 std::unique_ptr<SliderAttachment> enhanceAttachment,autoAttachment,holdAttachment,depthAttachment,speedAttachment;
 std::unique_ptr<ComboAttachment> styleAttachment;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartTahrirEditor)
};