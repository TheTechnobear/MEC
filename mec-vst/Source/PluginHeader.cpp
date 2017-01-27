/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 4.3.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "PluginHeader.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
#include "PluginSelector.h"
#include "PluginEditor.h"
//[/MiscUserDefs]

//==============================================================================
PluginHeader::PluginHeader ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    owner_ = nullptr;
    //[/Constructor_pre]

    addAndMakeVisible (pluginTxt_ = new PluginSelector ("pluginFile",
                                                        TRANS("select plugin...")));
    pluginTxt_->setFont (Font (17.60f, Font::plain));
    pluginTxt_->setJustificationType (Justification::centredLeft);
    pluginTxt_->setEditable (false, false, false);
    pluginTxt_->setColour (Label::backgroundColourId, Colours::white);
    pluginTxt_->setColour (Label::textColourId, Colours::black);
    pluginTxt_->setColour (Label::outlineColourId, Colour (0x00ffffff));
    pluginTxt_->setColour (TextEditor::textColourId, Colours::black);
    pluginTxt_->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));

    addAndMakeVisible (scanBtn_ = new TextButton ("scan plugins..."));
    scanBtn_->addListener (this);
    scanBtn_->setColour (TextButton::buttonColourId, Colour (0xfff8f8f8));

    addAndMakeVisible (mecBtn_ = new ToggleButton ("mec"));
    mecBtn_->setTooltip (TRANS("Enable MEC"));
    mecBtn_->setButtonText (TRANS("MEC"));
    mecBtn_->addListener (this);
    mecBtn_->setToggleState (true, dontSendNotification);
    mecBtn_->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (midiOutputBtn_ = new ToggleButton ("midi out"));
    midiOutputBtn_->setTooltip (TRANS("Enable midi output"));
    midiOutputBtn_->setButtonText (TRANS("Midi Output"));
    midiOutputBtn_->addListener (this);
    midiOutputBtn_->setToggleState (true, dontSendNotification);
    midiOutputBtn_->setColour (ToggleButton::textColourId, Colours::white);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 100);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

PluginHeader::~PluginHeader()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    pluginTxt_ = nullptr;
    scanBtn_ = nullptr;
    mecBtn_ = nullptr;
    midiOutputBtn_ = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PluginHeader::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::black);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PluginHeader::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    pluginTxt_->setBounds (8, 16, 128, 24);
    scanBtn_->setBounds (8, 48, 128, 24);
    mecBtn_->setBounds (160, 48, 150, 24);
    midiOutputBtn_->setBounds (160, 16, 112, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PluginHeader::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == scanBtn_)
    {
        //[UserButtonCode_scanBtn_] -- add your button handler code here..
        if(owner_)
        {
            owner_->getMecProcessor().openPluginList();
        }
        //[/UserButtonCode_scanBtn_]
    }
    else if (buttonThatWasClicked == mecBtn_)
    {
        //[UserButtonCode_mecBtn_] -- add your button handler code here..
        //[/UserButtonCode_mecBtn_]
    }
    else if (buttonThatWasClicked == midiOutputBtn_)
    {
        //[UserButtonCode_midiOutputBtn_] -- add your button handler code here..
        owner_->getMecProcessor().setMidiOutput(buttonThatWasClicked->getToggleState());
        //[/UserButtonCode_midiOutputBtn_]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void PluginHeader::setOwner(MecAudioProcessorEditor* owner) {
    owner_=owner;
    pluginTxt_->setOwner(owner_);
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PluginHeader" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="100">
  <BACKGROUND backgroundColour="ff000000"/>
  <LABEL name="pluginFile" id="997f1d608268aff8" memberName="pluginTxt_"
         virtualName="PluginSelector" explicitFocusOrder="0" pos="8 16 128 24"
         bkgCol="ffffffff" textCol="ff000000" outlineCol="ffffff" edTextCol="ff000000"
         edBkgCol="ffffff" labelText="select plugin..." editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="17.600000000000001421" bold="0" italic="0" justification="33"/>
  <TEXTBUTTON name="scan plugins..." id="32c934cd9cf75477" memberName="scanBtn_"
              virtualName="" explicitFocusOrder="0" pos="8 48 128 24" bgColOff="fff8f8f8"
              buttonText="scan plugins..." connectedEdges="0" needsCallback="1"
              radioGroupId="0"/>
  <TOGGLEBUTTON name="mec" id="a278bf2cd5bf218d" memberName="mecBtn_" virtualName=""
                explicitFocusOrder="0" pos="160 48 150 24" tooltip="Enable MEC"
                txtcol="ffffffff" buttonText="MEC" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="1"/>
  <TOGGLEBUTTON name="midi out" id="a86584f40ce2f98a" memberName="midiOutputBtn_"
                virtualName="" explicitFocusOrder="0" pos="160 16 112 24" tooltip="Enable midi output"
                txtcol="ffffffff" buttonText="Midi Output" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
