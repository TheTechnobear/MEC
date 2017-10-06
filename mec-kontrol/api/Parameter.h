#pragma once


#include <string>
#include <memory>
#include <vector>
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

enum ParameterType {
    PT_Invalid,
    PT_Float,
    PT_Boolean,
    PT_Int,
    PT_Pct,
    PT_LinHz,
    PT_mSec,
    PT_Semi
};

class Parameter {
public:
    static std::shared_ptr<Parameter> create(const std::vector<ParamValue>& args);

    Parameter(ParameterType type);
    virtual void createArgs(std::vector<ParamValue>& args) const;

    ParameterType type() const { return type_;};
    const std::string& id() const { return id_;};

    virtual const std::string& displayName() const;
    virtual std::string displayValue() const;
    virtual const std::string& displayUnit() const;

    ParamValue current() const { return current_;}

    virtual bool change(const ParamValue& c);
    virtual ParamValue calcRelative(float f);
    virtual ParamValue calcFloat(float f);
    virtual ParamValue calcMidi(int midi);

protected:
    virtual void init(const std::vector<ParamValue>& args, unsigned& pos);

    ParameterType type_;
    std::string id_;
    std::string displayName_;
    ParamValue current_;
};


class Parameter_Int : public Parameter  {
public:
    Parameter_Int(ParameterType type) : Parameter(type) { ; }
    virtual void createArgs(std::vector<ParamValue>& args) const;
    virtual std::string displayValue() const;

    virtual bool change(const ParamValue& c);
    virtual ParamValue calcRelative(float f);
    virtual ParamValue calcMidi(int midi);
    virtual ParamValue calcFloat(float f);
protected:
    virtual void init(const std::vector<ParamValue>& args, unsigned& pos);
    int def() const { return def_;}
    int min() const { return min_;}
    int max() const { return max_;}

    int min_;
    int max_;
    int def_;
};


class Parameter_Float : public Parameter {
public:
    Parameter_Float(ParameterType type);
    virtual void createArgs(std::vector<ParamValue>& args) const;
    virtual std::string displayValue() const;

    virtual bool change(const ParamValue& c);
    virtual ParamValue calcRelative(float f);
    virtual ParamValue calcMidi(int midi);
    virtual ParamValue calcFloat(float f);
protected:
    virtual void init(const std::vector<ParamValue>& args, unsigned& pos);

    float def() const { return def_;}
    float min() const { return min_;}
    float max() const { return max_;}

    float min_;
    float max_;
    float def_;
};

class Parameter_Boolean : public Parameter {
public:
    Parameter_Boolean(ParameterType type);
    virtual void createArgs(std::vector<ParamValue>& args) const;
    virtual std::string displayValue() const;

    virtual bool change(const ParamValue& c);
    virtual ParamValue calcRelative(float f);
    virtual ParamValue calcMidi(int midi);
    virtual ParamValue calcFloat(float f);
protected:
    virtual void init(const std::vector<ParamValue>& args, unsigned& pos);

    bool def() const { return def_;}
    bool def_;
};

class Parameter_Percent : public Parameter_Float {
public:
    Parameter_Percent(ParameterType type) : Parameter_Float(type) { ; }
    virtual const std::string& displayUnit() const;
};

class Parameter_LinearHz : public Parameter_Float  {
public:
    Parameter_LinearHz(ParameterType type) : Parameter_Float(type) { ; }
    virtual const std::string& displayUnit() const;
};

class Parameter_Time : public Parameter_Float  {
public:
    Parameter_Time(ParameterType type) : Parameter_Float(type) { ; }
    virtual const std::string& displayUnit() const;
};


class Parameter_Semi : public Parameter_Int  {
public:
    Parameter_Semi(ParameterType type) : Parameter_Int(type) { ; }
    virtual const std::string& displayUnit() const;
};


} //namespace
