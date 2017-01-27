#ifndef HOSTED_PLUGIN_EDITOR_H
#define HOSTED_PLUGIN_EDITOR_H

#include "../JuceLibraryCode/JuceHeader.h"


class MecAudioProcessor;
class MecAudioProcessorEditor;


class HostedPluginEditor  : public DocumentWindow
{
public:
    HostedPluginEditor (MecAudioProcessor&,
                        MecAudioProcessorEditor*,
                        AudioProcessorEditor* ,
                        bool);
    ~HostedPluginEditor();
    
    void moved();
    void closeButtonPressed();
    
private:
    MecAudioProcessor& processor_;
    MecAudioProcessorEditor* editor_;
    bool isGeneric_;
};


#endif  // HOSTED_PLUGIN_EDITOR_H

