/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include <memory>
class MecApi;

//==============================================================================
/**
*/
class MecAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    MecAudioProcessor();
    ~MecAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    
    void createPlugin(double sampleRate, int samplesPerBlock);
    void instanceCreated(AudioPluginInstance* instance,
                         double sampleRate,
                         int samplesPerBlock,
                         const String& error);
    AudioProcessorGraph::Node::Ptr getPlugInNode();
    
    void initMec();
    
    static int bipolar14bit(float v) {return ((v * 0x2000) + 0x2000);}
    static int bipolar7bit(float v) {return ((v / 2) + 0.5)  * 127; }
    static int unipolar7bit(float v) {return v * 127;}

    void openPluginList();
    void closePluginList();
    void addPluginsToMenu (PopupMenu& m) const;
    void chosenPluginMenu(int menuID);
    void createHostedPlugin(PluginDescription& desc, double sampleRate, int samplesPerBlock);
    PluginDescription& getHostedPlugDesc() {return hostedPlugDesc_;}
    void setMidiOutput(bool b);
    
    
private:
    AudioPluginFormatManager    formatManager_;
    AudioProcessorGraph         graph_;
    AudioProcessorGraph::Node*  node_;
    std::unique_ptr<MecApi>     mecapi_;
    MidiBuffer                  mecMidiQueue_;
    double                      sampleRate_;
    int                         samplesPerBlock_;
    bool                        midiOutput_;
    
    PluginDescription           hostedPlugDesc_;
    bool                        hostedPlugDescLoad_;
    MemoryBlock                 hostedPlugLoadData_;
    
    String                      mecPrefFile_;
    String                      crashedPlugsin_;
    
    
    KnownPluginList                 knownPluginList;
    KnownPluginList::SortMethod     pluginSortMethod;
    class PluginListWindow;
    ScopedPointer<PluginListWindow> pluginListWindow;
    ScopedPointer<PropertiesFile>   propertiesFile;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MecAudioProcessor)
};


#endif  // PLUGINPROCESSOR_H_INCLUDED
