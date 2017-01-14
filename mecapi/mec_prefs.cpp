#include "mec_prefs.h"

#include <fstream>
#include <iostream>

#include "mec_log.h"

MecPreferences::MecPreferences() : jsonData_(nullptr), owned_(true) {
    valid_ = loadPreferences();
}

MecPreferences::MecPreferences(void* p) : jsonData_(static_cast<cJSON*> (p)), owned_(false) {
}



MecPreferences::~MecPreferences() {
    if (jsonData_ && owned_ ) cJSON_Delete(jsonData_);
}

bool MecPreferences::loadPreferences() {
    if (jsonData_ && owned_) {
        cJSON_Delete(jsonData_);
        jsonData_ = nullptr;
    }

    std::string file = "./mec.json";
    std::ifstream t(file);
    if (t.good())
    {
        std::string jsonStr;
        t.seekg(0, std::ios::end);
        jsonStr.reserve(t.tellg());
        t.seekg(0, std::ios::beg);

        jsonStr.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        jsonData_ = cJSON_Parse(jsonStr.c_str());
        owned_ = true;


        if (jsonData_) {
            LOG_0(std::cout << "loaded preferences" << std::endl;)
            char* p = cJSON_Print(jsonData_);
            std::cout << p << std::endl;
            delete p;
            return true;
        }
        else
        {
            LOG_0(std::cerr << "unable to parse preferences file" << std::endl;)
        }
    }
    else
    {
        LOG_0(std::cerr << "unable to load preferences file" << std::endl;)
    }
    return false;
}

int MecPreferences::getInt(const std::string& v, int def) {
    if (! jsonData_) return def;
    cJSON * node = cJSON_GetObjectItem(jsonData_, v.c_str());
    if (node!=nullptr && node->type == cJSON_Number) {
        return node->valueint;
    }
    return def;
}


double MecPreferences::getDouble(const std::string& v, double def) {
    if (! jsonData_) return def;
    cJSON * node = cJSON_GetObjectItem(jsonData_, v.c_str());
    if (node!=nullptr && node->type == cJSON_Number) {
        return node->valuedouble;
    }
    return def;
}

std::string MecPreferences::getString(const std::string& v, const std::string def) {
    if (! jsonData_) return def;
    cJSON * node = cJSON_GetObjectItem(jsonData_, v.c_str());
    if (node!=nullptr && node->type == cJSON_String) {
        return node->valuestring;
    }
    return def;
}

void* MecPreferences::getArray(const std::string v) {
    if (! jsonData_) return nullptr;
    cJSON * node = cJSON_GetObjectItem(jsonData_, v.c_str());
    if (node!=nullptr && node->type == cJSON_Array) {
        return node;
    }
    return nullptr;
}

int MecPreferences::getArraySize(void* v) {
    cJSON *node = (cJSON*) v;
    if(node!=nullptr && node->type == cJSON_Array) {
        return cJSON_GetArraySize(node);
    }
    return 0;
}

int MecPreferences::getArrayInt(void* v,int i, int def) {
    cJSON *array = (cJSON*) v;
    cJSON *node = cJSON_GetArrayItem(array,i);
    if (node!=nullptr && node->type == cJSON_Number) {
        return node->valueint;
    }
    return def;
}


void* MecPreferences::getSubTree(const std::string v) {
    if (! jsonData_) return nullptr;
    cJSON * node = cJSON_GetObjectItem(jsonData_, v.c_str());
    if (node!=nullptr && node->type == cJSON_Object) {
        return node;
    }
    return nullptr;
}

bool MecPreferences::exists(const std::string v) {
    if (! jsonData_) return false;
    cJSON * node = cJSON_GetObjectItem(jsonData_, v.c_str());
    return node != nullptr;
}

void MecPreferences::print() {
    if (! jsonData_)  {
        LOG_0(std::cout << "invalid json data" << std::endl;)

    } else {
        char* p = cJSON_Print(jsonData_);
        LOG_0(std::cout << p << std::endl;)
        delete p;
    }
}



