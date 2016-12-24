
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppState.h"

#include <fstream>

MLAppState::MLAppState(MLPropertySet* pM, const std::string& name, const std::string& makerName, const std::string& appName, int version) :
    MLPropertyListener(pM),
	mExtraName(name),
	mMakerName(makerName),
	mAppName(appName),
	mAppVersion(version),
	mpTarget(pM)
{
	// default extra name
	if(mExtraName.length() == 0)
	{
		mExtraName = "App";
	}
	
	updateAllProperties();
}

MLAppState::~MLAppState()
{
}

void MLAppState::ignoreProperty(MLSymbol property)
{
	mIgnoredProperties.insert(property);
}

// MLPropertyListener implementation
// an updateChangedProperties() is needed to get these actions sent by the Model.
// 
void MLAppState::doPropertyChangeAction(MLSymbol p, const MLProperty & val)
{
    // nothing to do here, but we do need to be an MLPropertyListener in order to
    // know the update states of all the Properties.
	
	// debug() << "MLAppState " << mName << ": doPropertyChangeAction: " << p << " to " << val << "\n";
}


#pragma mark load and set state

bool MLAppState::loadStateFromAppStateFile()
{
	bool r = false;
    std::string file = "./" + mAppName + "AppState.txt";
    std::ifstream t(file);
    if(t.good())
    {
        std::string jsonStr;
        t.seekg(0, std::ios::end);
        jsonStr.reserve(t.tellg());
        t.seekg(0, std::ios::beg);

        jsonStr.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        r = setStateFromText(jsonStr);
    }
    else
    {
        debug() << "MLAppState::loadStateFromAppStateFile: couldn't open file!\n";
    }
    return r;
}

bool MLAppState::setStateFromText(std::string stateStr)
{
	bool r = false;
	cJSON* root = cJSON_Parse(stateStr.c_str());
	if(root)
	{
		setStateFromJSON(root);
		cJSON_Delete(root);
		r = true;
	}
	else
	{
		debug() << "MLAppState::setStateFromText: couldn't create JSON object!\n";
	}
	return r;
}

void MLAppState::setStateFromJSON(cJSON* pNode, int depth)
{
	if(!pNode) return;
	cJSON *child = pNode->child;
	while(child)
	{
		MLSymbol key(child->string);
		if(mIgnoredProperties.find(key) == mIgnoredProperties.end())
		{							
			switch(child->type & 255)
			{
				case cJSON_Number:
					//debug() << " depth " << depth << " loading float param " << child->string << " : " << child->valuedouble << "\n";
					mpTarget->setProperty(key, (float)child->valuedouble);
					break;
				case cJSON_String:
					//debug() << " depth " << depth << " loading string param " << child->string << " : " << child->valuestring << "\n";
					mpTarget->setProperty(key, child->valuestring);
					break;
				case cJSON_Object:
					//debug() << "looking at object: " << child->string << "\n";
					// see if object is a stored signal
					if(cJSON* pObjType = cJSON_GetObjectItem(child, "type"))
					{
						if(!strcmp(pObjType->valuestring, "signal") )
						{
							//debug() << " depth " << depth << " loading signal param " << child->string << "\n";
							int width = cJSON_GetObjectItem(child, "width")->valueint;
							int height = cJSON_GetObjectItem(child, "height")->valueint;
							int sigDepth = cJSON_GetObjectItem(child, "depth")->valueint;
							
							// read data into signal and set model param
							MLSignal signalValue(width, height, sigDepth);
							float* pSigData = signalValue.getBuffer();
							if(pSigData)
							{
								int widthBits = bitsToContain(width);
								int heightBits = bitsToContain(height);
								int depthBits = bitsToContain(sigDepth);
								int size = 1 << widthBits << heightBits << depthBits;
								
								cJSON* pData = cJSON_GetObjectItem(child, "data");
								int dataSize = cJSON_GetArraySize(pData);
								if(dataSize == size)
								{
									// read array
									cJSON *c=pData->child;
									int i = 0;
									while (c)
									{
										pSigData[i++] = c->valuedouble;
										c=c->next;
									}
								}
								else
								{
									debug() << "MLAppState::setStateFromJSON: wrong array size!\n";
								}
								mpTarget->setProperty(key, signalValue);
							}
						}
					}
					else
					{
						//debug() << " recursing into object " << child->string << ": \n";
						setStateFromJSON(child, depth + 1);
					}
					break;
				case cJSON_Array:
				default:
					break;
			}
		}
		child = child->next;
	}
}

void MLAppState::loadDefaultState()
{
//	load defaultappstate_txt ?
}



