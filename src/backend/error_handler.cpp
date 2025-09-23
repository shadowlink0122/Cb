#include "error_handler.h"
#include "../common/ast.h"
#include <fstream>
#include <iostream>
#include <sstream>

// グローバルエラーコンテキスト
ErrorContext *global_error_context = nullptr;

// CbException の実装
std::string CbException::to_string() const {
    std::ostringstream oss;
    oss << get_type_name() << ": " << message;
    if (!location.empty()) {
        oss << " (at " << location << ")";
    }
    if (error_code != 0) {
        oss << " [code: " << error_code << "]";
    }
    return oss.str();
}

const char *CbException::get_type_name() const {
    switch (type) {
    case ExceptionType::DIVISION_BY_ZERO:
        return "DivisionByZeroError";
    case ExceptionType::ARRAY_OUT_OF_BOUNDS:
        return "ArrayOutOfBoundsError";
    case ExceptionType::NULL_POINTER_REFERENCE:
        return "NullPointerError";
    case ExceptionType::TYPE_MISMATCH:
        return "TypeMismatchError";
    case ExceptionType::MODULE_NOT_FOUND:
        return "ModuleNotFoundError";
    case ExceptionType::FUNCTION_NOT_FOUND:
        return "FunctionNotFoundError";
    case ExceptionType::RUNTIME_ERROR:
        return "RuntimeError";
    case ExceptionType::USER_DEFINED:
        return "UserDefinedError";
    default:
        return "UnknownError";
    }
}

// ErrorContext の実装
void ErrorContext::push_stack_frame(const std::string &function_name,
                                    const std::string &module_name,
                                    const std::string &file_path,
                                    int line_number) {
    call_stack.emplace_back(function_name, module_name, file_path, line_number);
}

void ErrorContext::pop_stack_frame() {
    if (!call_stack.empty()) {
        call_stack.pop_back();
    }
}

void ErrorContext::throw_exception(ExceptionType type,
                                   const std::string &message,
                                   const std::string &location) {
    auto exception = std::make_unique<CbException>(type, message, location);
    throw_exception(std::move(exception));
}

void ErrorContext::throw_exception(std::unique_ptr<CbException> exception) {
    exception_stack.push_back(std::move(exception));
}

bool ErrorContext::has_exception() const { return !exception_stack.empty(); }

const CbException *ErrorContext::get_current_exception() const {
    if (exception_stack.empty()) {
        return nullptr;
    }
    return exception_stack.back().get();
}

std::unique_ptr<CbException> ErrorContext::pop_exception() {
    if (exception_stack.empty()) {
        return nullptr;
    }
    auto exception = std::move(exception_stack.back());
    exception_stack.pop_back();
    return exception;
}

void ErrorContext::register_handler(
    ExceptionType type, std::function<void(const CbException &)> handler) {
    handlers[type] = handler;
}

void ErrorContext::handle_exception(const CbException &exception) {
    auto it = handlers.find(exception.type);
    if (it != handlers.end()) {
        it->second(exception);
    } else {
        // デフォルトハンドラ：標準エラー出力にメッセージを出力
        std::cerr << "Unhandled exception: " << exception.to_string()
                  << std::endl;
        print_stack_trace();
    }
}

void ErrorContext::print_stack_trace() const {
    std::cerr << "Stack trace:" << std::endl;
    for (int i = call_stack.size() - 1; i >= 0; --i) {
        const auto &frame = call_stack[i];
        std::cerr << "  " << (call_stack.size() - 1 - i) << ": ";

        if (!frame.function_name.empty()) {
            std::cerr << frame.function_name;
        } else {
            std::cerr << "<unknown function>";
        }

        if (!frame.module_name.empty()) {
            std::cerr << " in module " << frame.module_name;
        }

        if (!frame.file_path.empty()) {
            std::cerr << " (" << frame.file_path;
            if (frame.line_number > 0) {
                std::cerr << ":" << frame.line_number;
            }
            std::cerr << ")";
        }

        std::cerr << std::endl;
    }
}

std::string ErrorContext::get_stack_trace_string() const {
    std::ostringstream oss;
    oss << "Stack trace:" << std::endl;
    for (int i = call_stack.size() - 1; i >= 0; --i) {
        const auto &frame = call_stack[i];
        oss << "  " << (call_stack.size() - 1 - i) << ": ";

        if (!frame.function_name.empty()) {
            oss << frame.function_name;
        } else {
            oss << "<unknown function>";
        }

        if (!frame.module_name.empty()) {
            oss << " in module " << frame.module_name;
        }

        if (!frame.file_path.empty()) {
            oss << " (" << frame.file_path;
            if (frame.line_number > 0) {
                oss << ":" << frame.line_number;
            }
            oss << ")";
        }

        oss << std::endl;
    }
    return oss.str();
}

void ErrorContext::clear_exceptions() { exception_stack.clear(); }

void ErrorContext::reset() {
    call_stack.clear();
    exception_stack.clear();
    handlers.clear();
}

// ヘルパー関数の実装
void throw_division_by_zero(const std::string &location) {
    if (global_error_context == nullptr) {
        global_error_context = new ErrorContext();
    }
    global_error_context->throw_exception(ExceptionType::DIVISION_BY_ZERO,
                                          "Division by zero", location);
}

void throw_array_out_of_bounds(int index, int size,
                               const std::string &location) {
    if (global_error_context == nullptr) {
        global_error_context = new ErrorContext();
    }
    std::string message = "Array index " + std::to_string(index) +
                          " is out of bounds (size: " + std::to_string(size) +
                          ")";
    global_error_context->throw_exception(ExceptionType::ARRAY_OUT_OF_BOUNDS,
                                          message, location);
}

void throw_null_pointer_reference(const std::string &location) {
    if (global_error_context == nullptr) {
        global_error_context = new ErrorContext();
    }
    global_error_context->throw_exception(ExceptionType::NULL_POINTER_REFERENCE,
                                          "Null pointer reference", location);
}

void throw_type_mismatch(const std::string &expected, const std::string &actual,
                         const std::string &location) {
    if (global_error_context == nullptr) {
        global_error_context = new ErrorContext();
    }
    std::string message =
        "Type mismatch: expected " + expected + ", got " + actual;
    global_error_context->throw_exception(ExceptionType::TYPE_MISMATCH, message,
                                          location);
}

void throw_module_not_found(const std::string &module_name,
                            const std::string &location) {
    if (global_error_context == nullptr) {
        global_error_context = new ErrorContext();
    }
    std::string message = "Module not found: " + module_name;
    global_error_context->throw_exception(ExceptionType::MODULE_NOT_FOUND,
                                          message, location);
}

void throw_function_not_found(const std::string &function_name,
                              const std::string &module_name,
                              const std::string &location) {
    if (global_error_context == nullptr) {
        global_error_context = new ErrorContext();
    }
    std::string message = "Function not found: " + function_name;
    if (!module_name.empty()) {
        message += " in module " + module_name;
    }
    global_error_context->throw_exception(ExceptionType::FUNCTION_NOT_FOUND,
                                          message, location);
}

void throw_runtime_error(const std::string &message,
                         const std::string &location) {
    if (global_error_context == nullptr) {
        global_error_context = new ErrorContext();
    }
    global_error_context->throw_exception(ExceptionType::RUNTIME_ERROR, message,
                                          location);
}

// 詳細なエラー表示機能の実装
void print_error_with_location(const std::string &message,
                               const std::string &filename, int line,
                               int column, const std::string &source_line) {
    std::cerr << "Location: " << filename << ":" << line << ":" << column
              << std::endl;
    std::cerr << "Error: " << message << std::endl;

    if (!source_line.empty()) {
        std::cerr << "Source:" << std::endl;
        // 行番号の桁数を計算
        int line_width = std::to_string(line).length();
        std::cerr << "  " << line << " |" << source_line << std::endl;

        // カラム位置にマーカーを追加
        // プレフィックス "  line "
        // の長さを計算して、正確にcolumn位置にマーカーを配置
        int spaces_count =
            2 + line_width + 4 + column - 1; // "  " + line + " " + (column-1)
        std::string spaces(spaces_count, ' ');
        std::cerr << spaces << "^" << std::endl;
    }
}

void print_error_with_ast_location(const std::string &message,
                                   const ASTNode *node) {
    if (node && !node->location.filename.empty()) {
        std::string source_line = node->location.source_line;
        if (source_line.empty()) {
            source_line =
                get_source_line(node->location.filename, node->location.line);
        }
        print_error_with_location(message, node->location.filename,
                                  node->location.line, node->location.column,
                                  source_line);
    } else {
        std::cerr << "Error: " << message << std::endl;
    }
}

std::string get_source_line(const std::string &filename, int line_number) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }

    std::string line;
    int current_line = 1;

    while (std::getline(file, line) && current_line <= line_number) {
        if (current_line == line_number) {
            return line;
        }
        current_line++;
    }

    return "";
}

std::string create_column_marker(int column, int length) {
    std::string marker(column - 1, ' ');
    marker += std::string(length, '^');
    return marker;
}
