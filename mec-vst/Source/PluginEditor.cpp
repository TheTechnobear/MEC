/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "HostedPluginEditor.h"
#include "PluginHeader.h"

const int PLUGINHEADER_HEIGHT = 100;

//==============================================================================
MecAudioProcessorEditor::MecAudioProcessorEditor (MecAudioProcessor& p)
    : AudioProcessorEditor (&p), processor_ (p), ui_(nullptr), hostedEditor_(nullptr)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    
    hostedEditor_ = nullptr;
    ui_ = nullptr;
    headerComp_ = new PluginHeader();
    headerComp_ ->setOwner(this);
    addAndMakeVisible(headerComp_.get());
 
    changePlugin();
}

void MecAudioProcessorEditor::removePlugin()
{
    if(ui_) {
        removeChildComponent(ui_);
        ui_ = nullptr;
    }
}

void MecAudioProcessorEditor::changePlugin()
{
    removePlugin();
    setSize(300,600);
    
    AudioProcessorGraph::Node::Ptr f= processor_.getPlugInNode();
    if(f!=nullptr) {
        bool popoutEditor = false;
        if(popoutEditor) {
            setBounds(headerComp_->getBounds());
            // popout hosted plugin into seperate window
            bool generic = false;
            AudioProcessorEditor* ui = getEditorFor(f, generic);
            hostedEditor_ = new HostedPluginEditor(processor_, this, ui, generic);
            hostedEditor_->toFront (true);
        } else {
            // use inplace editor
            ui_ = getEditorFor(f,false);
            if(ui_) {
                setBounds(0,0, ui_->getWidth(), ui_->getHeight()+ PLUGINHEADER_HEIGHT);
                ui_->setTopLeftPosition(0, PLUGINHEADER_HEIGHT);
                addAndMakeVisible(ui_.get());
            }
        }
    }
}


MecAudioProcessorEditor::~MecAudioProcessorEditor()
{
    processor_.closePluginList();
    ui_= nullptr;
    hostedEditor_= nullptr;
}

//==============================================================================
void MecAudioProcessorEditor::paint (Graphics& g)
{
}

void MecAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

void MecAudioProcessorEditor::childBoundsChanged (Component*)  {
    if(ui_) {
        setBounds(0,0, ui_->getWidth(), ui_->getHeight()+PLUGINHEADER_HEIGHT);
    }
}

void MecAudioProcessorEditor::closeHostedEditor(){ hostedEditor_= nullptr;}


AudioProcessorEditor* MecAudioProcessorEditor::getEditorFor (
    AudioProcessorGraph::Node* node,
    bool useGenericView)
{
    
    AudioProcessorEditor* ui = nullptr;
    
    if (! useGenericView)
    {
        ui = node->getProcessor()->createEditorIfNeeded();
        
        if (ui == nullptr)
            useGenericView = true;
    }
    
    if (useGenericView)
        ui = new GenericAudioProcessorEditor (node->getProcessor());
    
    return ui;
}


