/*
  ==============================================================================

    PluginSelector.h
    Created: 26 Jan 2017 10:31:36pm
    Author:  TheTechnobear

  ==============================================================================
*/

#ifndef PLUGINSELECTOR_H_INCLUDED
#define PLUGINSELECTOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class MecAudioProcessorEditor;

class PluginSelector : public Label
{
public:
    PluginSelector (const String& componentName = String(),const String& labelText = String());
 
    void mouseDown (const MouseEvent& e);
    void setOwner(MecAudioProcessorEditor*);
    
private:
    MecAudioProcessorEditor *owner_;
};



#endif  // PLUGINSELECTOR_H_INCLUDED
