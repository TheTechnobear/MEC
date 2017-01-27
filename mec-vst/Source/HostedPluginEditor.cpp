#include "HostedPluginEditor.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"


HostedPluginEditor::HostedPluginEditor (MecAudioProcessor& p,
                                        MecAudioProcessorEditor* editor,
                                        AudioProcessorEditor* ui,
                                        bool isGeneric) :
    DocumentWindow (ui->getName(),Colours::lightblue,
                    DocumentWindow::minimiseButton | DocumentWindow::closeButton),
    processor_(p),
    editor_(editor),
    isGeneric_(isGeneric) {
        
    setUsingNativeTitleBar(true);
    setBounds (ui->getBounds());
    
    setContentOwned (ui, true);
    if(processor_.getPlugInNode()) {
        setTopLeftPosition (processor_.getPlugInNode()->properties.getWithDefault ("x", 100),
                            processor_.getPlugInNode()->properties.getWithDefault ("y", 100));
    } else {
        setTopLeftPosition (100,100);
    }
  
    setVisible (true);
    setAlwaysOnTop(true);
}

HostedPluginEditor::~HostedPluginEditor()
{
    setVisible(false);
    editor_->closeHostedEditor();
    clearContentComponent();
}


void HostedPluginEditor::moved()
{
    if(processor_.getPlugInNode()) {
        processor_.getPlugInNode()->properties.set ("x", getX());
        processor_.getPlugInNode()->properties.set ("y", getY());
    }
}


void HostedPluginEditor::closeButtonPressed()
{
    delete this;
}

