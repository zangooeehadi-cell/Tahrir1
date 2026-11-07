#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
 using namespace juce;
 std::vector<std::unique_ptr<RangedAudioParameter>> p;
 p.push_back(std::make_unique<AudioParameterFloat>("enhance","Tahrir Boost",NormalisableRange<float>(0.0f,100.0f,0.1f),22.0f));
 p.push_back(std::make_unique<AudioParameterFloat>("autoTahrir","Auto Tahrir",NormalisableRange<float>(0.0f,100.0f,0.1f),78.0f));
 p.push_back(std::make_unique<AudioParameterFloat>("hold","Hold Time",NormalisableRange<float>(120.0f,1200.0f,1.0f),320.0f));
 p.push_back(std::make_unique<AudioParameterFloat>("depth","Tahrir Depth",NormalisableRange<float>(0.0f,100.0f,0.1f),58.0f));
 p.push_back(std::make_unique<AudioParameterFloat>("speed","Tahrir Speed",NormalisableRange<float>(2.5f,9.0f,0.01f),5.2f));
 p.push_back(std::make_unique<AudioParameterChoice>("style","Style",StringArray{"Persian","Arabic","Turkish"},0));
 return {p.begin(),p.end()};
}
SmartTahrirProcessor::SmartTahrirProcessor()
 : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
 .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
 parameters(*this,nullptr,"HADI_PARAMETERS",createParameterLayout()) {}

void SmartTahrirProcessor::prepareToPlay(double sampleRate,int samplesPerBlock)
{
 currentSampleRate = sampleRate > 1000.0 ? sampleRate : 44100.0;
 pitchWindowSamples = juce::jmax(256,(int)std::round(currentSampleRate*0.035));
 baseDelaySamples = juce::jmax(64,(int)std::round(currentSampleRate*0.020));
 setLatencySamples(baseDelaySamples);
 delayCapacity = juce::jmax(baseDelaySamples+pitchWindowSamples+samplesPerBlock+256,(int)std::ceil(currentSampleRate*0.12));
 delayMemory.assign((size_t)juce::jmax(1,getTotalNumInputChannels()),std::vector<float>((size_t)delayCapacity,0.0f));
 writePosition=0; phaseA=0.0; phaseB=0.5; offsetA=0.0; offsetB=0.0; tahrirPhase=0.0;
 voicedHoldSamples=0.0; gateEnvelope=0.0f;
 analysisBuffer.assign(2048,0.0f); analysisWrite=0; analysisFill=0; samplesUntilAnalysis=0;
 decimationCounter=0; decimationAccumulator=0.0f; detectedF0=0.0f; previousF0=0.0f; pitchConfidence=0.0f;
}
void SmartTahrirProcessor::releaseResources(){delayMemory.clear();analysisBuffer.clear();}

float SmartTahrirProcessor::readDelay(int channel,float delaySamples) const noexcept
{
 if(channel<0||channel>=(int)delayMemory.size()||delayCapacity<4) return 0.0f;
 const auto& line=delayMemory[(size_t)channel];
 float pos=(float)writePosition-delaySamples;
 while(pos<0.0f) pos+=(float)delayCapacity;
 while(pos>=(float)delayCapacity) pos-=(float)delayCapacity;
 const int a=(int)pos, b=(a+1)%delayCapacity;
 const float frac=pos-(float)a;
 return line[(size_t)a]+frac*(line[(size_t)b]-line[(size_t)a]);
}

void SmartTahrirProcessor::analysePitch() noexcept
{
 constexpr int window=1024;
 if(analysisFill<window){pitchConfidence=0.0f;detectedF0=0.0f;return;}
 const int analysisRate=(int)std::round(currentSampleRate*0.5);
 const int minLag=juce::jmax(12,analysisRate/1000);
 const int maxLag=juce::jmin(420,analysisRate/70);
 const int start=(analysisWrite+(int)analysisBuffer.size()-window)%(int)analysisBuffer.size();
 float best=0.0f; int bestLag=0;
 for(int lag=minLag;lag<=maxLag;++lag)
 {
  double cross=0.0,eA=0.0,eB=0.0;
  for(int n=lag;n<window;++n)
  {
   const float a=analysisBuffer[(size_t)((start+n)%(int)analysisBuffer.size())];
   const float b=analysisBuffer[(size_t)((start+n-lag)%(int)analysisBuffer.size())];
   cross+=(double)a*b; eA+=(double)a*a; eB+=(double)b*b;
  }
  const float corr=(float)(cross/(std::sqrt(eA*eB)+1.0e-12));
  if(corr>best){best=corr;bestLag=lag;}
 }
 pitchConfidence=best;
 const float f0=bestLag>0?(float)analysisRate/(float)bestLag:0.0f;
 if(best>0.42f&&f0>=65.0f&&f0<=1000.0f)
 {
  if(previousF0>0.0f)
  {
   const float ratio=f0/previousF0;
   detectedF0=(ratio>0.72f&&ratio<1.39f&&detectedF0>0.0f)?(0.72f*detectedF0+0.28f*f0):f0;
  } else detectedF0=f0;
  previousF0=f0;
 }
 else {pitchConfidence*=0.75f;detectedF0=0.0f;previousF0=0.0f;}
}

float SmartTahrirProcessor::makeTahrirCurve(double phase,int style) const noexcept
{
 phase-=std::floor(phase);
 if(style==1)
  return 0.72f*(float)std::sin(juce::MathConstants<double>::twoPi*phase)
       +0.23f*(float)std::sin(juce::MathConstants<double>::twoPi*2.0*phase+0.55);
 if(style==2)
  return 0.58f*(float)std::sin(juce::MathConstants<double>::twoPi*phase)
       +0.30f*(float)std::sin(juce::MathConstants<double>::twoPi*3.0*phase+0.9);
 struct Knot{float x,y;};
 constexpr Knot k[]={{0.00f,0.00f},{0.11f,0.95f},{0.27f,0.72f},{0.43f,-0.82f},
                     {0.59f,-0.35f},{0.73f,0.58f},{0.86f,0.18f},{1.00f,0.00f}};
 const float p=(float)phase;
 for(int i=0;i<7;++i) if(p>=k[i].x&&p<=k[i+1].x)
 {
  const float t=(p-k[i].x)/(k[i+1].x-k[i].x), eased=t*t*(3.0f-2.0f*t);
  return k[i].y+(k[i+1].y-k[i].y)*eased;
 }
 return 0.0f;
}

void SmartTahrirProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&)
{
 juce::ScopedNoDenormals noDenormals;
 const int channels=juce::jmin(buffer.getNumChannels(),(int)delayMemory.size()), samples=buffer.getNumSamples();
 if(channels<=0||delayCapacity<=0||samples<=0)return;
 const float autoAmount=parameters.getRawParameterValue("autoTahrir")->load();
 const float boost=parameters.getRawParameterValue("enhance")->load();
 const float holdMs=parameters.getRawParameterValue("hold")->load();
 const float depth=parameters.getRawParameterValue("depth")->load();
 const float speed=parameters.getRawParameterValue("speed")->load();
 const int style=(int)parameters.getRawParameterValue("style")->load();

 double energy=0.0; int count=0;
 for(int ch=0;ch<channels;++ch){const float* in=buffer.getReadPointer(ch);for(int i=0;i<samples;++i){energy+=(double)in[i]*in[i];++count;}}
 const float rms=count>0?(float)std::sqrt(energy/(double)count):0.0f;

 const float* left=buffer.getReadPointer(0);
 const float* right=channels>1?buffer.getReadPointer(1):left;
 for(int i=0;i<samples;++i)
 {
  decimationAccumulator+=0.5f*(left[i]+right[i]);
  if(++decimationCounter>=2)
  {
   analysisBuffer[(size_t)analysisWrite]=decimationAccumulator*0.5f;
   analysisWrite=(analysisWrite+1)%(int)analysisBuffer.size();
   analysisFill=juce::jmin(analysisFill+1,(int)analysisBuffer.size());
   decimationCounter=0;decimationAccumulator=0.0f;
   if(++samplesUntilAnalysis>=1024){samplesUntilAnalysis=0;analysePitch();}
  }
 }
 const bool voiced=rms>0.0045f&&pitchConfidence>0.42f&&detectedF0>=65.0f&&detectedF0<=1000.0f;
 if(voiced)voicedHoldSamples+=samples;else voicedHoldSamples=0.0;
 const double requiredHold=currentSampleRate*((double)holdMs/1000.0);
 const float target=(autoAmount>0.1f&&voiced&&voicedHoldSamples>=requiredHold)?1.0f:0.0f;
 const float attack=1.0f-std::exp(-1.0f/(float)(currentSampleRate*0.035));
 const float release=1.0f-std::exp(-1.0f/(float)(currentSampleRate*0.16));
 const float depthCents=juce::jlimit(0.0f,190.0f,depth*1.35f+boost*0.35f);
 const double phaseInc=(double)speed/currentSampleRate, window=(double)pitchWindowSamples;

 for(int i=0;i<samples;++i)
 {
  gateEnvelope+=(target-gateEnvelope)*(target>gateEnvelope?attack:release);
  const float contour=makeTahrirCurve(tahrirPhase,style);
  const float cents=contour*depthCents*gateEnvelope;
  const double ratio=std::pow(2.0,(double)cents/1200.0), slope=1.0-ratio;
  if(i==0&&phaseA==0.0&&phaseB==0.5&&offsetA==0.0&&offsetB==0.0)offsetB=0.5*window*slope;
  phaseA+=1.0/window;phaseB+=1.0/window;offsetA+=slope;offsetB+=slope;
  if(phaseA>=1.0){phaseA-=1.0;offsetA=0.0;}
  if(phaseB>=1.0){phaseB-=1.0;offsetB=0.0;}
  const float wa=(float)(0.5-0.5*std::cos(juce::MathConstants<double>::twoPi*phaseA));
  const float wb=1.0f-wa;
  const float da=(float)baseDelaySamples+(float)offsetA, db=(float)baseDelaySamples+(float)offsetB;
  const float wet=gateEnvelope*(autoAmount/100.0f);
  for(int ch=0;ch<channels;++ch)
  {
   auto* data=buffer.getWritePointer(ch);
   const float input=data[i];
   delayMemory[(size_t)ch][(size_t)writePosition]=input;
   const float dry=readDelay(ch,(float)baseDelaySamples);
   const float shifted=readDelay(ch,da)*wa+readDelay(ch,db)*wb;
   data[i]=dry+(shifted-dry)*wet;
  }
  writePosition=(writePosition+1)%delayCapacity;
  tahrirPhase+=phaseInc;if(tahrirPhase>=1.0)tahrirPhase-=std::floor(tahrirPhase);
 }
 for(int ch=channels;ch<buffer.getNumChannels();++ch)buffer.clear(ch,0,samples);
}

juce::AudioProcessorEditor* SmartTahrirProcessor::createEditor(){return new SmartTahrirEditor(*this);}
void SmartTahrirProcessor::getStateInformation(juce::MemoryBlock& destData)
{
 auto state=parameters.copyState();std::unique_ptr<juce::XmlElement> xml(state.createXml());copyXmlToBinary(*xml,destData);
}
void SmartTahrirProcessor::setStateInformation(const void* data,int sizeInBytes)
{
 std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,sizeInBytes));
 if(xml!=nullptr&&xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SmartTahrirProcessor();}
