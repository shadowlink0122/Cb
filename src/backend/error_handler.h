#pragma once
#include "../common/ast.h"
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// カスタム例外クラス（詳細なエラー表示済みを示すフラグ付き）
class DetailedErrorException : public std::runtime_error {
  public:
    DetailedErrorException(const std::string &message)
        : std::runtime_error(message), detailed_shown(true) {}
    bool detailed_shown;
};

// 例外の種類
enum class ExceptionType {
    DIVISION_BY_ZERO,
    ARRAY_OUT_OF_BOUNDS,
    NULL_POINTER_REFERENCE,
    TYPE_MISMATCH,
    MODULE_NOT_FOUND,
    FUNCTION_NOT_FOUND,
    RUNTIME_ERROR,
    USER_DEFINED
};

// 例外情報を保持するクラス
class CbException {
  public:
    ExceptionType type;
    std::string message;
    std::string location; // ファイル:行番号
    int64_t error_code;

    CbException(ExceptionType t, const std::string &msg,
                const std::string &loc = "", int64_t code = 0)
        : type(t), message(msg), location(loc), error_code(code) {}

    std::string to_string() const;
    const char *get_type_name() const;
};

// エラーコンテキストのスタックフレーム
struct ErrorStackFrame {
    std::string function_name;
    std::string module_name;
    std::string file_path;
    int line_number;

    ErrorStackFrame(const std::string &func = "",
                    const std::string &module = "",
                    const std::string &file = "", int line = 0)
        : function_name(func), module_name(module), file_path(file),
          line_number(line) {}
};

// エラーハンドリングコンテキスト
class ErrorContext {
  private:
    std::vector<ErrorStackFrame> call_stack;
    std::vector<std::unique_ptr<CbException>> exception_stack;
    std::unordered_map<ExceptionType, std::function<void(const CbException &)>>
        handlers;

  public:
    ErrorContext() = default;
    ~ErrorContext() = default;

    // スタック管理
    void push_stack_frame(const std::string &function_name,
                          const std::string &module_name = "",
                          const std::string &file_path = "",
                          int line_number = 0);
    void pop_stack_frame();

    // 例外処理
    void throw_exception(ExceptionType type, const std::string &message,
                         const std::string &location = "");
    void throw_exception(std::unique_ptr<CbException> exception);

    bool has_exception() const;
    const CbException *get_current_exception() const;
    std::unique_ptr<CbException> pop_exception();

    // エラーハンドラ登録
    void register_handler(ExceptionType type,
                          std::function<void(const CbException &)> handler);
    void handle_exception(const CbException &exception);

    // デバッグ情報
    void print_stack_trace() const;
    std::string get_stack_trace_string() const;

    // ユーティリティ
    void clear_exceptions();
    void reset();
    size_t stack_depth() const { return call_stack.size(); }
};

// グローバルエラーコンテキスト
extern ErrorContext *global_error_context;

// エラーハンドリングのユーティリティマクロ
#define CB_TRY_BEGIN()                                                         \
    do {                                                                       \
        if (global_error_context == nullptr) {                                 \
            global_error_context = new ErrorContext();                         \
        }

#define CB_THROW(type, message)                                                \
    global_error_context->throw_exception(                                     \
        type, message, __FILE__ ":" + std::to_string(__LINE__))

#define CB_TRY_END()                                                           \
    }                                                                          \
    while (0)

#define CB_CATCH(exception_var)                                                \
    if (global_error_context->has_exception()) {                               \
        auto exception_var = global_error_context->pop_exception();

#define CB_CATCH_END() }

// よく使われる例外を簡単に投げるためのヘルパー関数
void throw_division_by_zero(const std::string &location = "");
void throw_array_out_of_bounds(int index, int size,
                               const std::string &location = "");
void throw_null_pointer_reference(const std::string &location = "");
void throw_type_mismatch(const std::string &expected, const std::string &actual,
                         const std::string &location = "");
void throw_module_not_found(const std::string &module_name,
                            const std::string &location = "");
void throw_function_not_found(const std::string &function_name,
                              const std::string &module_name = "",
                              const std::string &location = "");
void throw_runtime_error(const std::string &message,
                         const std::string &location = "");

// 詳細なエラー表示機能
void print_error_with_location(const std::string &message,
                               const std::string &filename, int line,
                               int column, const std::string &source_line = "");

// ASTノードの位置情報を使ったエラー表示
void print_error_with_ast_location(const std::string &message,
                                   const ASTNode *node);

// 統一的なランタイムエラー（詳細表示付き）
void throw_detailed_runtime_error(const std::string &message,
                                  const ASTNode *node = nullptr);

// 統一的なランタイムエラー（位置情報付き）
void throw_detailed_runtime_error(const std::string &message,
                                  const std::string &filename, int line,
                                  int column,
                                  const std::string &source_line = "");

// ソースファイルから指定行を読み取る
std::string get_source_line(const std::string &filename, int line_number);

// カラム位置にマーカーを追加
std::string create_column_marker(int column, int length = 1);
