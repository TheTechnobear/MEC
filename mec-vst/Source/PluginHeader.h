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

#ifndef __JUCE_HEADER_DAB556BA07CA9A60__
#define __JUCE_HEADER_DAB556BA07CA9A60__

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
class MecAudioProcessorEditor;
class PluginSelector;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PluginHeader  : public Component,
                      public ButtonListener
{
public:
    //==============================================================================
    PluginHeader ();
    ~PluginHeader();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setOwner(MecAudioProcessorEditor*);
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    MecAudioProcessorEditor *owner_;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<PluginSelector> pluginTxt_;
    ScopedPointer<TextButton> scanBtn_;
    ScopedPointer<ToggleButton> mecBtn_;
    ScopedPointer<ToggleButton> midiOutputBtn_;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginHeader)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_DAB556BA07CA9A60__
