
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_APP_STATE_H
#define __ML_APP_STATE_H

#include "MLModel.h"
#include "cJSON.h"

#include <memory>
#include <set>

extern const char* kMLStateDirName;

class MLAppState : 
    public MLPropertyListener
{
public:
	MLAppState(MLPropertySet*, const std::string& name, const std::string& makerName, 
	   const std::string& appName, int version);

    ~MLAppState();
	
	// MLPropertyListener interface
	void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
	
	void ignoreProperty(MLSymbol property);
	
	std::string getStateAsText();
	cJSON* getStateAsJSON();

	// load and set state
	bool loadStateFromAppStateFile();
	bool setStateFromText(std::string stateStr);
	void setStateFromJSON(cJSON* pNode, int depth = 0);

	// load a state to use if there are no saved preferences.
	void loadDefaultState();
	
	
protected:
	std::string mExtraName;
	std::string mMakerName;
	std::string mAppName;
	int mAppVersion;
	
private:
	MLPropertySet* mpTarget;
	std::set<MLSymbol> mIgnoredProperties;
};

#endif // __ML_APP_STATE_H
