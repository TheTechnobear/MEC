#include "Parameter.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <mec_log.h>

namespace Kontrol {


static void throwError(const std::string &id, const char *what) {
    std::string w = id + std::string(what);
    std::runtime_error(w.c_str());
}


// Parameter : type id displayname
Parameter::Parameter(ParameterType type) : Entity("", ""), type_(type), current_(PV_INITVALUE) {
    ;
}

void Parameter::init(const std::vector<ParamValue> &args, unsigned &pos) {
    if (args.size() > pos && args[pos].type() == ParamValue::T_String) id_ = args[pos++].stringValue();
    else
        throwError("null", "missing id");
    if (args.size() > pos && args[pos].type() == ParamValue::T_String)
        displayName_ = args[pos++].stringValue();
    else throwError(id(), "missing displayName");
}

static const char *PTS_Float = "float";
static const char *PTS_Int = "int";
static const char *PTS_Boolean = "bool";
static const char *PTS_Percent = "pct";
static const char *PTS_Frequency = "freq";
static const char *PTS_Time = "time";
static const char *PTS_Pitch = "pitch";
static const char *PTS_Pan = "pan";

std::shared_ptr<Parameter> createParameter(const std::string &t) {
    if (t == PTS_Float) return std::make_shared<Parameter_Float>(PT_Float);
    else if (t == PTS_Int) return std::make_shared<Parameter_Int>(PT_Int);
    else if (t == PTS_Boolean) return std::make_shared<Parameter_Boolean>(PT_Boolean);
    else if (t == PTS_Percent) return std::make_shared<Parameter_Percent>(PT_Percent);
    else if (t == PTS_Frequency) return std::make_shared<Parameter_Frequency>(PT_Frequency);
    else if (t == PTS_Time) return std::make_shared<Parameter_Time>(PT_Time);
    else if (t == PTS_Pitch) return std::make_shared<Parameter_Pitch>(PT_Pitch);
    else if (t == PTS_Pan) return std::make_shared<Parameter_Pan>(PT_Pan);

    std::cerr << "parameter type not found: " << t << std::endl;

    return std::make_shared<Parameter>(PT_Invalid);
}

void Parameter::createArgs(std::vector<ParamValue> &args) const {
    switch (type_) {
        case PT_Float:
            args.push_back(ParamValue(PTS_Float));
            break;
        case PT_Boolean:
            args.push_back(ParamValue(PTS_Boolean));
            break;
        case PT_Int:
            args.push_back(ParamValue(PTS_Int));
            break;
        case PT_Percent:
            args.push_back(ParamValue(PTS_Percent));
            break;
        case PT_Frequency:
            args.push_back(ParamValue(PTS_Frequency));
            break;
        case PT_Time:
            args.push_back(ParamValue(PTS_Time));
            break;
        case PT_Pitch:
            args.push_back(ParamValue(PTS_Pitch));
            break;
        case PT_Pan:
            args.push_back(ParamValue(PTS_Pan));
            break;
        default:
            args.push_back(ParamValue("invalid"));
        case PT_Invalid:
            break;
    }
    args.push_back(ParamValue(id_));
    args.push_back(ParamValue(displayName_));
}


// factory for all type creation
std::shared_ptr<Parameter> Parameter::create(const std::vector<ParamValue> &args) {
    unsigned pos = 0;
    std::shared_ptr<Parameter> p;

    if (args.size() > pos && args[pos].type() == ParamValue::T_String) p = createParameter(args[pos++].stringValue());
    try {
        if (p->type() != PT_Invalid) p->init(args, pos);
    } catch (const std::runtime_error &e) {
        // perhaps report here why
        std::cerr << "error: " << e.what() << std::endl;
        p->type_ = PT_Invalid;
    }

    return p;
}


std::string Parameter::displayValue() const {
    static std::string sNullString;
    return sNullString;
}

const std::string &Parameter::displayUnit() const {
    static std::string sNullString;
    return sNullString;
}


ParamValue Parameter::calcRelative(float f) {
    switch (current_.type()) {
        case ParamValue::T_Float : {
            float v = current_.floatValue() + f;
            return calcFloat(v);
        }
        case ParamValue::T_String:
        default:;
    }
    return current_;
}

ParamValue Parameter::calcFloat(float f) {
    switch (current_.type()) {
        case ParamValue::T_Float : {
            return ParamValue(f);
        }
        case ParamValue::T_String:
        default:;
    }
    return current_;
}

ParamValue Parameter::calcMinimum() const {
    return ParamValue();
}

ParamValue Parameter::calcMaximum() const {
    return ParamValue();
}

ParamValue Parameter::calcMidi(int midi) {
    float f = (float) midi / 127.0f;
    return calcFloat(f);
}

float Parameter::asFloat(const ParamValue& pv) const {
    float v = pv.floatValue();
    v = std::max(v, -1.0f);
    v = std::min(v, 1.0f);
    return v;
}

bool Parameter::change(const ParamValue &c, bool force) {
    if (force || current_ != c) {
        current_ = c;
        return true;
    }
    return false;
}

void Parameter::dump() const {
    std::string d = id() + " : ";
    ParamValue cv = current();
    switch (cv.type()) {
        case ParamValue::T_Float :
            d += "  " + std::to_string(cv.floatValue()) + " [F],";
            break;
        case ParamValue::T_String :
        default:
            d += cv.stringValue() + " [S],";
            break;
    }
    LOG_1(d);
}

// Parameter_Float : [parameter] min max default
Parameter_Float::Parameter_Float(ParameterType type) : Parameter(type) {
    ;
}

void Parameter_Float::init(const std::vector<ParamValue> &args, unsigned &pos) {
    Parameter::init(args, pos);
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) min_ = args[pos++].floatValue();
    else
        throwError(id(), "missing min");
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) max_ = args[pos++].floatValue();
    else
        throwError(id(), "missing max");
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) def_ = args[pos++].floatValue();
    else
        throwError(id(), "missing def");
    change(def_, true);
}

void Parameter_Float::createArgs(std::vector<ParamValue> &args) const {
    Parameter::createArgs(args);
    args.push_back(ParamValue(min_));
    args.push_back(ParamValue(max_));
    args.push_back(ParamValue(def_));
}

std::string Parameter_Float::displayValue() const {
    char numbuf[11];
    snprintf(numbuf,11, "%.1f", current_.floatValue());
    return std::string(numbuf);
}


// Parameter_Float can assume float value
ParamValue Parameter_Float::calcRelative(float f) {
    float v = current().floatValue() + (f * (max() - min()));
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue(v);
}

ParamValue Parameter_Float::calcMidi(int midi) {
    float f = (float) midi / 127.0f;
    return calcFloat(f);
}

ParamValue Parameter_Float::calcFloat(float f) {
    float v = (f * (max() - min())) + min();
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue(v);
}

ParamValue Parameter_Float::calcMinimum() const {
    return ParamValue(min());
}

ParamValue Parameter_Float::calcMaximum() const {
    return ParamValue(max());
}

float Parameter_Float::asFloat(const ParamValue& v) const {
    float val = v.floatValue();
    float ret = (val - min()) / (max() - min());
    return ret;
}


bool Parameter_Float::change(const ParamValue &c, bool force) {
    switch (current_.type()) {
        case ParamValue::T_Float  : {
            float v = c.floatValue();
            v = std::max(v, min());
            v = std::min(v, max());
            return Parameter::change(ParamValue(v), force);
        }
        case ParamValue::T_String:
        default:;
    }
    return false;
}

// Parameter_Boolean :  [parameter] default
Parameter_Boolean::Parameter_Boolean(ParameterType type) : Parameter(type) {
    ;
}


std::string Parameter_Boolean::displayValue() const {
    if (current_.floatValue() > 0.5) {
        return "on";
    } else {
        return "off";
    }
}

void Parameter_Boolean::init(const std::vector<ParamValue> &args, unsigned &pos) {
    Parameter::init(args, pos);
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) {
        def_ = args[pos++].floatValue() > 0.5;
        change(def_, true);
    } else
        throwError(id(), "missing def");
}

void Parameter_Boolean::createArgs(std::vector<ParamValue> &args) const {
    Parameter::createArgs(args);
    args.push_back((float) def_);
}

bool Parameter_Boolean::change(const ParamValue &c, bool force) {
    switch (current_.type()) {
        case ParamValue::T_Float  : {
            float v = c.floatValue() > 0.5f ? 1.0f : 0.0f;
            return Parameter::change(ParamValue(v), force);
        }
        case ParamValue::T_String:
        default:;
    }
    return false;
}

ParamValue Parameter_Boolean::calcRelative(float f) {
    if (current_.floatValue() > 0.5 && f < -0.0001) {
        return ParamValue(0.0);
    }
    if (current_.floatValue() <= 0.5 && f > 0.0001) {
        return ParamValue(1.0);
    }
    return current_;
}

ParamValue Parameter_Boolean::calcFloat(float f) {
    return ParamValue(f > 0.5f ? 1.0f : 0.0f);
}

ParamValue Parameter_Boolean::calcMinimum() const {
    return ParamValue(0.0f);
}

ParamValue Parameter_Boolean::calcMaximum() const {
    return ParamValue(1.0f);
}

ParamValue Parameter_Boolean::calcMidi(int midi) {
    return ParamValue(midi > 63 ? 1.0f : 0.0f);
}

float Parameter_Boolean::asFloat(const ParamValue& v) const {
    return (current().floatValue() > 0.5f ? 1.0f : 0.0f);
}


// simple derivative types
const std::string &Parameter_Percent::displayUnit() const {
    static std::string sUnit = "%";
    return sUnit;
}

const std::string &Parameter_Frequency::displayUnit() const {
    static std::string sUnit = "Hz";
    return sUnit;
}

const std::string &Parameter_Time::displayUnit() const {
    static std::string sUnit = "mSec";
    return sUnit;
}

void Parameter_Int::init(const std::vector<ParamValue> &args, unsigned &pos) {
    Parameter::init(args, pos);
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) min_ = static_cast<int>(args[pos++].floatValue());
    else
        throwError(id(), "missing min");
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) max_ = static_cast<int>(args[pos++].floatValue());
    else
        throwError(id(), "missing max");
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) def_ = static_cast<int>(args[pos++].floatValue());
    else
        throwError(id(), "missing def");
    change(def_, true);
}

void Parameter_Int::createArgs(std::vector<ParamValue> &args) const {
    Parameter::createArgs(args);
    args.push_back(ParamValue(min_));
    args.push_back(ParamValue(max_));
    args.push_back(ParamValue(def_));
}

std::string Parameter_Int::displayValue() const {
    char numbuf[11];
    snprintf(numbuf, 11,"%d", (int) current_.floatValue());
    return std::string(numbuf);
}


bool Parameter_Int::change(const ParamValue &c, bool force) {
    switch (current_.type()) {
        case ParamValue::T_Float  : {
            int v = static_cast<int>(c.floatValue());
            v = std::max(v, min());
            v = std::min(v, max());
            return Parameter::change(ParamValue((float) v), force);
        }
        case ParamValue::T_String:
        default:;
    }
    return false;
}

ParamValue Parameter_Int::calcRelative(float f) {
    float chg = f;
    float rng = (max() - min());
    float step = 1.0f / rng;
    if (chg > 0.001 && chg < step) chg = step;
    else if (chg < 0.001 && chg > -step) chg = -step;
    int ichg = static_cast<int>(std::round(chg * rng));
    int v = static_cast<int>(current().floatValue()) + ichg;
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue((float) v);
}

ParamValue Parameter_Int::calcMidi(int midi) {
    float f = (float) midi / 127.0f;
    int v = static_cast<int>((f * (max() - min())) + min());
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue((float) v);
}

ParamValue Parameter_Int::calcFloat(float f) {
    int v = static_cast<int>((f * (max() - min())) + min());
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue((float) v);
}

ParamValue Parameter_Int::calcMinimum() const {
    return ParamValue(min());
}

ParamValue Parameter_Int::calcMaximum() const {
    return ParamValue(max());
}

float Parameter_Int::asFloat(const ParamValue& v) const {
    float val = v.floatValue();
    float ret = (val - min()) / (float(max()) - min());
    return ret;
}


const std::string &Parameter_Pitch::displayUnit() const {
    static std::string sUnit = "st";
    return sUnit;
}


std::string Parameter_Pan::displayValue() const {
    char buf[11];
    float c = current_.floatValue();
    if(c==0.5f) {
        snprintf(buf,11, "C");
    } else if (c>0.5f) {
        int i = int(((c - 0.5f) * 2.0f ) * 100.0f);
        snprintf(buf, 11,"%-3dR", i);
    } else {
        int i = int(((0.5 - c) * 2.0f ) * 100.0f);
        snprintf(buf, 11,"L%3d ", i);
    }
    return std::string(buf);
}


} //namespace
