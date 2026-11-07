#include "PluginProcessor.h"
#include "PluginEditor.h"
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(){
 using namespace juce; std::vector<std::unique_ptr<RangedAudioParameter>> p;
 p.push_back(std::make_unique<AudioParameterFloat>("enhance","Tahrir Boost",NormalisableRange<float>(0,100,0.1f),22));
 p.push_back(std::make_unique<AudioParameterFloat>("autoTahrir","Auto Tahrir",NormalisableRange<float>(0,100,0.1f),78));
 p.push_back(std::make_unique<AudioParameterFloat>("hold","Hold Time",NormalisableRange<float>(50,1000,1),140));
 p.push_back(std::make_unique<AudioParameterFloat>("depth","Tahrir Depth",NormalisableRange<float>(0,100,0.1f),62));
 p.push_back(std::make_unique<AudioParameterFloat>("speed","Tahrir Speed",NormalisableRange<float>(4,14,0.01f),8.0f));
 p.push_back(std::make_unique<AudioParameterChoice>("style","Style",StringArray{"Persian","Arabic","Turkish"},0));
 return {p.begin(),p.end()};
}
SmartTahrirProcessor::SmartTahrirProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,nullptr,"HADI_PARAMETERS",createParameterLayout()){
 for(auto& v:customCurve)v.store(0.0f);
}
void SmartTahrirProcessor::setCustomCurve(const std::array<float,curveSize>& curve,float durationSeconds)noexcept{
 for(int i=0;i<curveSize;++i)customCurve[(size_t)i].store(juce::jlimit(-1.0f,1.0f,curve[(size_t)i]),std::memory_order_relaxed);
 customCurveDurationSeconds.store(juce::jlimit(0.12f,12.0f,durationSeconds),std::memory_order_relaxed);
 customCurveEnabled.store(true,std::memory_order_relaxed);
}
void SmartTahrirProcessor::prepareToPlay(double sr,int block){
 currentSampleRate=sr>1000?sr:44100; maxBlockSize=juce::jmax(65536,block);
 pitchInput.assign(2,std::vector<float>((size_t)maxBlockSize)); pitchOutput.assign(2,std::vector<float>((size_t)maxBlockSize));
 pitchCurve.assign((size_t)maxBlockSize,0); wetCurve.assign((size_t)maxBlockSize,0); inputPointers.resize(2); outputPointers.resize(2);
 // Shorter analysis/hop settings reduce the response delay for fast ornamentation.
 pitchEngine.configure(2,1024,128,true); pitchEngine.reset(); pitchEngine.setFormantFactor(1.0f,true); pitchEngine.setFormantBase(0.0f);
 latencySamples=pitchEngine.inputLatency()+pitchEngine.outputLatency(); setLatencySamples(latencySamples);
 delayCapacity=latencySamples+maxBlockSize+512; dryDelay.assign(2,std::vector<float>((size_t)delayCapacity,0)); delayWrite=0;
 analysisBuffer.assign(2048,0); analysisWrite=analysisFill=samplesUntilAnalysis=decimationCounter=0;
 decimationAccumulator=detectedF0=previousF0=pitchConfidence=0; voicedHoldSamples=tahrirPhase=0; gateEnvelope=0;
}
void SmartTahrirProcessor::releaseResources(){pitchInput.clear();pitchOutput.clear();dryDelay.clear();analysisBuffer.clear();pitchCurve.clear();wetCurve.clear();inputPointers.clear();outputPointers.clear();}
float SmartTahrirProcessor::readDelay(int ch,float delay)const noexcept{
 if(ch<0||ch>=(int)dryDelay.size()||delayCapacity<4)return 0;
 const auto& line=dryDelay[(size_t)ch]; float pos=(float)delayWrite-delay;
 while(pos<0)pos+=(float)delayCapacity; while(pos>=(float)delayCapacity)pos-=(float)delayCapacity;
 const int a=(int)pos,b=(a+1)%delayCapacity; const float f=pos-a;
 return line[(size_t)a]+f*(line[(size_t)b]-line[(size_t)a]);
}
void SmartTahrirProcessor::analysePitch()noexcept{
 constexpr int win=1024; if(analysisFill<win){pitchConfidence=0;detectedF0=0;return;}
 const int rate=(int)std::round(currentSampleRate*0.5),minLag=juce::jmax(12,rate/1000),maxLag=juce::jmin(420,rate/65);
 const int start=(analysisWrite+(int)analysisBuffer.size()-win)%(int)analysisBuffer.size(); float best=0;int bestLag=0;
 for(int lag=minLag;lag<=maxLag;++lag){double cross=0,e1=0,e2=0;for(int n=lag;n<win;++n){
  const float a=analysisBuffer[(size_t)((start+n)%(int)analysisBuffer.size())],b=analysisBuffer[(size_t)((start+n-lag)%(int)analysisBuffer.size())];
  cross+=(double)a*b;e1+=(double)a*a;e2+=(double)b*b;}
  const float c=(float)(cross/(std::sqrt(e1*e2)+1.0e-12));if(c>best){best=c;bestLag=lag;}}
 pitchConfidence=best;const float f0=bestLag?(float)rate/bestLag:0;
 if(best>0.42f&&f0>=65&&f0<=1000){if(previousF0>0){const float r=f0/previousF0;detectedF0=(r>0.72f&&r<1.39f&&detectedF0>0)?0.72f*detectedF0+0.28f*f0:f0;}else detectedF0=f0;previousF0=f0;}
 else{pitchConfidence*=0.75f;detectedF0=previousF0=0;}
}
float SmartTahrirProcessor::makeTahrirCurve(double phase,int style)const noexcept{
 phase-=std::floor(phase);
 if(style==1)return 0.72f*(float)std::sin(juce::MathConstants<double>::twoPi*phase)+0.23f*(float)std::sin(juce::MathConstants<double>::twoPi*2*phase+0.55);
 if(style==2)return 0.58f*(float)std::sin(juce::MathConstants<double>::twoPi*phase)+0.30f*(float)std::sin(juce::MathConstants<double>::twoPi*3*phase+0.9);
 struct K{float x,y;}; constexpr K k[]={{0,0},{.11f,.95f},{.27f,.72f},{.43f,-.82f},{.59f,-.35f},{.73f,.58f},{.86f,.18f},{1,0}};
 const float p=(float)phase;for(int i=0;i<7;++i)if(p>=k[i].x&&p<=k[i+1].x){const float t=(p-k[i].x)/(k[i+1].x-k[i].x),s=t*t*(3-2*t);return k[i].y+(k[i+1].y-k[i].y)*s;}return 0;
}
void SmartTahrirProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
 juce::ScopedNoDenormals nd;const int nSamples=buffer.getNumSamples(),channels=juce::jmin(2,buffer.getNumChannels());
 if(nSamples<=0||channels<=0||delayCapacity<=0||nSamples>maxBlockSize)return;
 const float amount=parameters.getRawParameterValue("autoTahrir")->load(),boost=parameters.getRawParameterValue("enhance")->load();
 const float hold=parameters.getRawParameterValue("hold")->load(),depth=parameters.getRawParameterValue("depth")->load(),speed=parameters.getRawParameterValue("speed")->load();
 const int style=(int)parameters.getRawParameterValue("style")->load();
 double energy=0;int count=0;for(int c=0;c<channels;++c){const float* p=buffer.getReadPointer(c);for(int i=0;i<nSamples;++i){energy+=(double)p[i]*p[i];++count;}}
 const float rms=count?(float)std::sqrt(energy/count):0;
 for(int i=0;i<nSamples;++i){const float l=buffer.getReadPointer(0)[i],r=channels>1?buffer.getReadPointer(1)[i]:l;
  pitchInput[0][(size_t)i]=l;pitchInput[1][(size_t)i]=r;decimationAccumulator+=0.5f*(l+r);
  if(++decimationCounter>=2){analysisBuffer[(size_t)analysisWrite]=decimationAccumulator*0.5f;analysisWrite=(analysisWrite+1)%(int)analysisBuffer.size();
   analysisFill=juce::jmin(analysisFill+1,(int)analysisBuffer.size());decimationCounter=0;decimationAccumulator=0;
   if(++samplesUntilAnalysis>=512){samplesUntilAnalysis=0;analysePitch();}}
 }
 const bool voiced=rms>0.0035f&&pitchConfidence>0.42f&&detectedF0>=65&&detectedF0<=1000;
 if(voiced)voicedHoldSamples+=nSamples;else voicedHoldSamples=0;
 const double required=currentSampleRate*(hold/1000.0);
 const float atk=1.0f-std::exp(-1.0f/(float)(currentSampleRate*0.008)),rel=1.0f-std::exp(-1.0f/(float)(currentSampleRate*0.075));
 const bool useCustom=customCurveEnabled.load(std::memory_order_relaxed);
 // Auto Tahrir controls intensity, Depth controls maximum excursion.
 // Maximum deviation is capped at 35 cents to protect the sung note.
 const float intensity=juce::jlimit(0.0f,1.0f,amount/100.0f);
 const float depthScale=juce::jlimit(0.0f,1.0f,depth/100.0f);
 const float enhanceScale=1.0f+0.25f*juce::jlimit(0.0f,100.0f,boost)/100.0f;
 const float maxDepthCents=35.0f*intensity*depthScale*enhanceScale;
 const float target=(voiced&&voicedHoldSamples>=required&&maxDepthCents>0.05f)?1.0f:0.0f;
 const double patternHz=useCustom?((double)speed/8.0/(double)juce::jmax(0.12f,customCurveDurationSeconds.load(std::memory_order_relaxed))):(double)speed;
 const double phaseInc=patternHz/currentSampleRate;
 // Center each curve around zero so it cannot pull the sustained note sharp or flat.
 double contourMean=0.0;
 for(int k=0;k<curveSize;++k){
  if(useCustom) contourMean+=customCurve[(size_t)k].load(std::memory_order_relaxed);
  else contourMean+=makeTahrirCurve((double)k/(double)curveSize,style);
 }
 contourMean/=(double)curveSize;
 for(int i=0;i<nSamples;++i){gateEnvelope+=(target-gateEnvelope)*(target>gateEnvelope?atk:rel);
  float shape=makeTahrirCurve(tahrirPhase,style);
  if(useCustom){const float pos=(float)(tahrirPhase*(curveSize-1));const int a=juce::jlimit(0,curveSize-1,(int)pos),b=juce::jmin(curveSize-1,a+1);const float f=pos-a;
   shape=customCurve[(size_t)a].load(std::memory_order_relaxed)*(1-f)+customCurve[(size_t)b].load(std::memory_order_relaxed)*f;}
  shape-=(float)contourMean;
  pitchCurve[(size_t)i]=shape*maxDepthCents*gateEnvelope;
  // Avoid parallel dry/wet pitch mixing: it creates two simultaneous pitches.
  wetCurve[(size_t)i]=gateEnvelope;
  tahrirPhase+=phaseInc;if(tahrirPhase>=1)tahrirPhase-=std::floor(tahrirPhase);}
 constexpr int quantum=32;
 for(int offset=0;offset<nSamples;offset+=quantum){const int len=juce::jmin(quantum,nSamples-offset);double sum=0;
  for(int i=0;i<len;++i)sum+=pitchCurve[(size_t)(offset+i)];
  pitchEngine.setTransposeSemitones((float)(sum/len)/100.0f);
  // Keep formant reference neutral; do not feed the tracked F0 back as a formant setting.
  for(int c=0;c<2;++c){inputPointers[(size_t)c]=pitchInput[(size_t)c].data()+offset;outputPointers[(size_t)c]=pitchOutput[(size_t)c].data()+offset;}
  pitchEngine.process(inputPointers.data(),len,outputPointers.data(),len);
 }
 for(int i=0;i<nSamples;++i){for(int c=0;c<channels;++c){dryDelay[(size_t)c][(size_t)delayWrite]=pitchInput[(size_t)c][(size_t)i];
   const float dry=readDelay(c,(float)latencySamples),wet=juce::jlimit(0.0f,1.0f,wetCurve[(size_t)i]);
   buffer.getWritePointer(c)[i]=dry+(pitchOutput[(size_t)c][(size_t)i]-dry)*wet;}
  delayWrite=(delayWrite+1)%delayCapacity;}
}
juce::AudioProcessorEditor* SmartTahrirProcessor::createEditor(){return new SmartTahrirEditor(*this);}
void SmartTahrirProcessor::getStateInformation(juce::MemoryBlock& d){
 auto s=parameters.copyState();std::unique_ptr<juce::XmlElement>x(s.createXml());
 auto* curve=x->createNewChildElement("CUSTOM_TAHRIR_CURVE");
 curve->setAttribute("enabled",customCurveEnabled.load(std::memory_order_relaxed));
 curve->setAttribute("durationSeconds",(double)customCurveDurationSeconds.load(std::memory_order_relaxed));
 for(int i=0;i<curveSize;++i)curve->setAttribute("p"+juce::String(i),customCurve[(size_t)i].load(std::memory_order_relaxed));
 copyXmlToBinary(*x,d);
}
void SmartTahrirProcessor::setStateInformation(const void* d,int n){
 std::unique_ptr<juce::XmlElement>x(getXmlFromBinary(d,n));
 if(x&&x->hasTagName(parameters.state.getType())){
  parameters.replaceState(juce::ValueTree::fromXml(*x));
  if(auto* curve=x->getChildByName("CUSTOM_TAHRIR_CURVE")){
   for(int i=0;i<curveSize;++i)customCurve[(size_t)i].store((float)curve->getDoubleAttribute("p"+juce::String(i),0.0),std::memory_order_relaxed);
   customCurveDurationSeconds.store((float)curve->getDoubleAttribute("durationSeconds",0.55),std::memory_order_relaxed);
   customCurveEnabled.store(curve->getBoolAttribute("enabled",false),std::memory_order_relaxed);
  }
 }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SmartTahrirProcessor();}
