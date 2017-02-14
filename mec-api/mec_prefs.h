#ifndef MECPREFS_H
#define MECPREFS_H

#include <cstring>
#include <string>
#include <vector>
namespace mec {

class Preferences {
public:
    Preferences(const std::string& file = "./mec.json");
    virtual ~Preferences();
    Preferences(void* subtree);

    std::vector<std::string> getKeys() const;

    bool        getBool(const std::string& v, bool def = false) const; 
    int         getInt(const std::string& v, int def = 0) const;
    double      getDouble(const std::string& v, double def = 0.0) const;
    std::string getString(const std::string& v, const std::string def = "") const;

    void*       getArray(const std::string v) const;
    int         getArraySize(void*) const;
    int         getArrayInt(void*, int i,int def) const;
    double      getArrayDouble(void*, int i,double def) const;
    std::string getArrayString(void*,int i, const std::string def = "") const;

    void*       getSubTree(const std::string v) const;
    void*       getTree() const;

    bool        exists(const std::string v) const;
    void        print() const;
    bool        valid() const;
private:
    bool        loadPreferences(const std::string& file);

    // dont use unique_ptr, as need to use json delete
    void*   jsonData_;
    bool    owned_;
    bool    valid_;
};

}

#endif //MECPREFS_H