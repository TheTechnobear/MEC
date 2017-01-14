#ifndef MECPREFS_H
#define MECPREFS_H

#include <cstring>
#include <string>

class MecPreferences {
public:
    MecPreferences(const std::string& file = "./mec.json");
    virtual ~MecPreferences();
    MecPreferences(void* subtree);

    int         getInt(const std::string& v, int def = 0);
    double      getDouble(const std::string& v, double def = 0.0);
    std::string getString(const std::string& v, const std::string def = "");
    void*       getSubTree(const std::string v);
    void*       getArray(const std::string v);
    int         getArraySize(void*);
    int         getArrayInt(void*, int i,int def);

    bool        exists(const std::string v);
    void        print();
    bool        valid() {return valid_;}

private:
    bool        loadPreferences(const std::string& file);

    // dont use unique_ptr, as need to use json delete
    void*   jsonData_;
    bool    owned_;
    bool    valid_;
};

#endif //MECPREFS_H