#include "PluginEditor.h"
#include <algorithm>
TahrirCurveEditor::TahrirCurveEditor(){curve.fill(0.0f);setMouseCursor(juce::MouseCursor::CrosshairCursor);}
void TahrirCurveEditor::setCurve(const std::array<float,SmartTahrirProcessor::curveSize>& c){curve=c;repaint();}
void TahrirCurveEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff10151d));auto r=getLocalBounds().toFloat().reduced(2);
 g.setColour(juce::Colour(0xff3b4656));g.drawRoundedRectangle(r,7,1);
 g.setColour(juce::Colour(0xff354050));g.drawHorizontalLine((int)r.getCentreY(),r.getX(),r.getRight());
 for(int i=1;i<4;++i)g.drawVerticalLine((int)(r.getX()+r.getWidth()*i/4),r.getY(),r.getBottom());
 juce::Path p;for(int i=0;i<SmartTahrirProcessor::curveSize;++i){const float x=r.getX()+r.getWidth()*(float)i/(SmartTahrirProcessor::curveSize-1);
  const float y=r.getCentreY()-curve[(size_t)i]*(r.getHeight()*0.43f);if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);}
 g.setColour(juce::Colour(0xfff1b94e));g.strokePath(p,juce::PathStrokeType(2.4f));
 g.setColour(juce::Colour(0xffc7d0dd));g.setFont(10);g.drawText("+pitch",4,2,48,14,juce::Justification::left);g.drawText("-pitch",4,getHeight()-16,48,14,juce::Justification::left);
}
void TahrirCurveEditor::editAt(juce::Point<float> pos){
 auto r=getLocalBounds().toFloat().reduced(3);const float x=juce::jlimit(0.0f,1.0f,(pos.x-r.getX())/r.getWidth());
 const float y=juce::jlimit(-1.0f,1.0f,(r.getCentreY()-pos.y)/(r.getHeight()*0.43f));
 const int idx=juce::jlimit(0,SmartTahrirProcessor::curveSize-1,(int)std::round(x*(SmartTahrirProcessor::curveSize-1)));
 curve[(size_t)idx]=y;
 if(idx>0&&idx<SmartTahrirProcessor::curveSize-1){curve[(size_t)(idx-1)]=0.5f*(curve[(size_t)(idx-1)]+y);curve[(size_t)(idx+1)]=0.5f*(curve[(size_t)(idx+1)]+y);}
 repaint();if(onCurveChanged)onCurveChanged(curve);
}
void TahrirCurveEditor::mouseDown(const juce::MouseEvent&e){editAt(e.position);}
void TahrirCurveEditor::mouseDrag(const juce::MouseEvent&e){editAt(e.position);}
void SmartTahrirEditor::importSample(){
 chooser=std::make_unique<juce::FileChooser>("Choose a short WAV recording of one clean tahrir",juce::File{}, "*.wav");
 auto safe=juce::Component::SafePointer<SmartTahrirEditor>(this);
 chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& fc){
  if(safe==nullptr)return;const auto file=fc.getResult();if(!file.existsAsFile())return;
  juce::AudioFormatManager fm;fm.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
  if(!reader){safe->status.setText("Could not read this audio file",juce::dontSendNotification);return;}
  const auto frames=(int)juce::jmin<juce::int64>(reader->lengthInSamples,(juce::int64)(reader->sampleRate*12.0));
  if(frames<2048){safe->status.setText("Sample is too short; use a clear 0.3-5 sec WAV",juce::dontSendNotification);return;}
  juce::AudioBuffer<float> audio((int)reader->numChannels,frames);reader->read(&audio,0,frames,0,true,true);
  std::vector<float> mono((size_t)frames,0.0f);for(int i=0;i<frames;++i){for(int ch=0;ch<audio.getNumChannels();++ch)mono[(size_t)i]+=audio.getSample(ch,i)/(float)audio.getNumChannels();}
  constexpr int N=1024, count=SmartTahrirProcessor::curveSize;std::array<float,count> f0{};std::array<float,count> contour{};
  const double sr=reader->sampleRate;std::vector<float> window((size_t)N);
  for(int k=0;k<count;++k){const int center=(int)((double)k/(count-1)*(frames-1));const int start=juce::jlimit(0,juce::jmax(0,frames-N),center-N/2);
   for(int n=0;n<N;++n)window[(size_t)n]=mono[(size_t)(start+n)];
   const int rate=(int)sr,minLag=juce::jmax(20,rate/1000),maxLag=juce::jmin(N/2-2,rate/65);float best=0;int bestLag=0;
   for(int lag=minLag;lag<=maxLag;++lag){double cross=0,e1=0,e2=0;for(int n=lag;n<N;++n){const float a=window[(size_t)n],b=window[(size_t)(n-lag)];cross+=(double)a*b;e1+=(double)a*a;e2+=(double)b*b;}
    const float c=(float)(cross/(std::sqrt(e1*e2)+1.0e-12));if(c>best){best=c;bestLag=lag;}}
   f0[(size_t)k]=(best>0.40f&&bestLag>0)?(float)sr/(float)bestLag:0.0f;
  }
  std::vector<float> valid;for(float v:f0)if(v>65&&v<1000)valid.push_back(v);
  if(valid.size()<count/4){safe->status.setText("Could not track pitch: use a clean solo voice sample",juce::dontSendNotification);return;}
  std::sort(valid.begin(),valid.end());const float median=valid[valid.size()/2];float maxAbs=0;
  for(int i=0;i<count;++i){const float v=f0[(size_t)i];contour[(size_t)i]=(v>65&&v<1000)?12.0f*std::log2(v/median):0.0f;maxAbs=juce::jmax(maxAbs,std::abs(contour[(size_t)i]));}
  if(maxAbs<0.15f){safe->status.setText("Sample has almost no pitch movement; choose a clear tahrir",juce::dontSendNotification);return;}
  for(auto& v:contour)v=juce::jlimit(-1.0f,1.0f,v/(maxAbs*0.92f));
  safe->curveEditor.setCurve(contour);safe->processor.setCustomCurve(contour);
  safe->status.setText("Sample analysed: pitch contour loaded from "+file.getFileName(),juce::dontSendNotification);
 });
}
SmartTahrirEditor::SmartTahrirEditor(SmartTahrirProcessor&p):AudioProcessorEditor(&p),processor(p){
 setSize(760,650);title.setText("HADI",juce::dontSendNotification);title.setJustificationType(juce::Justification::centred);
 title.setFont(juce::Font(34,juce::Font::bold));title.setColour(juce::Label::textColourId,juce::Colour(0xfff4d58d));addAndMakeVisible(title);
 subtitle.setText("VOCAL TAHRIR ENGINE | SHAPE YOUR OWN ORNAMENT",juce::dontSendNotification);subtitle.setJustificationType(juce::Justification::centred);
 subtitle.setColour(juce::Label::textColourId,juce::Colour(0xffaab3c2));addAndMakeVisible(subtitle);
 auto setup=[this](juce::Slider&s,juce::Label&l,const juce::String&name){l.setText(name,juce::dontSendNotification);l.setJustificationType(juce::Justification::centred);l.setColour(juce::Label::textColourId,juce::Colour(0xffe8edf5));addAndMakeVisible(l);
  s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,76,20);s.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffd9a441));
  s.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xfff4f4f4));s.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff282d36));addAndMakeVisible(s);};
 setup(enhance,enhanceLabel,"Tahrir Boost");setup(autoTahrir,autoLabel,"Auto Tahrir");setup(hold,holdLabel,"Hold (ms)");setup(depth,depthLabel,"Pitch Depth");setup(speed,speedLabel,"Speed");
 styleLabel.setText("Style",juce::dontSendNotification);styleLabel.setJustificationType(juce::Justification::centred);styleLabel.setColour(juce::Label::textColourId,juce::Colour(0xffe8edf5));addAndMakeVisible(styleLabel);
 style.addItem("Persian",1);style.addItem("Arabic",2);style.addItem("Turkish",3);style.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff282d36));style.setColour(juce::ComboBox::textColourId,juce::Colour(0xfff4f4f4));addAndMakeVisible(style);
 curveLabel.setText("Tahrir shape — drag to draw pitch movement",juce::dontSendNotification);curveLabel.setColour(juce::Label::textColourId,juce::Colour(0xffe8edf5));addAndMakeVisible(curveLabel);
 addAndMakeVisible(curveEditor);curveEditor.onCurveChanged=[this](const auto& c){processor.setCustomCurve(c);status.setText("Custom drawn curve active",juce::dontSendNotification);};
 importButton.onClick=[this]{importSample();};addAndMakeVisible(importButton);
 drawButton.onClick=[this]{processor.setCustomCurveEnabled(true);status.setText("Drawn curve active",juce::dontSendNotification);};addAndMakeVisible(drawButton);
 resetButton.onClick=[this]{processor.setCustomCurveEnabled(false);status.setText("Built-in style active",juce::dontSendNotification);};addAndMakeVisible(resetButton);
 status.setText("Load a clean WAV tahrir or draw your own curve below",juce::dontSendNotification);status.setJustificationType(juce::Justification::centred);
 status.setFont(juce::Font(11));status.setColour(juce::Label::textColourId,juce::Colour(0xff9aa5b5));addAndMakeVisible(status);
 enhanceAttachment=std::make_unique<SliderAttachment>(processor.parameters,"enhance",enhance);autoAttachment=std::make_unique<SliderAttachment>(processor.parameters,"autoTahrir",autoTahrir);
 holdAttachment=std::make_unique<SliderAttachment>(processor.parameters,"hold",hold);depthAttachment=std::make_unique<SliderAttachment>(processor.parameters,"depth",depth);
 speedAttachment=std::make_unique<SliderAttachment>(processor.parameters,"speed",speed);styleAttachment=std::make_unique<ComboAttachment>(processor.parameters,"style",style);
}
void SmartTahrirEditor::paint(juce::Graphics&g){g.fillAll(juce::Colour(0xff171a20));g.setColour(juce::Colour(0xff343b48));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8),14,1.5f);g.setColour(juce::Colour(0xff252b35));g.fillRoundedRectangle(juce::Rectangle<float>(18,95,(float)getWidth()-36,270),12);}
void SmartTahrirEditor::resized(){
 auto a=getLocalBounds().reduced(24);title.setBounds(a.removeFromTop(44));subtitle.setBounds(a.removeFromTop(22));a.removeFromTop(10);
 const int w=a.getWidth()/5;auto controls=a.removeFromTop(165);juce::Slider*s[]={&enhance,&autoTahrir,&hold,&depth,&speed};juce::Label*l[]={&enhanceLabel,&autoLabel,&holdLabel,&depthLabel,&speedLabel};
 for(int i=0;i<5;++i){auto cell=controls.removeFromLeft(w);l[i]->setBounds(cell.removeFromTop(27));s[i]->setBounds(cell.reduced(3,0));}
 auto st=a.removeFromTop(32);styleLabel.setBounds(st.removeFromLeft(80));style.setBounds(st.removeFromLeft(180).reduced(2));
 a.removeFromTop(8);curveLabel.setBounds(a.removeFromTop(22));curveEditor.setBounds(a.removeFromTop(150));
 auto buttons=a.removeFromTop(34);importButton.setBounds(buttons.removeFromLeft(170).reduced(2));drawButton.setBounds(buttons.removeFromLeft(160).reduced(2));resetButton.setBounds(buttons.removeFromLeft(90).reduced(2));
 status.setBounds(a.removeFromTop(34));
}
