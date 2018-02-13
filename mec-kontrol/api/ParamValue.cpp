#include "ParamValue.h"

namespace Kontrol {

int operator>(const ParamValue &lhs, const ParamValue &rhs) {
    if (lhs.type() != rhs.type()) return lhs.type() > rhs.type();
    switch (lhs.type()) {
        case ParamValue::T_Float : {
            return lhs.floatValue() > rhs.floatValue();
        }
        case ParamValue::T_String:
            return lhs.stringValue() > rhs.stringValue();
        default:;
    }
    return lhs.stringValue() > rhs.stringValue();
}

int operator<(const ParamValue &lhs, const ParamValue &rhs) {
    if (lhs.type() != rhs.type()) return lhs.type() < rhs.type();
    switch (lhs.type()) {
        case ParamValue::T_Float : {
            return lhs.floatValue() < rhs.floatValue();
        }
        case ParamValue::T_String:
            return lhs.stringValue() < rhs.stringValue();
        default:;
    }
    return lhs.stringValue() > rhs.stringValue();
}

bool operator!=(const ParamValue &lhs, const ParamValue &rhs) {
    return !(lhs == rhs);
}

bool operator==(const ParamValue &lhs, const ParamValue &rhs) {
    if (lhs.type() != rhs.type()) return false;
    switch (lhs.type()) {
        case ParamValue::T_Float : {
            return lhs.floatValue() == rhs.floatValue();
        }
        case ParamValue::T_String:
            return lhs.stringValue() == rhs.stringValue();
        default:;
    }
    return lhs.stringValue() == rhs.stringValue();
}

}// namespace