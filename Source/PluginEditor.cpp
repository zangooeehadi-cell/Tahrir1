#include "PluginEditor.h"
SmartTahrirEditor::SmartTahrirEditor(SmartTahrirProcessor& p):AudioProcessorEditor(&p),processor(p){
 setSize(720,480);title.setText("HADI",juce::dontSendNotification);title.setJustificationType(juce::Justification::centred);
 title.setFont(juce::Font(34,juce::Font::bold));title.setColour(juce::Label::textColourId,juce::Colour(0xfff4d58d));addAndMakeVisible(title);
 subtitle.setText("INTELLIGENT VOCAL TAHRIR ENGINE",juce::dontSendNotification);subtitle.setJustificationType(juce::Justification::centred);
 subtitle.setFont(juce::Font(12));subtitle.setColour(juce::Label::textColourId,juce::Colour(0xffaab3c2));addAndMakeVisible(subtitle);
 auto setup=[this](juce::Slider&s,juce::Label&l,const juce::String&name){l.setText(name,juce::dontSendNotification);l.setJustificationType(juce::Justification::centred);
  l.setColour(juce::Label::textColourId,juce::Colour(0xffe8edf5));addAndMakeVisible(l);s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,76,22);s.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffd9a441));
  s.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xfff4f4f4));s.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff282d36));addAndMakeVisible(s);};
 setup(enhance,enhanceLabel,"Tahrir Boost");setup(autoTahrir,autoLabel,"Auto Tahrir");setup(hold,holdLabel,"Hold (ms)");setup(depth,depthLabel,"Pitch Depth");setup(speed,speedLabel,"Speed");
 styleLabel.setText("Style",juce::dontSendNotification);styleLabel.setJustificationType(juce::Justification::centred);styleLabel.setColour(juce::Label::textColourId,juce::Colour(0xffe8edf5));addAndMakeVisible(styleLabel);
 style.addItem("Persian",1);style.addItem("Arabic",2);style.addItem("Turkish",3);style.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff282d36));style.setColour(juce::ComboBox::textColourId,juce::Colour(0xfff4f4f4));addAndMakeVisible(style);
 status.setText("SIGNALSMITH SPECTRAL PITCH ENGINE | FORMANT COMPENSATION | AUTO VOICED GATE",juce::dontSendNotification);
 status.setJustificationType(juce::Justification::centred);status.setFont(juce::Font(10));status.setColour(juce::Label::textColourId,juce::Colour(0xff9aa5b5));addAndMakeVisible(status);
 enhanceAttachment=std::make_unique<SliderAttachment>(processor.parameters,"enhance",enhance);autoAttachment=std::make_unique<SliderAttachment>(processor.parameters,"autoTahrir",autoTahrir);
 holdAttachment=std::make_unique<SliderAttachment>(processor.parameters,"hold",hold);depthAttachment=std::make_unique<SliderAttachment>(processor.parameters,"depth",depth);
 speedAttachment=std::make_unique<SliderAttachment>(processor.parameters,"speed",speed);styleAttachment=std::make_unique<ComboAttachment>(processor.parameters,"style",style);
}
void SmartTahrirEditor::paint(juce::Graphics&g){g.fillAll(juce::Colour(0xff171a20));g.setColour(juce::Colour(0xff343b48));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8),14,1.5f);g.setColour(juce::Colour(0xff252b35));g.fillRoundedRectangle(juce::Rectangle<float>(22,105,(float)getWidth()-44,300),12);}
void SmartTahrirEditor::resized(){auto a=getLocalBounds().reduced(24);title.setBounds(a.removeFromTop(48));subtitle.setBounds(a.removeFromTop(24));a.removeFromTop(18);
 const int w=a.getWidth()/5;auto controls=a.removeFromTop(260);juce::Slider*s[]={&enhance,&autoTahrir,&hold,&depth,&speed};juce::Label*l[]={&enhanceLabel,&autoLabel,&holdLabel,&depthLabel,&speedLabel};
 for(int i=0;i<5;++i){auto cell=controls.removeFromLeft(w);l[i]->setBounds(cell.removeFromTop(32));s[i]->setBounds(cell.reduced(5,0));}
 auto sa=a.removeFromTop(55);styleLabel.setBounds(sa.removeFromLeft(110));style.setBounds(sa.removeFromLeft(190).reduced(4));status.setBounds(a.removeFromTop(35));}
