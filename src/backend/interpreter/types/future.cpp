#include "future.h"
#include <cstring>
#include <stdexcept>

namespace cb {

FutureValue::FutureValue(FutureValueType type)
    : type_(type), is_ready_(false), value_(nullptr) {}

FutureValue::~FutureValue() { free_value(); }

void FutureValue::free_value() {
    if (value_) {
        switch (type_) {
        case FutureValueType::INT:
            delete static_cast<int *>(value_);
            break;
        case FutureValueType::LONG:
            delete static_cast<long *>(value_);
            break;
        case FutureValueType::DOUBLE:
            delete static_cast<double *>(value_);
            break;
        case FutureValueType::STRING:
            delete static_cast<std::string *>(value_);
            break;
        case FutureValueType::VOID:
            // void型は値を持たない
            break;
        }
        value_ = nullptr;
    }
}

// int型の値を設定
void FutureValue::set_value_int(int value) {
    if (type_ != FutureValueType::INT) {
        throw std::runtime_error("Type mismatch: expected INT");
    }
    free_value();
    value_ = new int(value);
    is_ready_ = true;
}

// long型の値を設定
void FutureValue::set_value_long(long value) {
    if (type_ != FutureValueType::LONG) {
        throw std::runtime_error("Type mismatch: expected LONG");
    }
    free_value();
    value_ = new long(value);
    is_ready_ = true;
}

// double型の値を設定
void FutureValue::set_value_double(double value) {
    if (type_ != FutureValueType::DOUBLE) {
        throw std::runtime_error("Type mismatch: expected DOUBLE");
    }
    free_value();
    value_ = new double(value);
    is_ready_ = true;
}

// string型の値を設定
void FutureValue::set_value_string(const std::string &value) {
    if (type_ != FutureValueType::STRING) {
        throw std::runtime_error("Type mismatch: expected STRING");
    }
    free_value();
    value_ = new std::string(value);
    is_ready_ = true;
}

// void型の完了を設定
void FutureValue::set_ready() {
    if (type_ != FutureValueType::VOID) {
        throw std::runtime_error("set_ready() only for VOID type");
    }
    is_ready_ = true;
}

// int型の値を取得
int FutureValue::get_value_int() const {
    if (type_ != FutureValueType::INT) {
        throw std::runtime_error("Type mismatch: expected INT");
    }
    if (!is_ready_) {
        throw std::runtime_error("Future not ready");
    }
    return *static_cast<int *>(value_);
}

// long型の値を取得
long FutureValue::get_value_long() const {
    if (type_ != FutureValueType::LONG) {
        throw std::runtime_error("Type mismatch: expected LONG");
    }
    if (!is_ready_) {
        throw std::runtime_error("Future not ready");
    }
    return *static_cast<long *>(value_);
}

// double型の値を取得
double FutureValue::get_value_double() const {
    if (type_ != FutureValueType::DOUBLE) {
        throw std::runtime_error("Type mismatch: expected DOUBLE");
    }
    if (!is_ready_) {
        throw std::runtime_error("Future not ready");
    }
    return *static_cast<double *>(value_);
}

// string型の値を取得
std::string FutureValue::get_value_string() const {
    if (type_ != FutureValueType::STRING) {
        throw std::runtime_error("Type mismatch: expected STRING");
    }
    if (!is_ready_) {
        throw std::runtime_error("Future not ready");
    }
    return *static_cast<std::string *>(value_);
}

} // namespace cb
