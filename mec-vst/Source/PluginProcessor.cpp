/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <processors/mec_midiprocessor.h>


//==============================================================================
MecAudioProcessor::MecAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    node_ = nullptr;
    hostedPlugDescLoad_ = false;
    formatManager_.addDefaultFormats();
    PropertiesFile::Options options;
    options.applicationName = "MEC";
    options.folderName = "MEC";
    options.osxLibrarySubFolder = "Application Support";
    
    propertiesFile = new PropertiesFile(options);
    File appdir =propertiesFile->getFile().getParentDirectory();
    mecPrefFile_ = appdir.getChildFile("mec.json").getFullPathName();
    crashedPlugsin_ = appdir.getChildFile("crashedPlugins").getFullPathName();
    
    ScopedPointer<XmlElement> savedPluginList (propertiesFile->getXmlValue ("pluginList"));
    
    if (savedPluginList != nullptr)
        knownPluginList.recreateFromXml (*savedPluginList);
}

MecAudioProcessor::~MecAudioProcessor()
{
}

//==============================================================================
const String MecAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MecAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MecAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

double MecAudioProcessor::getTailLengthSeconds() const
{
    return node_ !=nullptr ?  node_->getProcessor()->getTailLengthSeconds() : 0.0;
}

int MecAudioProcessor::getNumPrograms()
{
    return node_ !=nullptr ?  node_->getProcessor()->getNumPrograms() : 1;
}

int MecAudioProcessor::getCurrentProgram()
{
    return node_ !=nullptr ?  node_->getProcessor()->getCurrentProgram() : 0;
}

void MecAudioProcessor::setCurrentProgram (int index)
{
    if(node_ !=nullptr) node_->getProcessor()->setCurrentProgram(index);
}

const String MecAudioProcessor::getProgramName (int index)
{
    return node_ !=nullptr ?  node_->getProcessor()->getProgramName(index) : String();
}

void MecAudioProcessor::changeProgramName (int index, const String& newName)
{
    if(node_ !=nullptr) node_->getProcessor()->changeProgramName(index,newName);
}

//==============================================================================
void MecAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // THIS NEEDS CHANGING - TODO
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    sampleRate_ = sampleRate;
    samplesPerBlock_ = samplesPerBlock;
    struct MecMidiProcessor : public mec::MidiProcessor {
        const float PBR = 48.0f;
        MecMidiProcessor(MidiBuffer& midiBuf) :
            mec::MidiProcessor(PBR),
            mecMidiQueue_(midiBuf) {
        }
        
        void  process(mec::MidiProcessor::MidiMsg& m) {
            mecMidiQueue_.addEvent(m.data, m.size, 0);
        }
        MidiBuffer&   mecMidiQueue_;
    };
    
    
    
    if(mecapi_==nullptr) {
        mecapi_.reset(new mec::MecApi(mecPrefFile_.toRawUTF8()));
        mecapi_->subscribe(new MecMidiProcessor(mecMidiQueue_));
        mecapi_->init();
    }

    if(hostedPlugDescLoad_) {
        hostedPlugDescLoad_ = false;
        createHostedPlugin(hostedPlugDesc_, sampleRate_, samplesPerBlock_);
    }
}

void MecAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    if(node_ !=nullptr) node_->getProcessor()->releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MecAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
//    return node_ !=nullptr ?  node_->getProcessor()->isBusesLayoutSupported(layouts) : false;
}
#endif

void MecAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    if(mecapi_!=nullptr) {
        mecapi_->process();
    }
    
    MidiBuffer::Iterator iter(mecMidiQueue_);
    MidiMessage msg;
    int samplePos = 0;
    int firstSample = midiMessages.getFirstEventTime();
    while(iter.getNextEvent(msg, samplePos)) {
        midiMessages.addEvent(msg,firstSample);
    }
    if(node_ !=nullptr) node_->getProcessor()->processBlock(buffer,midiMessages);
    
    if(midiOutput_) {
        midiMessages.addEvents(mecMidiQueue_,0, -1,0);
    }
    mecMidiQueue_.clear();
    
//    // This is the place where you'd normally do the guts of your plugin's
//    // audio processing...
//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    {
//        float* channelData = buffer.getWritePointer (channel);
//
//        // ..do something to the data...
//    }
}

//==============================================================================
bool MecAudioProcessor::hasEditor() const
{
//    return node_ !=nullptr ?  node_->getProcessor()->hasEditor() : true;
    return true;
}

AudioProcessorEditor* MecAudioProcessor::createEditor()
{
    return new MecAudioProcessorEditor (*this);
}

const long MECPLUGSTATESZ = 1024;

//==============================================================================
void MecAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
 
    XmlElement xml ("MecPlugin");
    
    xml.setAttribute("midiOutput",midiOutput_);
    
    // Store the values of all our parameters, using their param ID as the XML attribute
    for (int i = 0; i < getNumParameters(); ++i)
        if (AudioProcessorParameterWithID* p = dynamic_cast<AudioProcessorParameterWithID*> (getParameters().getUnchecked(i)))
            xml.setAttribute (p->paramID, p->getValue());
    
    XmlElement *pHostPlug = hostedPlugDesc_.createXml();
    DBG(pHostPlug->getTagName());
    xml.addChildElement(pHostPlug);
    
    
    copyXmlToBinary (xml, destData);
 
    MemoryOutputStream str(destData,true);
    str.writeRepeatedByte(0, MECPLUGSTATESZ - (long) destData.getSize());
    
    MemoryBlock hostPlug;
    if(node_ !=nullptr) node_->getProcessor()->getStateInformation(hostPlug);
    
    str.write(hostPlug.getData(), (long) hostPlug.getSize());
    
    unsigned long long sum = 0;
    for(int i=0;i<hostPlug.getSize();i++) {
        char* ad =(char*)(hostPlug.getData()) +i;
        sum += *ad;
    }
}

void MecAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    if(sizeInBytes < MECPLUGSTATESZ) {
        DBG("corrupt state info expecting at least "<< MECPLUGSTATESZ << " bytes got " << sizeInBytes);
    }
    
    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, MECPLUGSTATESZ));
    
    if (xmlState != nullptr)
    {
        // make sure that it's actually our type of XML object..
        if (xmlState->hasTagName ("MecPlugin"))
        {
            midiOutput_  = xmlState->getBoolAttribute("midiOutput",false);
            
            // Now reload our parameters..
            for (int i = 0; i < getNumParameters(); ++i)
                if (AudioProcessorParameterWithID* p = dynamic_cast<AudioProcessorParameterWithID*> (getParameters().getUnchecked(i)))
                    p->setValue ((float) xmlState->getDoubleAttribute (p->paramID, p->getValue()));

            // loaded hosted plugin
            XmlElement* hostedDesc = xmlState->getChildByName("PLUGIN");
            if(hostedDesc) {
                hostedPlugDesc_.loadFromXml(*hostedDesc);
                hostedPlugDescLoad_ = true;
                
            } else {
                DBG("unabled to find hosted plug desc");
            }

        }
    }
    
    if( sizeInBytes > MECPLUGSTATESZ) {
        int  sz = sizeInBytes - MECPLUGSTATESZ;
        void* addr =((char*) data) + (size_t) MECPLUGSTATESZ;
        // load the state info once the plugin has loaded
        hostedPlugLoadData_.append(addr, sz);
    }
}


//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MecAudioProcessor();
}



//==============================================================================
// specific to MecAudioProcessor
AudioProcessorGraph::Node::Ptr MecAudioProcessor::getPlugInNode()
{
    if (graph_.getNumNodes() > 0)
    {
        AudioProcessorGraph::Node* node = graph_.getNode(0);
        if (node != nullptr)
        {
            return node;
        }
    }
    return nullptr;
}


void MecAudioProcessor::instanceCreated(AudioPluginInstance* instance,double sampleRate, int samplesPerBlock, const String& error) {
    if (instance != nullptr)
    {
        graph_.clear();
        node_ = graph_.addNode (instance);
    }
    
    if (node_ != nullptr)
    {
        if(getActiveEditor()) {
            ((MecAudioProcessorEditor*) getActiveEditor())->changePlugin();
        }
//        node_->properties.set ("x", 100);
//        node_->properties.set ("y", 100);
        node_->getProcessor()->setStateInformation(hostedPlugLoadData_.getData(),hostedPlugLoadData_.getSize());
        hostedPlugLoadData_.reset();
        node_->getProcessor()->prepareToPlay(sampleRate, samplesPerBlock);
    }
    else
    {
        DBG("MecAudioProcessor::instanceCreated - could not create plugin: " << error);
    }
}

// #define USE_ASYNC_CREATE 1
void MecAudioProcessor::createHostedPlugin(PluginDescription& desc,double sampleRate, int samplesPerBlock)
{
    struct  PlugInstanceCallback : public AudioPluginFormat::InstantiationCompletionCallback {
        PlugInstanceCallback(MecAudioProcessor* p,
                             double sampleRate,
                             int samplesPerBlock)
        :   p_(p),
        sampleRate_(sampleRate),
        samplesPerBlock_(samplesPerBlock)
        {
        }
        virtual void completionCallback (AudioPluginInstance* instance, const String& error) override {
            p_->instanceCreated(instance,sampleRate_,samplesPerBlock_, error);
        }
        MecAudioProcessor* p_;
        double sampleRate_;
        int samplesPerBlock_;
    };
    
    String error;
    if(node_ != nullptr) {
        if(getActiveEditor()) {
            // need to remove the hosted plugin, before switching nodes.
            // to ensure its 'editor' is closed
            ((MecAudioProcessorEditor*) getActiveEditor())->removePlugin();
        }
        node_ = nullptr;
    }
#ifdef USE_ASYNC_CREATE
    // TODO FIX LEAK of CB
    PlugInstanceCallback * cb = new PlugInstanceCallback(this,sampleRate, samplesPerBlock);
    formatManager_.createPluginInstanceAsync(desc, sampleRate, samplesPerBlock, cb);
#else
    AudioPluginInstance* instance =
    formatManager_.createPluginInstance(desc, sampleRate, samplesPerBlock, error);
    instanceCreated(instance, sampleRate, samplesPerBlock, error);
#endif
    
}


//==============================================================================
class MecAudioProcessor::PluginListWindow  : public DocumentWindow
{
public:
    PluginListWindow (MecAudioProcessor& owner, AudioPluginFormatManager& pluginFormatManager)
    : DocumentWindow ("Available Plugins", Colours::white,
                      DocumentWindow::minimiseButton | DocumentWindow::closeButton),
    owner_ (owner)
    {
        const File deadMansPedalFile(owner_.crashedPlugsin_);
        
        setContentOwned (new PluginListComponent (pluginFormatManager,
                                                  owner.knownPluginList,
                                                  deadMansPedalFile,
                                                  owner.propertiesFile, true), true);
        
        setResizable (true, false);
        setResizeLimits (300, 400, 800, 1500);
        setTopLeftPosition (60, 60);
        
//        restoreWindowStateFromString (getAppProperties().getUserSettings()->getValue ("listWindowPos"));
        setVisible (true);
    }
    
    ~PluginListWindow()
    {
//        getAppProperties().getUserSettings()->setValue ("listWindowPos", getWindowStateAsString());
        
        clearContentComponent();
    }
    
    void closeButtonPressed()
    {
        ScopedPointer<XmlElement> savedPluginList (owner_.knownPluginList.createXml());
        owner_.propertiesFile->setValue ("pluginList", savedPluginList);
        owner_.propertiesFile->saveIfNeeded();
        owner_.pluginListWindow = nullptr;
    }
    
private:
    MecAudioProcessor& owner_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListWindow)
};

void MecAudioProcessor::closePluginList() {
    pluginListWindow = nullptr;
}

void MecAudioProcessor::openPluginList() {
    if(pluginListWindow ==nullptr) {
        pluginListWindow = new PluginListWindow(*this, formatManager_);
    }
}
void MecAudioProcessor::addPluginsToMenu (PopupMenu& m) const
{
    knownPluginList.addToMenu(m,KnownPluginList::defaultOrder);
}

void MecAudioProcessor::chosenPluginMenu(int menuID) {
    PluginDescription *desc =knownPluginList.getType (knownPluginList.getIndexChosenByMenu (menuID));
    if(desc) {
        hostedPlugDesc_ = *desc;
        createHostedPlugin(*desc, sampleRate_, samplesPerBlock_);
    }
}

void MecAudioProcessor::setMidiOutput(bool b)
{
    midiOutput_ = b;
}

