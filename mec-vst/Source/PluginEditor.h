/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

class HostedPluginEditor;
class PluginHeader;


//==============================================================================
/**
*/
class MecAudioProcessorEditor  : public AudioProcessorEditor
{
public:
    MecAudioProcessorEditor (MecAudioProcessor&);
    ~MecAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void childBoundsChanged (Component*) override;
    
    // called when hosted editor is closed, so we no longer own it
    void closeHostedEditor();
    MecAudioProcessor& getMecProcessor() { return processor_;}
    void removePlugin();
    void changePlugin();
    
private:
    AudioProcessorEditor* getEditorFor(AudioProcessorGraph::Node* node,bool useGenericView);
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MecAudioProcessor& processor_;
    ScopedPointer<AudioProcessorEditor> ui_;
    ScopedPointer<HostedPluginEditor> hostedEditor_;

    ScopedPointer<PluginHeader> headerComp_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MecAudioProcessorEditor)
};


#endif  // PLUGINEDITOR_H_INCLUDED
