#pragma once

namespace Kontrol {

class ParamValue {
public:
    enum Type {
        T_Float,
        T_String
    };

    ParamValue(const ParamValue& p) : type_(p.type_), strValue_(p.strValue_), floatValue_(p.floatValue_) {;}
    ParamValue& operator=(const ParamValue& p)  {
        type_ = p.type_;
        strValue_ = p.strValue_;
        floatValue_ = p.floatValue_;
        return *this;
    }

    ParamValue() { type_ = T_Float; floatValue_ = 0.0f; strValue_ = "";}
    ParamValue(const char* value) { type_ = T_String; strValue_ = value; floatValue_ = 0.0;}
    ParamValue(const std::string& value) { type_ = T_String; strValue_ = value; floatValue_ = 0.0; }
    ParamValue(float value) { type_ = T_Float; floatValue_ = value; strValue_ = "";}
    Type type() const { return type_;}
    const std::string& stringValue() const {return strValue_;}
    float  floatValue() const {return floatValue_;}

private:
    Type type_;
    std::string strValue_;
    float floatValue_;
};


int operator>(const ParamValue&, const ParamValue&);
int operator<(const ParamValue&, const ParamValue&);
int operator==(const ParamValue&, const ParamValue&);
int operator!=(const ParamValue&, const ParamValue&);

} //namespace
