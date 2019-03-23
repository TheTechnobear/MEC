#pragma once

#include <string>
#include <memory>
#include <vector>


#include "Entity.h"
#include "ParamValue.h"

namespace Kontrol {

enum ParameterType {
    PT_Invalid,
    PT_Float,
    PT_Boolean,
    PT_Int,
    PT_Percent,
    PT_Frequency,
    PT_Time,
    PT_Pitch,
    PT_Pan
};

class Parameter : public Entity {
public:
    static std::shared_ptr<Parameter> create(const std::vector<ParamValue> &args);

    Parameter(ParameterType type);
    virtual void createArgs(std::vector<ParamValue> &args) const;

    ParameterType type() const { return type_; };

    virtual std::string displayValue() const;
    virtual const std::string &displayUnit() const;

    ParamValue current() const { return current_; }

    virtual bool change(const ParamValue &c, bool force);
    virtual ParamValue calcRelative(float f);
    virtual ParamValue calcFloat(float f);
    virtual ParamValue calcMidi(int midi);

    virtual ParamValue calcMinimum() const;
    virtual ParamValue calcMaximum() const;

    virtual float asFloat (const ParamValue& v) const;

    virtual bool valid() { return Entity::valid() && type_ != PT_Invalid; }

    void dump() const;

protected:
    virtual void init(const std::vector<ParamValue> &args, unsigned &pos);

    ParameterType type_;
    ParamValue current_;
};


class Parameter_Int : public Parameter {
public:
    Parameter_Int(ParameterType type) : Parameter(type) { ; }

    void createArgs(std::vector<ParamValue> &args) const override;
    std::string displayValue() const override;

    bool change(const ParamValue &c, bool force) override;
    ParamValue calcRelative(float f) override;
    ParamValue calcMidi(int midi) override;
    ParamValue calcFloat(float f) override;

    ParamValue calcMinimum() const override;
    ParamValue calcMaximum() const override;

    float asFloat(const ParamValue& v) const override;

protected:
    void init(const std::vector<ParamValue> &args, unsigned &pos) override;

    int def() const { return def_; }

    int min() const { return min_; }

    int max() const { return max_; }

    int min_;
    int max_;
    int def_;
};


class Parameter_Float : public Parameter {
public:
    Parameter_Float(ParameterType type);
    void createArgs(std::vector<ParamValue> &args) const override;
    std::string displayValue() const override;

    bool change(const ParamValue &c, bool force) override;
    ParamValue calcRelative(float f) override;
    ParamValue calcMidi(int midi) override;
    ParamValue calcFloat(float f) override;

    ParamValue calcMinimum() const override;
    ParamValue calcMaximum() const override;

    float asFloat(const ParamValue& v) const override;

protected:
    void init(const std::vector<ParamValue> &args, unsigned &pos) override;

    float def() const { return def_; }

    float min() const { return min_; }

    float max() const { return max_; }

    float min_;
    float max_;
    float def_;
};

class Parameter_Boolean : public Parameter {
public:
    Parameter_Boolean(ParameterType type);
    void createArgs(std::vector<ParamValue> &args) const override;
    std::string displayValue() const override;

    bool change(const ParamValue &c, bool force) override;
    ParamValue calcRelative(float f) override;
    ParamValue calcMidi(int midi) override;
    ParamValue calcFloat(float f) override;

    ParamValue calcMinimum() const override;
    ParamValue calcMaximum() const override;

    float asFloat(const ParamValue& v) const override;

protected:
    void init(const std::vector<ParamValue> &args, unsigned &pos) override;

    bool def() const { return def_; }

    bool def_;
};

class Parameter_Percent : public Parameter_Float {
public:
    Parameter_Percent(ParameterType type) : Parameter_Float(type) { ; }

    const std::string &displayUnit() const override;
};

class Parameter_Frequency : public Parameter_Float {
public:
    Parameter_Frequency(ParameterType type) : Parameter_Float(type) { ; }

    const std::string &displayUnit() const override;
};

class Parameter_Time : public Parameter_Float {
public:
    Parameter_Time(ParameterType type) : Parameter_Float(type) { ; }

    const std::string &displayUnit() const override;
};


class Parameter_Pitch : public Parameter_Int {
public:
    Parameter_Pitch(ParameterType type) : Parameter_Int(type) { ; }

    const std::string &displayUnit() const override;
};


class Parameter_Pan : public Parameter_Float {
public:
    Parameter_Pan(ParameterType type) : Parameter_Float(type) { ; }

    std::string displayValue() const override;
};

} //namespace
