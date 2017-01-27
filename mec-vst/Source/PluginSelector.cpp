/*
  ==============================================================================

    PluginSelector.cpp
    Created: 26 Jan 2017 10:31:36pm
    Author:  Kodiak

  ==============================================================================
*/

#include "PluginSelector.h"

#include "PluginEditor.h"

PluginSelector::PluginSelector (const String& componentName, const String& labelText)
    : Label(componentName,labelText){
}


void PluginSelector::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        PopupMenu m;
        if (owner_)
        {
            owner_->getMecProcessor().addPluginsToMenu(m);
            owner_->getMecProcessor().chosenPluginMenu(m.show());
            
            PluginDescription& desc=owner_->getMecProcessor().getHostedPlugDesc();
            setText(desc.descriptiveName, sendNotificationAsync );
        }
    }
}

void PluginSelector::setOwner(MecAudioProcessorEditor* owner) {
    owner_=owner;
    PluginDescription& desc=owner_->getMecProcessor().getHostedPlugDesc();
    if(desc.descriptiveName.length()>0)
        setText(desc.descriptiveName, sendNotificationAsync);
    else
        setText("select plugin...", sendNotificationAsync);
}
