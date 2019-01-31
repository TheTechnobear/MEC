#include "mec_prefs.h"

#include <fstream>

#include <cJSON.h>

#include "mec_log.h"

namespace mec {

Preferences::Preferences(const std::string &file) :
        jsonData_(nullptr), owned_(true) {
    valid_ = loadPreferences(file);
}

Preferences::Preferences(void *p) :
        jsonData_(static_cast<cJSON *> (p)), owned_(false),
        valid_(jsonData_ != nullptr) {
}


Preferences::~Preferences() {
    if (jsonData_ && owned_) cJSON_Delete((cJSON *) jsonData_);
}

bool Preferences::loadPreferences(const std::string &file) {
    if (jsonData_ && owned_) {
        cJSON_Delete((cJSON *) jsonData_);
        jsonData_ = nullptr;
    }

    std::ifstream t(file);
    if (t.good()) {
        std::string jsonStr;
        t.seekg(0, std::ios::end);
        jsonStr.reserve((unsigned) t.tellg());
        t.seekg(0, std::ios::beg);

        jsonStr.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        jsonData_ = cJSON_Parse(jsonStr.c_str());
        owned_ = true;

        if (jsonData_) {
            return true;
        }

        LOG_0("unable to parse preferences file : " << file);
        std::string eptr = cJSON_GetErrorPtr();
        if (eptr.length() > 60) eptr = eptr.substr(0, 60);
        LOG_0("error around : " << eptr);
        return false;
    }
    LOG_0("unable to load preferences file : " << file);
    return false;
}

std::vector<std::string> Preferences::getKeys() const {
    std::vector<std::string> ret;

    auto *c = static_cast<cJSON *>(jsonData_)->child;
    while (c) {
        std::string s = c->string;
        ret.push_back(s);
        c = c->next;
    }
    return ret;
}

static Preferences::Type convertJsonType(int t) {
    switch (t) {
        case cJSON_False :
            return Preferences::P_BOOL;
        case cJSON_True :
            return Preferences::P_BOOL;
        case cJSON_NULL :
            return Preferences::P_NULL;
        case cJSON_Number :
            return Preferences::P_NUMBER;
        case cJSON_String :
            return Preferences::P_STRING;
        case cJSON_Array :
            return Preferences::P_ARRAY;
        case cJSON_Object :
            return Preferences::P_OBJECT;

        default:
            return Preferences::P_NULL;
    }
//    return Preferences::P_NULL;
}

Preferences::Type Preferences::getType(const std::string &v) const {
    if (!jsonData_) return P_NULL;
    auto *node = cJSON_GetObjectItem((cJSON *) jsonData_, v.c_str());
    if (node != nullptr) {
        return convertJsonType(node->type);
    }
    return P_NULL;
}


bool Preferences::getBool(const std::string &v, bool def) const {
    if (!jsonData_) return def;
    auto *node = cJSON_GetObjectItem((cJSON *) jsonData_, v.c_str());
    if (node != nullptr) {
        if (node->type == cJSON_True) return true;
        else if (node->type == cJSON_False) return false;
    }
    return def;
}


int Preferences::getInt(const std::string &v, int def) const {
    if (!jsonData_) return def;
    auto *node = cJSON_GetObjectItem((cJSON *) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_Number) {
        return node->valueint;
    }
    return def;
}


double Preferences::getDouble(const std::string &v, double def) const {
    if (!jsonData_) return def;
    auto *node = cJSON_GetObjectItem((cJSON *) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_Number) {
        return node->valuedouble;
    }
    return def;
}

std::string Preferences::getString(const std::string &v, const std::string def) const {
    if (!jsonData_) return def;
    auto *node = cJSON_GetObjectItem((cJSON *) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_String) {
        return node->valuestring;
    }
    return def;
}


void *Preferences::getTree() const {
    return jsonData_;
}

void *Preferences::getSubTree(const std::string v) const {
    if (!jsonData_) return nullptr;
    auto *node = cJSON_GetObjectItem((cJSON *) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_Object) {
        return node;
    }
    return nullptr;
}

bool Preferences::exists(const std::string v) const {
    if (!jsonData_) return false;
    auto *node = cJSON_GetObjectItem((cJSON *) jsonData_, v.c_str());
    return node != nullptr;
}

void Preferences::print() const {
    if (!jsonData_) {
        LOG_0("invalid json data");

    } else {
        char *p = cJSON_Print((cJSON *) jsonData_);
        LOG_0(p);
        delete p;
    }
}

bool Preferences::valid() const {
    return valid_;
}

void *Preferences::getArray(const std::string v) const {
    if (!jsonData_) return nullptr;
    auto *node = cJSON_GetObjectItem((cJSON *) jsonData_, v.c_str());
    if (node != nullptr && node->type == cJSON_Array) {
        return node;
    }
    return nullptr;
}


Preferences::Array::Array(void *j) : jsonData_(j) {

}

Preferences::Array::~Array() { ; }

int Preferences::Array::getSize() {
    auto *array = (cJSON *) jsonData_;
    if (array != nullptr && array->type == cJSON_Array) {
        return cJSON_GetArraySize(array);
    }
    return 0;
}

bool Preferences::Array::valid() const {
    return jsonData_ != nullptr;
}

bool Preferences::Array::getBool(unsigned i) const {
    auto *array = (cJSON *) jsonData_;
    auto *node = cJSON_GetArrayItem(array, i);
    return node != nullptr && node->type == cJSON_True;
}

int Preferences::Array::getInt(unsigned i) const {
    auto *array = (cJSON *) jsonData_;
    auto *node = cJSON_GetArrayItem(array, i);
    if (node != nullptr && node->type == cJSON_Number) {
        return node->valueint;
    }
    return 0;
}

double Preferences::Array::getDouble(unsigned i) const {
    auto *array = (cJSON *) jsonData_;
    cJSON *node = cJSON_GetArrayItem(array, i);
    if (node != nullptr && node->type == cJSON_Number) {
        return node->valuedouble;
    }
    return 0.0;
}

std::string Preferences::Array::getString(unsigned i) const {
    auto *array = (cJSON *) jsonData_;
    auto *node = cJSON_GetArrayItem(array, i);
    if (node != nullptr && node->type == cJSON_String) {
        return node->valuestring;
    }
    return "";
}

Preferences::Type Preferences::Array::getType(unsigned i) const {
    auto *array = (cJSON *) jsonData_;

    auto *node = cJSON_GetArrayItem(array, i);
    if (node != nullptr) return convertJsonType(node->type);
    return Preferences::P_NULL;
}

void *Preferences::Array::getArray(unsigned i) const {
    if (!jsonData_) return nullptr;
    auto *node = cJSON_GetArrayItem((cJSON *) jsonData_, i);
    if (node != nullptr && node->type == cJSON_Array) {
        return node;
    }
    return nullptr;
}

void *Preferences::Array::getObject(unsigned i) const {
    if (!jsonData_) return nullptr;
    auto *node = cJSON_GetArrayItem((cJSON *) jsonData_, i);
    if (node != nullptr && node->type == cJSON_Array) {
        return node;
    }
    return nullptr;
}

} // namespace

