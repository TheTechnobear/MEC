#include "mec_prefs.h"

#include <fstream>

#include <cJSON.h>

#include "mec_log.h"

namespace mec {

Preferences::Preferences(const std::string& file) :
    jsonData_(nullptr), owned_(true) {
    valid_ = loadPreferences(file);
}

Preferences::Preferences(void* p) :
    jsonData_(static_cast<cJSON*> (p)), owned_(false),
    valid_(jsonData_ != nullptr) {
}


Preferences::~Preferences() {
    if (jsonData_ && owned_ ) cJSON_Delete((cJSON*) jsonData_);
}

bool Preferences::loadPreferences(const std::string& file) {
    if (jsonData_ && owned_) {
        cJSON_Delete((cJSON*) jsonData_);
        jsonData_ = nullptr;
    }

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
            return true;
        }
    }
    LOG_0("unable to load preferences file : " << file);
    return false;
}

std::vector<std::string> Preferences::getKeys() const {
    std::vector<std::string> ret;

    cJSON *c = static_cast<cJSON*>(jsonData_)->child;
    while (c)  {
        std::string s = c->string;
        if(s.substr(0, 1) != "_") {
            ret.push_back(s);
        }
        c = c->next;
    }
    return ret;
}

bool Preferences::getBool(const std::string& v, bool def) const {
    if (! jsonData_) return def;
    cJSON * node = cJSON_GetObjectItem((cJSON*) jsonData_, v.c_str());
    if (node != nullptr) {
        if (node->type == cJSON_True) return true;
        else if (node->type == cJSON_False) return false;
    }
    return def;
}


int Preferences::getInt(const std::string& v, int def) const {
    if (! jsonData_) return def;
    cJSON * node = cJSON_GetObjectItem((cJSON*) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_Number) {
        return node->valueint;
    }
    return def;
}


double Preferences::getDouble(const std::string& v, double def) const {
    if (! jsonData_) return def;
    cJSON * node = cJSON_GetObjectItem((cJSON*) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_Number) {
        return node->valuedouble;
    }
    return def;
}

std::string Preferences::getString(const std::string& v, const std::string def) const {
    if (! jsonData_) return def;
    cJSON * node = cJSON_GetObjectItem((cJSON*) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_String) {
        return node->valuestring;
    }
    return def;
}

void* Preferences::getArray(const std::string v) const {
    if (! jsonData_) return nullptr;
    cJSON * node = cJSON_GetObjectItem((cJSON*) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_Array) {
        return node;
    }
    return nullptr;
}

int Preferences::getArraySize(void* v) const {
    cJSON *node = (cJSON*) v;
    if (node != nullptr && node->type == cJSON_Array) {
        return cJSON_GetArraySize(node);
    }
    return 0;
}

int Preferences::getArrayInt(void* v, int i, int def) const {
    cJSON *array = (cJSON*) v;
    cJSON *node = cJSON_GetArrayItem(array, i);
    if (node != nullptr && node->type == cJSON_Number) {
        return node->valueint;
    }
    return def;
}

double Preferences::getArrayDouble(void* v, int i, double def) const {
    cJSON *array = (cJSON*) v;
    cJSON *node = cJSON_GetArrayItem(array, i);
    if (node != nullptr && node->type == cJSON_Number) {
        return node->valuedouble;
    }
    return def;
}

std::string Preferences::getArrayString(void* v, int i, const std::string def) const {
    cJSON *array = (cJSON*) v;
    cJSON *node = cJSON_GetArrayItem(array, i);
    if (node != nullptr && node->type == cJSON_String) {
        return node->valuestring;
    }
    return def;
}


void* Preferences::getTree() const {
    return jsonData_;
}

void* Preferences::getSubTree(const std::string v) const {
    if (! jsonData_) return nullptr;
    cJSON * node = cJSON_GetObjectItem((cJSON*) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_Object) {
        return node;
    }
    return nullptr;
}

bool Preferences::exists(const std::string v) const {
    if (! jsonData_) return false;
    cJSON * node = cJSON_GetObjectItem((cJSON*) jsonData_, v.c_str());
    return node != nullptr;
}

void Preferences::print() const {
    if (! jsonData_)  {
        LOG_0("invalid json data");

    } else {
        char* p = cJSON_Print((cJSON*) jsonData_);
        LOG_0(p);
        delete p;
    }
}

bool Preferences::valid() const {
    return valid_;
}


} // namespace

