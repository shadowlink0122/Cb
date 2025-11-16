#pragma once
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "type_inference.h"
#include <cstdio>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cb {
class EventLoop;
class SimpleEventLoop;
class FFIManager;
} // namespace cb

// 前方宣言
class OutputManager;
class StatementExecutor;
class VariableManager;
class ArrayManager;
class TypeManager;
class ExpressionEvaluator;
class StatementExecutor;
class CommonOperations;
class VariableAccessService; // DRY効率化: 統一変数アクセスサービス
class ExpressionService;     // DRY効率化: 統一式評価サービス
class ArrayProcessingService;  // DRY効率化: 統一配列処理サービス
class EnumManager;             // enum管理サービス
class StaticVariableManager;   // static変数管理サービス
class InterfaceOperations;     // interface/impl管理サービス
class StructOperations;        // struct操作管理サービス
class StructVariableManager;   // struct変数管理サービス
class StructAssignmentManager; // struct代入管理サービス
class StructSyncManager;       // struct同期管理サービス
class GlobalInitializationManager; // グローバル初期化管理サービス
class CommonOperations;
class ControlFlowExecutor;        // 制御フロー実行サービス
class StatementListExecutor;      // 文リスト・複合文実行サービス
class ReturnHandler;              // return文処理サービス
class AssertionHandler;           // アサーション文処理サービス
class BreakContinueHandler;       // break/continue文処理サービス
class FunctionDeclarationHandler; // 関数宣言処理サービス
class StructDeclarationHandler;   // 構造体宣言処理サービス
class InterfaceDeclarationHandler; // インターフェース宣言処理サービス
class ImplDeclarationHandler;     // impl宣言処理サービス
class ExpressionStatementHandler; // 式文処理サービス
class RecursiveParser;            // enum定義同期用

// 変数・関数の格納構造
struct Variable {
    TypeInfo type = TYPE_INT; // デフォルト型
    bool is_const = false;
    bool is_array = false;
    bool is_assigned = false;
    bool is_multidimensional = false;          // 多次元配列フラグ
    bool is_struct = false;                    // struct型かどうか
    bool is_pointer = false;                   // ポインタ型かどうか
    int pointer_depth = 0;                     // ポインタの深さ
    std::string pointer_base_type_name;        // ポインタ基底型名
    TypeInfo pointer_base_type = TYPE_UNKNOWN; // ポインタ基底型
    bool is_pointer_const = false;    // ポインタ自体がconst (T* const)
    bool is_pointee_const = false;    // ポイント先がconst (const T*)
    bool is_reference = false;        // 参照型かどうか (T&)
    bool is_rvalue_reference = false; // 右辺値参照かどうか (T&&) v0.10.0
    std::string reference_target;     // 参照している変数名 v0.10.0
    bool is_unsigned = false;         // unsigned修飾子かどうか
    std::string struct_type_name;     // struct型名
    bool is_private_member = false;   // struct privateメンバーフラグ
    bool destructor_called =
        false; // デストラクタが既に呼ばれたかどうか (double free防止)
    bool points_to_heap_memory =
        false; // new演算子で割り当てた生メモリを指すポインタかどうか

    // enum型用（v0.11.0 generics）
    bool is_enum = false;       // enum型かどうか
    std::string enum_type_name; // enum型名（"Option_int"など）
    std::string enum_variant;   // バリアント名（"Some", "None"など）
    // 関連値（拡張版：int64_t, string, および任意の型をサポート）
    bool has_associated_value = false;
    int64_t associated_int_value = 0;
    std::string associated_str_value;
    // v0.13.4: struct/enum型の関連値をサポート（生ポインタを使用）
    Variable *associated_value = nullptr; // 任意の型の関連値（所有権なし）

    // union型用
    std::string type_name;                // union型名（union型の場合）
    TypeInfo current_type = TYPE_UNKNOWN; // union変数の現在の型

    // 値
    int64_t value = 0;
    std::string str_value;
    float float_value = 0.0f;
    double double_value = 0.0;
    long double quad_value = 0.0L;
    __int128_t big_value = 0;

    // 配列用
    int array_size = 0;
    std::vector<int64_t> array_values;
    std::vector<float> array_float_values;
    std::vector<double> array_double_values;
    std::vector<long double> array_quad_values;
    std::vector<std::string> array_strings;

    // struct用メンバ変数
    std::map<std::string, Variable> struct_members;

    // v0.13.1: デストラクタスコープでの参照用
    // nullptrでない場合、struct_membersの代わりにこちらを参照する
    std::map<std::string, Variable> *struct_members_ref = nullptr;

    // interface用
    std::string interface_name;      // interface型の場合のinterface名
    std::string implementing_struct; // interfaceを実装しているstruct名

    // 関数ポインタ用
    bool is_function_pointer = false;  // 関数ポインタかどうか
    std::string function_pointer_name; // 指している関数名

    // 多次元配列用
    ArrayTypeInfo array_type_info;     // 多次元配列の型情報
    std::vector<int> array_dimensions; // 各次元のサイズ
    std::vector<int64_t>
        multidim_array_values; // 多次元配列データ（フラット化、整数系）
    std::vector<float> multidim_array_float_values;
    std::vector<double> multidim_array_double_values;
    std::vector<long double> multidim_array_quad_values;
    std::vector<std::string> multidim_array_strings; // 多次元文字列配列データ

    // 将来の拡張計画 (struct/interface実装後):
    // - 静的配列: int[N] - 固定サイズ、コンパイル時型チェック
    // - 動的配列: int[] - 可変サイズ、実行時操作
    // - 共通メソッド: .size(), .len() - サイズ取得
    // - 動的配列専用メソッド: .push(value), .pop(), .clear() - 要素操作
    // - 境界チェック: 配列アクセス時の自動境界検証
    // - 型安全性: 異なるサイズの静的配列間での代入エラー検出

    Variable() = default; // コンパイラ生成のデフォルトコンストラクタを使用

    // ムーブコンストラクタとムーブ代入演算子をデフォルトに
    Variable(Variable &&) = default;
    Variable &operator=(Variable &&) = default;

    // コピーコンストラクタをデバッグ出力付きで定義
    __attribute__((noinline)) Variable(const Variable &other) {
        // すべてのメンバをコピー
        type = other.type;
        is_const = other.is_const;
        is_array = other.is_array;
        is_assigned = other.is_assigned;
        is_multidimensional = other.is_multidimensional;
        is_struct = other.is_struct;
        is_pointer = other.is_pointer;
        pointer_depth = other.pointer_depth;
        pointer_base_type_name = other.pointer_base_type_name;
        pointer_base_type = other.pointer_base_type;
        is_pointer_const = other.is_pointer_const;
        is_pointee_const = other.is_pointee_const;
        is_reference = other.is_reference;
        is_rvalue_reference = other.is_rvalue_reference;
        reference_target = other.reference_target;
        is_unsigned = other.is_unsigned;
        struct_type_name = other.struct_type_name;
        is_private_member = other.is_private_member;

        // enum関連フィールド
        is_enum = other.is_enum;
        enum_type_name = other.enum_type_name;
        enum_variant = other.enum_variant;
        has_associated_value = other.has_associated_value;
        associated_int_value = other.associated_int_value;
        associated_str_value = other.associated_str_value;
        // v0.13.4: associated_valueをdeep copy
        if (other.associated_value) {
            associated_value = new Variable(*other.associated_value);
        } else {
            associated_value = nullptr;
        }

        {
            char dbg_buf[128];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[VAR_COPY_CTOR] Copying Variable: is_enum %d -> %d",
                     other.is_enum, is_enum);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        // 残りのメンバをコピー
        type_name = other.type_name;
        current_type = other.current_type;
        value = other.value;
        str_value = other.str_value;
        float_value = other.float_value;
        double_value = other.double_value;
        quad_value = other.quad_value;
        big_value = other.big_value;
        array_size = other.array_size;
        array_values = other.array_values;
        array_float_values = other.array_float_values;
        array_double_values = other.array_double_values;
        array_quad_values = other.array_quad_values;
        array_strings = other.array_strings;
        struct_members = other.struct_members;
        interface_name = other.interface_name;
        implementing_struct = other.implementing_struct;
        is_function_pointer = other.is_function_pointer;
        function_pointer_name = other.function_pointer_name;
        array_type_info = other.array_type_info;
        array_dimensions = other.array_dimensions;
        multidim_array_values = other.multidim_array_values;
        multidim_array_float_values = other.multidim_array_float_values;
        multidim_array_double_values = other.multidim_array_double_values;
        multidim_array_quad_values = other.multidim_array_quad_values;
        multidim_array_strings = other.multidim_array_strings;

        // v0.13.1: 参照もコピーする（デストラクタでselfが使用）
        struct_members_ref = other.struct_members_ref;
    }

    // 代入演算子をデバッグ出力付きで定義
    Variable &operator=(const Variable &other) {
        if (this != &other) {
            {
                char dbg_buf[128];
                snprintf(
                    dbg_buf, sizeof(dbg_buf),
                    "[VAR_ASSIGN_OP] Before: this.is_enum=%d, other.is_enum=%d",
                    is_enum, other.is_enum);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }

            // すべてのメンバをコピー
            type = other.type;
            is_const = other.is_const;
            is_array = other.is_array;
            is_assigned = other.is_assigned;
            is_multidimensional = other.is_multidimensional;
            is_struct = other.is_struct;
            is_pointer = other.is_pointer;
            pointer_depth = other.pointer_depth;
            pointer_base_type_name = other.pointer_base_type_name;
            pointer_base_type = other.pointer_base_type;
            is_pointer_const = other.is_pointer_const;
            is_pointee_const = other.is_pointee_const;
            is_reference = other.is_reference;
            is_rvalue_reference = other.is_rvalue_reference;
            reference_target = other.reference_target;
            is_unsigned = other.is_unsigned;
            struct_type_name = other.struct_type_name;
            is_private_member = other.is_private_member;

            // enum関連フィールド
            is_enum = other.is_enum;
            enum_type_name = other.enum_type_name;
            enum_variant = other.enum_variant;
            has_associated_value = other.has_associated_value;
            associated_int_value = other.associated_int_value;
            associated_str_value = other.associated_str_value;
            // v0.13.4: associated_valueをdeep copy
            if (associated_value) {
                delete associated_value;
            }
            if (other.associated_value) {
                associated_value = new Variable(*other.associated_value);
            } else {
                associated_value = nullptr;
            }

            // 残りのメンバをコピー
            type_name = other.type_name;
            current_type = other.current_type;
            value = other.value;
            str_value = other.str_value;
            float_value = other.float_value;
            double_value = other.double_value;
            quad_value = other.quad_value;
            big_value = other.big_value;
            array_size = other.array_size;
            array_values = other.array_values;
            array_float_values = other.array_float_values;
            array_double_values = other.array_double_values;
            array_quad_values = other.array_quad_values;
            array_strings = other.array_strings;
            struct_members = other.struct_members;
            interface_name = other.interface_name;
            implementing_struct = other.implementing_struct;
            is_function_pointer = other.is_function_pointer;
            function_pointer_name = other.function_pointer_name;
            array_type_info = other.array_type_info;
            array_dimensions = other.array_dimensions;
            multidim_array_values = other.multidim_array_values;
            multidim_array_float_values = other.multidim_array_float_values;
            multidim_array_double_values = other.multidim_array_double_values;
            multidim_array_quad_values = other.multidim_array_quad_values;
            multidim_array_strings = other.multidim_array_strings;

            // v0.13.1: 参照もコピーする（デストラクタでselfが使用）
            struct_members_ref = other.struct_members_ref;

            {
                char dbg_buf[256];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "[VAR_ASSIGN_OP] After copy: this.is_enum=%d, "
                         "is_array=%d, array_size=%d, array_strings.size()=%zu",
                         is_enum, is_array, array_size, array_strings.size());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
        return *this;
    }

    // 多次元配列用コンストラクタ
    Variable(const ArrayTypeInfo &array_info)
        : type(TYPE_INT), is_const(false), is_array(true), is_assigned(false),
          is_multidimensional(true), is_struct(false), is_pointer(false),
          pointer_depth(0), pointer_base_type_name(""),
          pointer_base_type(TYPE_UNKNOWN), is_pointer_const(false),
          is_pointee_const(false), is_reference(false),
          is_rvalue_reference(false), reference_target(""), is_unsigned(false),
          struct_type_name(""), is_private_member(false), is_enum(false),
          enum_type_name(""), enum_variant(""), has_associated_value(false),
          associated_int_value(0), associated_str_value(""), type_name(""),
          current_type(TYPE_UNKNOWN), value(0), str_value(""),
          float_value(0.0f), double_value(0.0), quad_value(0.0L), big_value(0),
          array_size(0), is_function_pointer(false), function_pointer_name(""),
          array_type_info(array_info) {}

    // v0.13.1: struct_membersへのアクセス（参照があればそれを使う）
    std::map<std::string, Variable> &get_struct_members() {
        return struct_members_ref ? *struct_members_ref : struct_members;
    }

    const std::map<std::string, Variable> &get_struct_members() const {
        return struct_members_ref ? *struct_members_ref : struct_members;
    }

    // struct用コンストラクタ
    Variable(const std::string &struct_name)
        : type(TYPE_STRUCT), is_const(false), is_array(false),
          is_assigned(false), is_multidimensional(false), is_struct(true),
          is_pointer(false), pointer_depth(0), pointer_base_type_name(""),
          pointer_base_type(TYPE_UNKNOWN), is_pointer_const(false),
          is_pointee_const(false), is_reference(false),
          is_rvalue_reference(false), reference_target(""), is_unsigned(false),
          struct_type_name(struct_name), is_private_member(false),
          is_enum(false), enum_type_name(""), enum_variant(""),
          has_associated_value(false), associated_int_value(0),
          associated_str_value(""), type_name(""), current_type(TYPE_UNKNOWN),
          value(0), str_value(""), float_value(0.0f), double_value(0.0),
          quad_value(0.0L), big_value(0), array_size(0),
          is_function_pointer(false), function_pointer_name("") {
        // デバッグ出力を削除（無限再帰を防ぐため）
        // extern bool debug_mode;
        // if (debug_mode) {
        //     debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE,
        //     struct_name.c_str(), "struct");
        // }
    }

    // interface用コンストラクタ
    Variable(const std::string &interface_name, bool is_interface_flag)
        : type(TYPE_INTERFACE), is_const(false), is_array(false),
          is_assigned(false), is_multidimensional(false), is_struct(false),
          is_pointer(false), pointer_depth(0), pointer_base_type_name(""),
          pointer_base_type(TYPE_UNKNOWN), is_pointer_const(false),
          is_pointee_const(false), is_reference(false),
          is_rvalue_reference(false), reference_target(""), is_unsigned(false),
          struct_type_name(""), is_private_member(false), is_enum(false),
          enum_type_name(""), enum_variant(""), has_associated_value(false),
          associated_int_value(0), associated_str_value(""), type_name(""),
          current_type(TYPE_UNKNOWN), value(0), str_value(""),
          float_value(0.0f), double_value(0.0), quad_value(0.0L), big_value(0),
          array_size(0), interface_name(interface_name),
          is_function_pointer(false), function_pointer_name("") {
        // デバッグ用
        extern bool debug_mode;
        if (debug_mode) {
            debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE,
                      interface_name.c_str(), "interface");
        }
    }

    // 多次元配列のフラットインデックス計算
    int calculate_flat_index(const std::vector<int> &indices) const {
        debug_msg(DebugMsgId::FLAT_INDEX_CALCULATED,
                  std::to_string(indices.size()).c_str(),
                  std::to_string(array_type_info.dimensions.size()).c_str());
        if (indices.size() != array_type_info.dimensions.size()) {
            throw std::runtime_error("Dimension mismatch in array access");
        }

        int flat_index = 0;
        int multiplier = 1;

        // 最後の次元から計算（row-major order）
        for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
            if (indices[i] < 0 ||
                indices[i] >= array_type_info.dimensions[i].size) {
                throw std::runtime_error("Array index out of bounds");
            }
            flat_index += indices[i] * multiplier;
            multiplier *= array_type_info.dimensions[i].size;
        }

        return flat_index;
    }

    // 多次元配列のサイズ検証
    bool validate_indices(const std::vector<int> &indices) const {
        if (indices.size() != array_type_info.dimensions.size()) {
            return false;
        }

        for (size_t i = 0; i < indices.size(); ++i) {
            if (indices[i] < 0 ||
                indices[i] >= array_type_info.dimensions[i].size) {
                return false;
            }
        }

        return true;
    }
};

// 前方宣言
struct Scope;

// v0.12.0: 非同期タスク情報（Future実行に使用）
// v0.12.1: EventLoopベースのバックグラウンド実行サポート
struct AsyncTask {
    int task_id;                  // 一意なタスクID
    const ASTNode *function_node; // 実行する関数のASTノード
    std::string function_name;    // 関数名
    std::vector<Variable> args; // 引数リスト（完全なVariable情報を保存）
    Variable *future_var; // 紐づくFuture変数へのポインタ（外部管理）

    // v0.12.1: Future情報を内部で保持（寿命管理）
    Variable internal_future; // タスク内部で管理するFuture変数
    bool use_internal_future = true; // internal_futureを使用するか

    // v0.12.1 Phase 2.0: メソッド呼び出し対応
    bool has_self = false;          // selfが存在するか
    Variable self_value;            // selfの値
    bool has_self_receiver = false; // selfの書き戻し先があるか
    std::string self_receiver_name; // selfの書き戻し先変数名

    // 実行状態
    bool is_started = false;  // 実行開始済みか
    bool is_executed = false; // 実行完了したか
    size_t current_statement_index =
        0; // 現在実行中のステートメントインデックス
    std::shared_ptr<Scope> task_scope; // タスク専用スコープ

    // 戻り値
    bool has_return_value = false;    // 戻り値があるか
    int64_t return_value = 0;         // 戻り値（int型の場合）
    double return_double_value = 0.0; // 戻り値（浮動小数点の場合）
    std::string return_string_value;  // 戻り値（string型の場合）
    Variable return_struct_value;     // 戻り値（構造体の場合）
    TypeInfo return_type = TYPE_VOID; // 戻り値の型
    bool return_is_struct = false;    // 戻り値が構造体か

    // 協調的マルチタスク用
    bool auto_yield = true; // 各ステートメント後に自動yield（デフォルトtrue）
    bool is_statement_first_time = true; // 現在のステートメントが初回実行か
    std::shared_ptr<std::map<const ASTNode *, size_t>>
        statement_positions; // ステートメント再開位置

    // v0.12.0: 非同期sleep対応
    bool is_sleeping = false; // sleep中か
    int64_t wake_up_time_ms = 0; // 起床時刻（エポックからのミリ秒）

    // v0.12.1: await対応 - 待機中のタスク管理
    bool is_waiting = false;      // 別のタスクの完了を待機中か
    int waiting_for_task_id = -1; // 待機中のタスクID (-1=待機なし)

    // v0.12.1: タイムアウト対応
    bool has_timeout = false; // タイムアウトが設定されているか
    int64_t timeout_ms = 0; // タイムアウト時刻（エポックからのミリ秒）

    AsyncTask()
        : task_id(0), function_node(nullptr), function_name(""),
          future_var(nullptr), use_internal_future(true), has_self(false),
          has_self_receiver(false), is_started(false), is_executed(false),
          current_statement_index(0), task_scope(nullptr),
          has_return_value(false), return_value(0), return_double_value(0.0),
          return_string_value(""), return_type(TYPE_VOID),
          return_is_struct(false), auto_yield(true),
          is_statement_first_time(true), is_sleeping(false), wake_up_time_ms(0),
          is_waiting(false), waiting_for_task_id(-1), has_timeout(false),
          timeout_ms(0) {}

    AsyncTask(int id, const ASTNode *node, const std::string &name,
              std::vector<Variable> arguments, Variable *future)
        : task_id(id), function_node(node), function_name(name),
          args(std::move(arguments)), future_var(future),
          use_internal_future(true), has_self(false), has_self_receiver(false),
          is_started(false), is_executed(false), current_statement_index(0),
          task_scope(nullptr), has_return_value(false), return_value(0),
          return_double_value(0.0), return_string_value(""),
          return_type(TYPE_VOID), return_is_struct(false), auto_yield(true),
          is_statement_first_time(true), is_sleeping(false), wake_up_time_ms(0),
          is_waiting(false), waiting_for_task_id(-1) {}
};

// 関数ポインタ情報
struct FunctionPointer {
    const ASTNode *function_node; // 関数定義のASTノード
    std::string function_name;    // 関数名
    TypeInfo return_type;         // 戻り値の型

    FunctionPointer()
        : function_node(nullptr), function_name(""), return_type(TYPE_UNKNOWN) {
    }

    FunctionPointer(const ASTNode *node, const std::string &name,
                    TypeInfo ret_type)
        : function_node(node), function_name(name), return_type(ret_type) {}
};

// スコープ管理
struct Scope {
    std::map<std::string, Variable> variables;
    std::map<std::string, const ASTNode *> functions;
    std::map<std::string, FunctionPointer>
        function_pointers; // 関数ポインタ変数
    std::string scope_id; // スコープの一意識別子（implメソッド用）
    std::shared_ptr<std::map<const ASTNode *, size_t>> statement_positions;

    void clear() {
        variables.clear();
        functions.clear();
        function_pointers.clear();
        scope_id.clear();
        statement_positions.reset();
    }
};

// v0.12.1 Phase 2: YieldException - async関数内でyieldした際の例外
class YieldException {
  public:
    bool is_from_loop; // ループ内の自動yieldかどうか

    YieldException(bool from_loop = false) : is_from_loop(from_loop) {}
};

class ReturnException {
  public:
    int64_t value;
    double double_value;
    long double quad_value;
    std::string str_value;
    TypeInfo type;

    // 配列戻り値サポート
    bool is_array = false;
    std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
    std::vector<std::vector<std::vector<std::string>>> str_array_3d;
    std::vector<std::vector<std::vector<double>>>
        double_array_3d; // float/double配列用
    std::string array_type_name;

    // struct戻り値サポート
    bool is_struct = false;
    Variable struct_value;

    // 構造体配列戻り値サポート
    bool is_struct_array = false;
    std::vector<std::vector<std::vector<Variable>>> struct_array_3d;
    std::string struct_type_name;

    // 参照戻り値サポート
    bool is_reference = false;
    bool is_rvalue_reference = false;     // v0.10.0: 右辺値参照
    Variable *reference_target = nullptr; // 参照先の変数へのポインタ

    // 関数ポインタ戻り値サポート
    bool is_function_pointer = false;
    std::string function_pointer_name;
    const ASTNode *function_pointer_node = nullptr;

    // ポインタ戻り値のconst情報サポート (v0.9.2)
    bool is_pointer = false;
    bool is_pointee_const = false; // const T*
    bool is_pointer_const = false; // T* const
    int pointer_depth = 0;
    TypeInfo pointer_base_type = TYPE_UNKNOWN;
    std::string pointer_base_type_name;

    // 完全初期化コンストラクタ群
    ReturnException(int64_t val, TypeInfo t = TYPE_INT)
        : value(val), double_value(static_cast<double>(val)),
          quad_value(static_cast<long double>(val)), str_value(""), type(t),
          is_array(false), is_struct(false), is_struct_array(false),
          is_reference(false), reference_target(nullptr),
          is_function_pointer(false), function_pointer_name(""),
          function_pointer_node(nullptr), is_pointer(false),
          is_pointee_const(false), is_pointer_const(false), pointer_depth(0),
          pointer_base_type(TYPE_UNKNOWN), pointer_base_type_name("") {}
    ReturnException(double val, TypeInfo t = TYPE_DOUBLE)
        : value(static_cast<int64_t>(val)), double_value(val),
          quad_value(static_cast<long double>(val)), str_value(""), type(t),
          is_array(false), is_struct(false), is_struct_array(false),
          is_reference(false), reference_target(nullptr),
          is_function_pointer(false), function_pointer_name(""),
          function_pointer_node(nullptr), is_pointer(false),
          is_pointee_const(false), is_pointer_const(false), pointer_depth(0),
          pointer_base_type(TYPE_UNKNOWN), pointer_base_type_name("") {}
    ReturnException(long double val, TypeInfo t = TYPE_QUAD)
        : value(static_cast<int64_t>(val)),
          double_value(static_cast<double>(val)), quad_value(val),
          str_value(""), type(t), is_array(false), is_struct(false),
          is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr),
          is_pointer(false), is_pointee_const(false), is_pointer_const(false),
          pointer_depth(0), pointer_base_type(TYPE_UNKNOWN),
          pointer_base_type_name("") {}
    ReturnException(const std::string &str)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(str),
          type(TYPE_STRING), is_array(false), is_struct(false),
          is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr),
          is_pointer(false), is_pointee_const(false), is_pointer_const(false),
          pointer_depth(0), pointer_base_type(TYPE_UNKNOWN),
          pointer_base_type_name("") {}

    // 配列戻り値用コンストラクタ
    ReturnException(const std::vector<std::vector<std::vector<int64_t>>> &arr,
                    const std::string &type_name, TypeInfo t)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""), type(t),
          is_array(true), int_array_3d(arr), array_type_name(type_name),
          is_struct(false), is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr),
          is_pointer(false), is_pointee_const(false), is_pointer_const(false),
          pointer_depth(0), pointer_base_type(TYPE_UNKNOWN),
          pointer_base_type_name("") {}

    ReturnException(
        const std::vector<std::vector<std::vector<std::string>>> &arr,
        const std::string &type_name, TypeInfo t)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""), type(t),
          is_array(true), str_array_3d(arr), array_type_name(type_name),
          is_struct(false), is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr),
          is_pointer(false), is_pointee_const(false), is_pointer_const(false),
          pointer_depth(0), pointer_base_type(TYPE_UNKNOWN),
          pointer_base_type_name("") {}

    // float/double配列戻り値用コンストラクタ
    ReturnException(const std::vector<std::vector<std::vector<double>>> &arr,
                    const std::string &type_name, TypeInfo t)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""), type(t),
          is_array(true), double_array_3d(arr), array_type_name(type_name),
          is_struct(false), is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr),
          is_pointer(false), is_pointee_const(false), is_pointer_const(false),
          pointer_depth(0), pointer_base_type(TYPE_UNKNOWN),
          pointer_base_type_name("") {}

    // struct戻り値用コンストラクタ
    ReturnException(const Variable &struct_var)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""),
          type(struct_var.type), is_array(false), is_struct(true),
          struct_value(struct_var), is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr),
          is_pointer(false), is_pointee_const(false), is_pointer_const(false),
          pointer_depth(0), pointer_base_type(TYPE_UNKNOWN),
          pointer_base_type_name("") {}

    // 構造体配列戻り値用コンストラクタ
    ReturnException(
        const std::vector<std::vector<std::vector<Variable>>> &struct_arr,
        const std::string &type_name)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""),
          type(TYPE_STRUCT), is_array(true), is_struct(true),
          is_struct_array(true), struct_array_3d(struct_arr),
          struct_type_name(type_name), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr),
          is_pointer(false), is_pointee_const(false), is_pointer_const(false),
          pointer_depth(0), pointer_base_type(TYPE_UNKNOWN),
          pointer_base_type_name("") {}

    // 参照戻り値用コンストラクタ
    ReturnException(Variable *ref_target)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""),
          type(ref_target ? ref_target->type : TYPE_UNKNOWN), is_array(false),
          is_struct(false), is_struct_array(false), is_reference(true),
          reference_target(ref_target), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr),
          is_pointer(false), is_pointee_const(false), is_pointer_const(false),
          pointer_depth(0), pointer_base_type(TYPE_UNKNOWN),
          pointer_base_type_name("") {}

    // 関数ポインタ戻り値用コンストラクタ
    ReturnException(int64_t val, const std::string &func_name,
                    const ASTNode *func_node, TypeInfo t)
        : value(val), double_value(static_cast<double>(val)),
          quad_value(static_cast<long double>(val)), str_value(""), type(t),
          is_array(false), is_struct(false), is_struct_array(false),
          is_reference(false), reference_target(nullptr),
          is_function_pointer(true), function_pointer_name(func_name),
          function_pointer_node(func_node), is_pointer(false),
          is_pointee_const(false), is_pointer_const(false), pointer_depth(0),
          pointer_base_type(TYPE_UNKNOWN), pointer_base_type_name("") {}
};

class BreakException {
  public:
    int64_t condition;
    BreakException(int64_t cond = 1) : condition(cond) {}
};

class ContinueException {
  public:
    int64_t condition;
    ContinueException(int64_t cond = 1) : condition(cond) {}
};

// インタープリター実装
class Interpreter : public EvaluatorInterface {
  private:
    std::vector<Scope> scope_stack;
    Scope global_scope;
    std::vector<std::shared_ptr<std::map<const ASTNode *, size_t>>>
        statement_position_stack_;
    bool debug_mode;
    std::unique_ptr<OutputManager> output_manager_;
    std::map<std::string, std::string>
        typedef_map; // typedef alias -> base type mapping
    std::map<std::string, StructDefinition>
        struct_definitions_; // struct定義の保存

    // v0.10.0: コンストラクタ/デストラクタ管理
    // struct名 → コンストラクタリスト（オーバーロード対応）
    std::map<std::string, std::vector<const ASTNode *>> struct_constructors_;
    // struct名 → デストラクタ（1つのみ）
    std::map<std::string, const ASTNode *> struct_destructors_;

    // Defer管理（スコープごとのdeferスタック）
    std::vector<std::vector<const ASTNode *>> defer_stacks_;

    // v0.10.0: デストラクタ管理（スコープごとのstruct変数スタック）
    // 各スコープで作成されたstruct変数を記録（LIFO順で破棄するため）
    std::vector<std::vector<std::pair<std::string, std::string>>>
        destructor_stacks_;
    // pair<変数名, struct型名>

    // v0.10.0: モジュール管理（インポート済みモジュールの追跡）
    std::set<std::string> loaded_modules;

    // v0.11.0: 実行時型解決システム
    // メソッド呼び出し時の型コンテキストスタック（ネストした呼び出しに対応）
    std::vector<TypeContext> type_context_stack_;

    // v0.11.0: implノードの所有権管理（Parser破棄後もノードを保持）
    std::vector<std::unique_ptr<ASTNode>> impl_nodes_;

    // ポインタ要素型マップ（deep copyされた配列の型情報保持）
    std::map<uintptr_t, std::string> pointer_element_types_;

    // v0.12.0: Event Loop（非同期処理基盤）
    std::unique_ptr<class cb::EventLoop> event_loop_;

    // v0.12.1 Phase 2.0: Simple Event Loop（協調的マルチタスキング）
    std::unique_ptr<class cb::SimpleEventLoop> simple_event_loop_;

    // v0.13.0: Foreign Function Interface manager
    std::unique_ptr<class cb::FFIManager> ffi_manager_;

    // v0.12.0: async関数のタスクカウンター（一意なFuture識別用）
    int async_task_counter_ = 0;

    // v0.12.0: 非同期タスク管理（task_id -> AsyncTask）
    std::map<int, AsyncTask> async_tasks_;

    // v0.12.0: Future変数名からタスクIDへのマッピング
    std::map<std::string, int> future_to_task_;

    // v0.12.0 Phase 8: ヒープに確保されたFuture変数の管理（task_id -> Future）
    // イベントループからアクセス可能にするため
    std::map<int, std::shared_ptr<Variable>> future_variables_;

    // v0.12.0: auto_yieldタスク実行中フラグ
    // forループやwhileループの各イテレーション後に自動yieldするために使用
    bool is_in_auto_yield_task_ = false;

    // v0.12.1: 現在実行中のタスクID（await時の親タスク特定用）
    int current_executing_task_id_ = -1;

    // Manager instances
    std::unique_ptr<VariableManager> variable_manager_;
    std::unique_ptr<ArrayManager> array_manager_;
    std::unique_ptr<TypeManager> type_manager_;
    std::unique_ptr<ExpressionEvaluator> expression_evaluator_;
    std::unique_ptr<StatementExecutor> statement_executor_;
    std::unique_ptr<CommonOperations> common_operations_;
    std::unique_ptr<VariableAccessService>
        variable_access_service_; // DRY効率化: 統一変数アクセス
    std::unique_ptr<ExpressionService>
        expression_service_; // DRY効率化: 統一式評価
    std::unique_ptr<ArrayProcessingService>
        array_processing_service_; // DRY効率化: 統一配列処理
    std::unique_ptr<EnumManager> enum_manager_; // enum管理サービス
    std::unique_ptr<StaticVariableManager>
        static_variable_manager_; // static変数管理
    std::unique_ptr<InterfaceOperations>
        interface_operations_; // interface/impl管理
    std::unique_ptr<StructOperations> struct_operations_; // struct操作管理
    std::unique_ptr<StructVariableManager>
        struct_variable_manager_; // struct変数管理
    std::unique_ptr<StructAssignmentManager>
        struct_assignment_manager_; // struct代入管理
    std::unique_ptr<StructSyncManager> struct_sync_manager_; // struct同期管理
    std::unique_ptr<GlobalInitializationManager>
        global_initialization_manager_; // グローバル初期化管理
    std::unique_ptr<ControlFlowExecutor>
        control_flow_executor_; // 制御フロー実行
    std::unique_ptr<StatementListExecutor>
        statement_list_executor_; // 文リスト・複合文実行
    std::unique_ptr<ReturnHandler> return_handler_; // return文処理
    std::unique_ptr<AssertionHandler> assertion_handler_; // アサーション文処理
    std::unique_ptr<BreakContinueHandler>
        break_continue_handler_; // break/continue文処理
    std::unique_ptr<FunctionDeclarationHandler>
        function_declaration_handler_; // 関数宣言処理
    std::unique_ptr<StructDeclarationHandler>
        struct_declaration_handler_; // 構造体宣言処理
    std::unique_ptr<InterfaceDeclarationHandler>
        interface_declaration_handler_; // インターフェース宣言処理
    std::unique_ptr<ImplDeclarationHandler>
        impl_declaration_handler_; // impl宣言処理
    std::unique_ptr<ExpressionStatementHandler>
        expression_statement_handler_; // 式文処理

    // Grant access to managers
    friend class VariableManager;
    friend class ArrayManager;
    friend class TypeManager;
    friend class ArrayProcessingService; // DRY効率化: 配列処理統合サービス
    friend class EnumManager;            // enum管理サービス
    friend class StructOperations;            // struct操作管理
    friend class StructVariableManager;       // struct変数管理
    friend class StructAssignmentManager;     // struct代入管理
    friend class StructSyncManager;           // struct同期管理
    friend class GlobalInitializationManager; // グローバル初期化管理
    friend class ControlFlowExecutor;         // 制御フロー実行
    friend class StatementListExecutor;       // 文リスト・複合文実行
    friend class ReturnHandler;               // return文処理
    friend class AssertionHandler;            // アサーション文処理
    friend class BreakContinueHandler;        // break/continue文処理
    friend class FunctionDeclarationHandler;  // 関数宣言処理
    friend class StructDeclarationHandler;    // 構造体宣言処理
    friend class InterfaceDeclarationHandler; // インターフェース宣言処理
    friend class ImplDeclarationHandler;      // impl宣言処理
    friend class ExpressionStatementHandler; // 式文処理

  public:
    Interpreter(bool debug = false);
    virtual ~Interpreter();

    // EvaluatorInterface実装
    void process(const ASTNode *ast) override;
    int64_t evaluate(const ASTNode *node) override;
    TypedValue evaluate_typed(const ASTNode *node);

    // 出力管理
    OutputManager &get_output_manager() { return *output_manager_; }

    // スコープ管理
    void push_scope();
    void push_scope(const std::string &scope_id); // スコープID付きpush
    void pop_scope();
    void push_interpreter_scope() { push_scope(); }
    void pop_interpreter_scope() { pop_scope(); }
    Scope &current_scope();
    Scope &get_current_scope() { return current_scope(); }
    Scope &get_global_scope() { return global_scope; }
    std::shared_ptr<std::map<const ASTNode *, size_t>>
    current_statement_positions() {
        if (statement_position_stack_.empty()) {
            statement_position_stack_.emplace_back(
                std::make_shared<std::map<const ASTNode *, size_t>>());
        }
        return statement_position_stack_.back();
    }
    void set_current_statement_positions(
        std::shared_ptr<std::map<const ASTNode *, size_t>> positions) {
        if (!positions) {
            positions = std::make_shared<std::map<const ASTNode *, size_t>>();
        }
        if (statement_position_stack_.empty()) {
            statement_position_stack_.push_back(positions);
        } else {
            statement_position_stack_.back() = positions;
        }
        if (!scope_stack.empty()) {
            scope_stack.back().statement_positions =
                statement_position_stack_.back();
        }
    }

    // デストラクタスコープ管理（変数スコープは作成しない）
    void push_destructor_scope();
    void pop_destructor_scope();

    // Defer管理
    void push_defer_scope();
    void pop_defer_scope();
    void add_defer(const ASTNode *stmt);
    void execute_defers();
    void execute_pre_return_cleanup(); // return文実行前のクリーンアップ

    // 変数・関数アクセス
    Variable *find_variable(const std::string &name);
    Variable *get_variable(const std::string &name) {
        return find_variable(name);
    }

    // v0.12.1: 変数の存在チェック（forループのinit式スキップ用）
    bool variable_exists(const std::string &name) {
        return find_variable(name) != nullptr;
    }

    // v0.12.1 Phase 2.0 FIX: 現在のスコープのみでの変数存在チェック
    // 複数のforループで同じ変数名を使っても干渉しないようにする
    bool variable_exists_in_current_scope(const std::string &name) {
        if (scope_stack.empty()) {
            return false;
        }
        const auto &current_scope = scope_stack.back();
        return current_scope.variables.find(name) !=
               current_scope.variables.end();
    }

    // v0.12.1 Phase 2.0 FIX: 現在のスコープから変数を削除
    // forループのinit式で宣言された変数をループ終了時に削除する
    void remove_variable_from_current_scope(const std::string &name) {
        if (scope_stack.empty()) {
            return;
        }
        auto &current_scope = scope_stack.back();
        current_scope.variables.erase(name);
    }

    std::string find_variable_name_by_address(const Variable *target_var);

    // 一時変数管理（メソッドチェーン用） (InterfaceOperationsへ委譲)
    void add_temp_variable(const std::string &name, const Variable &var);
    void remove_temp_variable(const std::string &name);
    void clear_temp_variables();
    const ASTNode *find_function(const std::string &name);
    const ASTNode *get_function(const std::string &name) {
        return find_function(name);
    }

    // AST処理
    void register_global_declarations(const ASTNode *node);
    void initialize_global_variables(const ASTNode *node);

    // 組み込み型の初期化
    void initialize_builtin_types();
    void register_builtin_struct_future(); // v0.12.1: Future<T>
    void register_builtin_enum_option();
    void register_builtin_enum_result();

    void execute_statement(const ASTNode *node);
    void exec_statement(const ASTNode *node) { execute_statement(node); }
    int64_t eval_expression(const ASTNode *node) { return evaluate(node); }

    // 変数操作 (publicに移動)
    void assign_variable(const std::string &name, int64_t value,
                         TypeInfo type = TYPE_INT);
    void assign_variable(const std::string &name, const std::string &value);
    void assign_variable(const std::string &name, int64_t value, TypeInfo type,
                         bool is_const);
    void assign_variable(const std::string &name, const std::string &value,
                         bool is_const);
    void assign_variable(const std::string &name, const TypedValue &value,
                         TypeInfo type_hint, bool is_const);

    // Union型代入
    void assign_union_variable(const std::string &name,
                               const ASTNode *value_node);

    void assign_function_parameter(const std::string &name, int64_t value,
                                   TypeInfo type, bool is_unsigned);
    void assign_function_parameter(const std::string &name,
                                   const TypedValue &value, TypeInfo type,
                                   bool is_unsigned);
    // v0.11.0 Phase 1a: 型名を受け取るオーバーロード
    void assign_function_parameter(const std::string &name,
                                   const TypedValue &value, TypeInfo type,
                                   const std::string &type_name,
                                   bool is_unsigned);
    void assign_array_parameter(const std::string &name,
                                const Variable &source_array, TypeInfo type);
    void assign_interface_view(const std::string &dest_name,
                               Variable interface_var,
                               const Variable &source_var,
                               const std::string &source_var_name);
    void assign_array_element(const std::string &name, int64_t index,
                              int64_t value);
    void assign_array_element_float(const std::string &name, int64_t index,
                                    double value);
    void assign_string_element(const std::string &name, int64_t index,
                               const std::string &value);

    // v0.11.0 Week 3 Day 1: 構造体配列要素への代入
    void assign_struct_to_array_element(const std::string &array_name,
                                        int64_t index,
                                        const Variable &struct_value);

    // 配列リテラル割り当て
    void assign_array_literal(const std::string &name,
                              const ASTNode *literal_node);

    // 関数戻り値からの配列割り当て
    void assign_array_from_return(const std::string &name,
                                  const ReturnException &ret);

    // 型解決 (TypeManagerへの薄いラッパー、将来的にはインライン化予定)
    TypeInfo resolve_type_alias(TypeInfo base_type,
                                const std::string &type_name);

    // typedef処理 (TypeManagerへの薄いラッパー、将来的にはインライン化予定)
    std::string resolve_typedef(const std::string &type_name);

    // struct処理
    void register_struct_definition(const std::string &struct_name,
                                    const StructDefinition &definition);
    const StructDefinition *
    find_struct_definition(const std::string &struct_name);

    const ASTNode *find_union_definition(const std::string &union_name);
    const ASTNode *find_typedef_definition(const std::string &typedef_name);
    void create_struct_variable(const std::string &var_name,
                                const std::string &struct_type_name);
    Variable *get_struct_member(const std::string &var_name,
                                const std::string &member_name);
    void assign_struct_literal(const std::string &var_name,
                               const ASTNode *literal_node);
    // 構造体メンバの個別変数を再帰的に作成
    void create_struct_member_variables_recursively(
        const std::string &base_path, const std::string &struct_type_name,
        Variable &parent_var);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name, int64_t value);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const std::string &value);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const TypedValue &typed_value);
    // v0.12.1: enum変数などVariable全体を渡す版
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const Variable &value_var);
    // 構造体メンバーに構造体を代入
    void assign_struct_member_struct(const std::string &var_name,
                                     const std::string &member_name,
                                     const Variable &struct_value);
    void assign_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index, int64_t value);
    void assign_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index,
                                            const std::string &value);
    int64_t get_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index);
    // N次元配列アクセス対応版
    int64_t get_struct_member_multidim_array_element(
        const std::string &var_name, const std::string &member_name,
        const std::vector<int64_t> &indices);
    std::string get_struct_member_array_string_element(
        const std::string &var_name, const std::string &member_name, int index);
    void assign_struct_member_array_literal(const std::string &var_name,
                                            const std::string &member_name,
                                            const ASTNode *array_literal);
    // 型名から型情報への変換
    // (TypeManagerへの薄いラッパー、将来的にはインライン化予定)
    TypeInfo string_to_type_info(const std::string &type_str);
    void sync_struct_members_from_direct_access(const std::string &var_name);
    void sync_direct_access_from_struct_value(const std::string &var_name,
                                              const Variable &struct_value);
    void sync_individual_member_from_struct(Variable *struct_var,
                                            const std::string &member_name);
    void ensure_struct_member_access_allowed(const std::string &accessor_name,
                                             const std::string &member_name);
    bool is_current_impl_context_for(const std::string &struct_type_name);

    // static変数処理 (StaticVariableManagerへ委譲)
    Variable *find_static_variable(const std::string &name);
    void create_static_variable(const std::string &name, const ASTNode *node);

    // v0.10.0: コンストラクタ/デストラクタ呼び出し
    void call_default_constructor(const std::string &var_name,
                                  const std::string &struct_type_name);
    void call_constructor(const std::string &var_name,
                          const std::string &struct_type_name,
                          const std::vector<TypedValue> &args);
    void call_copy_constructor(const std::string &var_name,
                               const std::string &struct_type_name,
                               const std::string &source_var_name);
    void call_destructor(const std::string &var_name,
                         const std::string &struct_type_name);
    void register_destructor_call(const std::string &var_name,
                                  const std::string &struct_type_name);

    // v0.12.1: コンストラクタ/デストラクタの登録
    void register_constructor(const std::string &struct_name,
                              const ASTNode *ctor_node);
    void register_destructor(const std::string &struct_name,
                             const ASTNode *dtor_node);

    // impl static変数処理 (StaticVariableManagerへ委譲)
    Variable *find_impl_static_variable(const std::string &name);
    void create_impl_static_variable(const std::string &name,
                                     const ASTNode *node);
    void enter_impl_context(const std::string &interface_name,
                            const std::string &struct_type_name);
    void exit_impl_context();
    std::string get_impl_static_namespace() const;

    // interface管理 (InterfaceOperationsへ委譲)
    void register_interface_definition(const std::string &interface_name,
                                       const InterfaceDefinition &definition);
    const InterfaceDefinition *
    find_interface_definition(const std::string &interface_name);

    // v0.11.0 Phase 1a: インターフェース境界の型チェック
    void validate_all_interface_bounds();

    // impl管理 (InterfaceOperationsへ委譲)
    void register_impl_definition(const ImplDefinition &impl_def);
    const ImplDefinition *
    find_impl_for_struct(const std::string &struct_name,
                         const std::string &interface_name);

    // interface型変数管理 (InterfaceOperationsへ委譲)
    void create_interface_variable(const std::string &var_name,
                                   const std::string &interface_name);
    Variable *get_interface_variable(const std::string &var_name);

    // impl定義へのアクセサ (InterfaceOperationsへ委譲)
    const std::deque<ImplDefinition> &get_impl_definitions() const;

    // 関数コンテキスト
    std::string current_function_name; // 現在実行中の関数名

  private:
    void print_value(const ASTNode *expr);
    void print_formatted(const ASTNode *format_str, const ASTNode *arg_list);
    void validate_struct_recursion_rules();

    // v0.10.0: デストラクタ呼び出し中フラグ（無限再帰防止）
    bool is_calling_destructor_ = false;

    // N次元配列リテラル処理の再帰関数
    void process_ndim_array_literal(const ASTNode *literal_node, Variable &var,
                                    TypeInfo elem_type, int &flat_index,
                                    int max_size);

    // impl宣言処理ヘルパー
    void handle_impl_declaration(const ASTNode *node);

    // import文処理ヘルパー
    void handle_import_statement(const ASTNode *node);

  public:
    void check_type_range(TypeInfo type, int64_t value, const std::string &name,
                          bool is_unsigned = false);

    // 関数コンテキストへのアクセス
    const std::string &get_current_function_name() const {
        return current_function_name;
    }

    // v0.13.1: デストラクタ実行中かチェック
    bool is_calling_destructor() const { return is_calling_destructor_; }

    // エラー表示ヘルパー関数
    void throw_runtime_error_with_location(const std::string &message,
                                           const ASTNode *node = nullptr);
    void print_error_at_node(const std::string &message,
                             const ASTNode *node = nullptr);

    // v0.12.0: async/await support
    int get_async_task_counter() const { return async_task_counter_; }
    void increment_async_task_counter() { async_task_counter_++; }

    // v0.12.1 Phase 2.0: SimpleEventLoop access
    class cb::SimpleEventLoop &get_simple_event_loop();
    bool has_simple_event_loop() const { return simple_event_loop_ != nullptr; }

    // v0.12.1 Phase 2.0: バックグラウンドタスクを1サイクル実行
    // 非async関数のループからバックグラウンドタスクを進めるために使用
    void run_background_tasks_one_cycle();

    // v0.12.0: auto_yieldタスク実行制御
    void set_auto_yield_mode(bool enabled) { is_in_auto_yield_task_ = enabled; }
    bool is_in_auto_yield_mode() const { return is_in_auto_yield_task_; }

    // v0.12.0: 非同期タスク管理
    void register_async_task(int task_id, const AsyncTask &task) {
        async_tasks_[task_id] = task;
    }

    AsyncTask *get_async_task(int task_id) {
        auto it = async_tasks_.find(task_id);
        return (it != async_tasks_.end()) ? &it->second : nullptr;
    }

    void associate_future_with_task(const std::string &future_var_name,
                                    int task_id) {
        future_to_task_[future_var_name] = task_id;
    }

    int get_task_id_for_future(const std::string &future_var_name) {
        auto it = future_to_task_.find(future_var_name);
        return (it != future_to_task_.end()) ? it->second : -1;
    }

    // v0.12.0 Phase 8: Future変数のヒープ管理
    void register_future_variable(int task_id,
                                  std::shared_ptr<Variable> future_var) {
        future_variables_[task_id] = future_var;
    }

    std::shared_ptr<Variable> get_future_variable(int task_id) {
        auto it = future_variables_.find(task_id);
        return (it != future_variables_.end()) ? it->second : nullptr;
    }

    // デバッグ機能
    void set_debug_mode(bool debug) { debug_mode = debug; }
    bool is_debug_mode() const { return debug_mode; }

    // モジュール機能
    bool is_module_imported(const std::string &module_name) const {
        return loaded_modules.find(module_name) != loaded_modules.end();
    }
    void mark_module_loaded(const std::string &module_path) {
        loaded_modules.insert(module_path);
    }

    // 共通操作へのアクセス
    CommonOperations *get_common_operations() {
        return common_operations_.get();
    }

    // ExpressionEvaluatorへのアクセス
    ExpressionEvaluator *get_expression_evaluator() {
        return expression_evaluator_.get();
    }
    TypedValue evaluate_typed_expression(const ASTNode *node);

    // FFI managerへのアクセス
    cb::FFIManager *get_ffi_manager() { return ffi_manager_.get(); }

    // TypeManagerへのアクセス
    TypeManager *get_type_manager() { return type_manager_.get(); }

    // EnumManagerへのアクセス
    EnumManager *get_enum_manager() { return enum_manager_.get(); }

    // VariableManagerへのアクセス
    VariableManager *get_variable_manager() { return variable_manager_.get(); }

    // スコープへのアクセス（InterfaceOperations用）
    std::vector<Scope> &get_scope_stack() { return scope_stack; }

    // 変数・関数の登録ヘルパー（InterfaceOperations用）
    void add_variable_to_current_scope(const std::string &name,
                                       const Variable &var) {
        current_scope().variables[name] = var;
    }
    void register_function_to_global(const std::string &key,
                                     const ASTNode *func) {
        global_scope.functions[key] = func;
    }

    // 型推論付き三項演算子評価
    TypedValue evaluate_ternary_typed(const ASTNode *node);

    // 関数定義の検索
    const ASTNode *find_function_definition(const std::string &func_name);

    // Parserからのenum定義同期
    void sync_enum_definitions_from_parser(RecursiveParser *parser);

    // Parserからのstruct定義同期
    void sync_struct_definitions_from_parser(RecursiveParser *parser);

    // v0.11.0: Parserからのinterface/impl定義同期
    void sync_interface_definitions_from_parser(RecursiveParser *parser);
    void sync_impl_definitions_from_parser(RecursiveParser *parser);

    // struct定義へのアクセス（sizeof演算子などから使用）
    const std::map<std::string, StructDefinition> &
    get_struct_definitions() const {
        return struct_definitions_;
    }

    bool has_struct_definition(const std::string &struct_name) const {
        return struct_definitions_.find(struct_name) !=
               struct_definitions_.end();
    }

    const StructDefinition *
    get_struct_definition(const std::string &struct_name) const {
        auto it = struct_definitions_.find(struct_name);
        return (it != struct_definitions_.end()) ? &it->second : nullptr;
    }

    // typedef定義へのアクセス（sizeof演算子などから使用）
    const std::map<std::string, std::string> &get_typedef_map() const {
        return typedef_map;
    }

    std::string resolve_typedef(const std::string &type_name) const {
        auto it = typedef_map.find(type_name);
        return (it != typedef_map.end()) ? it->second : type_name;
    }

    // N次元配列アクセス用のヘルパー関数
    std::string extract_array_name(const ASTNode *node);
    std::vector<int64_t> extract_array_indices(const ASTNode *node);
    std::string extract_array_element_name(const ASTNode *node);

    // Priority 3: 変数ポインターから名前を取得するヘルパー
    std::string find_variable_name(const Variable *var);

    // ArrayManagerへのアクセス
    ArrayManager *get_array_manager() { return array_manager_.get(); }

    // DRY効率化: サービスクラスへのアクセサ
    ExpressionService *get_expression_service() {
        return expression_service_.get();
    }
    VariableAccessService *get_variable_access_service() {
        return variable_access_service_.get();
    }
    ArrayProcessingService *get_array_processing_service() {
        return array_processing_service_.get();
    }

    int64_t
    getMultidimensionalArrayElement(const Variable &var,
                                    const std::vector<int64_t> &indices);

    // Priority 3: 統一された配列要素アクセス (ArrayProcessingService経由)
    int64_t
    getMultidimensionalArrayElement(Variable &var,
                                    const std::vector<int64_t> &indices);
    void setMultidimensionalArrayElement(Variable &var,
                                         const std::vector<int64_t> &indices,
                                         int64_t value);
    void setMultidimensionalArrayElement(Variable &var,
                                         const std::vector<int64_t> &indices,
                                         double value);

    // Priority 3: 統一された文字列配列アクセス (ArrayProcessingService経由)
    std::string
    getMultidimensionalStringArrayElement(Variable &var,
                                          const std::vector<int64_t> &indices);
    void
    setMultidimensionalStringArrayElement(Variable &var,
                                          const std::vector<int64_t> &indices,
                                          const std::string &value);

    // self処理用ヘルパー関数 (InterfaceOperationsへ委譲)
    std::string get_self_receiver_path();
    void sync_self_to_receiver(const std::string &receiver_path);

    // v0.11.0: 実行時型解決システム - TypeContext管理
    void push_type_context(const TypeContext &ctx) {
        type_context_stack_.push_back(ctx);
    }
    void pop_type_context() {
        if (!type_context_stack_.empty()) {
            type_context_stack_.pop_back();
        }
    }

    // ポインタ要素型の登録と取得
    void register_pointer_element_type(void *ptr,
                                       const std::string &element_type) {
        if (debug_mode) {
            std::cerr << "[REGISTER_PTR] Registering 0x" << std::hex << ptr
                      << std::dec << " -> " << element_type << "\n";
        }
        pointer_element_types_[reinterpret_cast<uintptr_t>(ptr)] = element_type;
        if (debug_mode) {
            std::cerr << "[REGISTER_PTR] Registration successful, map size="
                      << pointer_element_types_.size() << "\n";
        }
    }

    std::string get_pointer_element_type(void *ptr) const {
        auto it = pointer_element_types_.find(reinterpret_cast<uintptr_t>(ptr));
        return (it != pointer_element_types_.end()) ? it->second : "";
    }
    const TypeContext *get_current_type_context() const {
        return type_context_stack_.empty() ? nullptr
                                           : &type_context_stack_.back();
    }
    // 型名を現在のコンテキストで解決
    std::string resolve_type_in_context(const std::string &type_name) const {
        auto ctx = get_current_type_context();
        return ctx ? ctx->resolve_complex_type(type_name) : type_name;
    }

    // v0.11.0 Phase 1a: メモリ管理演算子
    int64_t evaluate_new_expression(const ASTNode *node);
    int64_t evaluate_delete_expression(const ASTNode *node);
    int64_t evaluate_sizeof_expression(const ASTNode *node);

    // v0.12.0: Event Loop アクセス
    cb::EventLoop &get_event_loop();

    // v0.12.1: 現在実行中のタスクID管理
    void set_current_executing_task_id(int task_id) {
        current_executing_task_id_ = task_id;
    }
    int get_current_executing_task_id() const {
        return current_executing_task_id_;
    }
};
