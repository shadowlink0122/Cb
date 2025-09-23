#include "parser_utils.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

// パーサーが使用するグローバル変数（宣言のみ）
extern "C" {
extern const char *current_filename;
}
extern std::vector<std::string> file_lines;

// 現在の宣言で使用されている型情報（型チェック用）
static TypeInfo current_declared_type = TYPE_INT;

// 文字列リテラルのパース
static std::string parse_string_literal(const char *raw) {
    int len = std::strlen(raw);
    if (len < 2 || raw[0] != '"' || raw[len - 1] != '"') {
        return std::string(raw);
    }

    std::string result;
    for (int i = 1; i < len - 1; ++i) {
        if (raw[i] == '\\' && i + 1 < len - 1) {
            switch (raw[i + 1]) {
            case '"':
                result.push_back('"');
                ++i;
                break;
            case '\\':
                result.push_back('\\');
                ++i;
                break;
            case 'n':
                result.push_back('\n');
                ++i;
                break;
            case 't':
                result.push_back('\t');
                ++i;
                break;
            case 'r':
                result.push_back('\r');
                ++i;
                break;
            default:
                result.push_back(raw[i]);
                break;
            }
        } else {
            result.push_back(raw[i]);
        }
    }
    return result;
}

// ノード作成関数の実装
extern "C" {

ASTNode *create_stmt_list() {
    debug_msg(DebugMsgId::NODE_CREATE_STMTLIST);
    return new ASTNode(ASTNodeType::AST_STMT_LIST);
}

ASTNode *create_type_node(TypeInfo type) {
    debug_msg(DebugMsgId::NODE_CREATE_TYPESPEC, type);
    auto node = new ASTNode(ASTNodeType::AST_TYPE_SPEC);
    node->type_info = type;

    // 現在の型として設定
    set_current_type(type);

    return node;
}

ASTNode *create_storage_spec(bool is_static, bool is_const) {
    auto node = new ASTNode(ASTNodeType::AST_STORAGE_SPEC);
    node->is_static = is_static;
    node->is_const = is_const;
    return node;
}

ASTNode *create_var_decl(const char *name) {
    debug_msg(DebugMsgId::NODE_CREATE_VAR_DECL, name);
    auto node = new ASTNode(ASTNodeType::AST_VAR_DECL);
    node->name = std::string(name);
    return node;
}

ASTNode *create_var_init(const char *name, ASTNode *init_expr) {
    debug_msg(DebugMsgId::NODE_CREATE_ASSIGN, name);
    auto node = new ASTNode(ASTNodeType::AST_ASSIGN);
    node->name = std::string(name);
    node->right = std::unique_ptr<ASTNode>(init_expr);
    return node;
}

ASTNode *create_array_decl(const char *name, ASTNode *size_expr) {
    debug_msg(DebugMsgId::NODE_CREATE_ARRAY_DECL, name);
    auto node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
    node->name = std::string(name);
    node->array_size_expr = std::unique_ptr<ASTNode>(size_expr);
    return node;
}

ASTNode *create_array_init(const char *name, ASTNode *init_list) {
    debug_msg(DebugMsgId::ARRAY_INIT_CALLED, name);

    // 現在宣言されている型を使用して型チェックを実行
    TypeInfo expected_type = get_current_type();

    // 型が不正な場合はデフォルトを使用
    if (expected_type < TYPE_VOID || expected_type > TYPE_BOOL) {
        expected_type = TYPE_INT;
        debug_msg(DebugMsgId::CURRENT_TYPE_SET, (int)expected_type);
    }

    return create_array_init_with_type(name, expected_type, init_list);
}

ASTNode *create_array_init_with_type(const char *name, TypeInfo element_type,
                                     ASTNode *init_list) {
    debug_msg(DebugMsgId::ARRAY_INIT_WITH_TYPE_CALLED, name, (int)element_type);
    auto node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
    node->name = std::string(name);
    node->type_info = element_type;

    if (init_list && init_list->children.size() > 0) {
        debug_msg(DebugMsgId::ARRAY_INIT_ELEMENTS, init_list->children.size());

        // 各要素の型をチェック
        for (size_t i = 0; i < init_list->children.size(); i++) {
            ASTNode *element = init_list->children[i].get();
            if (!element)
                continue; // nullポインタチェック

            TypeInfo actual_type = TYPE_VOID;

            // 要素の型を判定
            if (element->node_type == ASTNodeType::AST_NUMBER) {
                actual_type = TYPE_INT; // 数値リテラルはintとして扱う
            } else if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
                actual_type = TYPE_STRING;
            }

            // 型が不一致の場合エラー
            if (actual_type != TYPE_VOID && actual_type != element_type) {
                const char *expected_str = type_info_to_string(element_type);
                const char *actual_str = type_info_to_string(actual_type);
                debug_msg(DebugMsgId::TYPE_MISMATCH_ARRAY_INIT, i, expected_str,
                          actual_str);

                // エラーメッセージを標準エラーに出力
                std::cerr << "Error: Array '" << name << "' element " << i
                          << ": " << expected_str << " type expected but "
                          << actual_str << " type provided" << std::endl;
                exit(1);
            }
        }

        node->children = std::move(init_list->children);
        node->array_size = node->children.size();
        delete init_list;
    } else if (init_list) {
        // 空配列の場合
        node->array_size = 0;
        delete init_list;
    } else {
        // init_listがnullの場合
        node->array_size = 0;
    }
    debug_msg(DebugMsgId::ARRAY_INIT_WITH_TYPE_COMPLETED);
    return node;
}

void set_current_type(TypeInfo type) {
    current_declared_type = type;
    debug_msg(DebugMsgId::CURRENT_TYPE_SET, (int)type);
}

TypeInfo get_current_type() { return current_declared_type; }

ASTNode *create_array_init_with_size(const char *name, ASTNode *size_expr,
                                     ASTNode *init_list) {
    auto node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
    node->name = std::string(name);
    node->array_size_expr = std::unique_ptr<ASTNode>(size_expr);
    if (init_list) {
        node->children = std::move(init_list->children);
        // サイズ式と初期化リストの両方がある場合は、明示的なサイズを優先
        delete init_list;
    }
    return node;
}

ASTNode *create_function_def(const char *name, ASTNode *decl_spec,
                             ASTNode *unused, ASTNode *params, ASTNode *body) {
    debug_msg(DebugMsgId::NODE_CREATE_FUNC_DECL, name);
    auto node = new ASTNode(ASTNodeType::AST_FUNC_DECL);
    node->name = std::string(name);

    if (decl_spec) {
        // 関数の型情報はデバッグメッセージとして出力しない（詳細すぎるため）
        node->is_static = decl_spec->is_static;
        node->is_const = decl_spec->is_const;
        node->type_info = decl_spec->type_info;
    }

    if (params) {
        debug_msg(DebugMsgId::PARAM_LIST_START);
        debug_msg(DebugMsgId::PARAM_LIST_SIZE, params->parameters.size());
        node->parameters = std::move(params->parameters);
        debug_msg(DebugMsgId::PARAM_LIST_COMPLETE);
        delete params;
        debug_msg(DebugMsgId::PARAM_LIST_DELETE);
    } else {
        debug_msg(DebugMsgId::PARAM_LIST_NONE);
    }

    debug_msg(DebugMsgId::FUNC_BODY_START);
    if (body) {
        debug_msg(DebugMsgId::FUNC_BODY_EXISTS);
        node->body = std::unique_ptr<ASTNode>(body);
        debug_msg(DebugMsgId::FUNC_BODY_SET_COMPLETE);
    } else {
        debug_msg(DebugMsgId::FUNC_BODY_NONE);
    }
    debug_msg(DebugMsgId::FUNC_DEF_COMPLETE);
    return node;
}

ASTNode *create_param_list() {
    return new ASTNode(ASTNodeType::AST_STMT_LIST); // 一時的なリスト
}

ASTNode *create_parameter(ASTNode *type, const char *name) {
    auto node = new ASTNode(ASTNodeType::AST_PARAM_DECL);
    node->name = std::string(name);
    node->type_info = type->type_info;
    return node;
}

ASTNode *create_print_stmt(ASTNode *expr) {
    auto node = new ASTNode(ASTNodeType::AST_PRINT_STMT);
    node->left = std::unique_ptr<ASTNode>(expr);
    return node;
}

ASTNode *create_println_stmt(ASTNode *expr) {
    auto node = new ASTNode(ASTNodeType::AST_PRINTLN_STMT);
    node->left = std::unique_ptr<ASTNode>(expr);
    return node;
}

ASTNode *create_println_empty() {
    auto node = new ASTNode(ASTNodeType::AST_PRINTLN_EMPTY);
    return node;
}

ASTNode *create_printf_stmt(ASTNode *format_str, ASTNode *arg_list) {
    auto node = new ASTNode(ASTNodeType::AST_PRINTF_STMT);
    node->left = std::unique_ptr<ASTNode>(format_str); // フォーマット文字列
    node->right = std::unique_ptr<ASTNode>(arg_list); // 引数リスト
    return node;
}

ASTNode *create_printlnf_stmt(ASTNode *format_str, ASTNode *arg_list) {
    auto node = new ASTNode(ASTNodeType::AST_PRINTLNF_STMT);
    node->left = std::unique_ptr<ASTNode>(format_str); // フォーマット文字列
    node->right = std::unique_ptr<ASTNode>(arg_list); // 引数リスト
    return node;
}

ASTNode *create_if_stmt(ASTNode *cond, ASTNode *then_stmt, ASTNode *else_stmt) {
    auto node = new ASTNode(ASTNodeType::AST_IF_STMT);
    node->condition = std::unique_ptr<ASTNode>(cond);
    node->left = std::unique_ptr<ASTNode>(then_stmt);
    if (else_stmt) {
        node->right = std::unique_ptr<ASTNode>(else_stmt);
    }
    return node;
}

ASTNode *create_while_stmt(ASTNode *cond, ASTNode *body) {
    auto node = new ASTNode(ASTNodeType::AST_WHILE_STMT);
    node->condition = std::unique_ptr<ASTNode>(cond);
    node->body = std::unique_ptr<ASTNode>(body);
    return node;
}

ASTNode *create_for_stmt(ASTNode *init, ASTNode *cond, ASTNode *update,
                         ASTNode *body) {
    auto node = new ASTNode(ASTNodeType::AST_FOR_STMT);
    node->init_expr = std::unique_ptr<ASTNode>(init);
    node->condition = std::unique_ptr<ASTNode>(cond);
    node->update_expr = std::unique_ptr<ASTNode>(update);
    node->body = std::unique_ptr<ASTNode>(body);
    return node;
}

ASTNode *create_for_stmt_with_decl(ASTNode *decl, ASTNode *cond,
                                   ASTNode *update, ASTNode *body) {
    auto node = new ASTNode(ASTNodeType::AST_FOR_STMT);
    node->init_expr = std::unique_ptr<ASTNode>(decl);
    node->condition = std::unique_ptr<ASTNode>(cond);
    node->update_expr = std::unique_ptr<ASTNode>(update);
    node->body = std::unique_ptr<ASTNode>(body);
    return node;
}

ASTNode *create_return_stmt(ASTNode *expr) {
    auto node = new ASTNode(ASTNodeType::AST_RETURN_STMT);
    if (expr) {
        node->left = std::unique_ptr<ASTNode>(expr);
    }
    return node;
}

ASTNode *create_break_stmt(ASTNode *expr) {
    auto node = new ASTNode(ASTNodeType::AST_BREAK_STMT);
    if (expr) {
        node->left = std::unique_ptr<ASTNode>(expr);
    }
    return node;
}

ASTNode *create_assign_expr(const char *name, ASTNode *expr) {
    auto node = new ASTNode(ASTNodeType::AST_ASSIGN);
    node->name = std::string(name);
    node->right = std::unique_ptr<ASTNode>(expr);
    return node;
}

ASTNode *create_array_assign(const char *name, ASTNode *index, ASTNode *expr) {
    auto node = new ASTNode(ASTNodeType::AST_ASSIGN);
    node->name = std::string(name);

    // 配列参照ノードを作成
    auto array_ref = new ASTNode(ASTNodeType::AST_ARRAY_REF);
    array_ref->name = std::string(name);
    array_ref->array_index = std::unique_ptr<ASTNode>(index);

    node->left = std::unique_ptr<ASTNode>(array_ref);
    node->right = std::unique_ptr<ASTNode>(expr);
    return node;
}

ASTNode *create_compound_assign(const char *name, const char *op,
                                ASTNode *expr) {
    // a += b を a = a + b に変換
    auto assign = new ASTNode(ASTNodeType::AST_ASSIGN);
    assign->name = std::string(name);

    auto binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
    binop->op = std::string(op);
    binop->left = std::unique_ptr<ASTNode>(create_var_ref(name));
    binop->right = std::unique_ptr<ASTNode>(expr);

    assign->right = std::unique_ptr<ASTNode>(binop);
    return assign;
}

ASTNode *create_binop(const char *op, ASTNode *left, ASTNode *right) {
    auto node = new ASTNode(ASTNodeType::AST_BINARY_OP);
    node->op = std::string(op);
    node->left = std::unique_ptr<ASTNode>(left);
    node->right = std::unique_ptr<ASTNode>(right);

    // 型推論
    TypeInfo ltype = left ? left->type_info : TYPE_INT;
    TypeInfo rtype = right ? right->type_info : TYPE_INT;

    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
        strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 ||
        strcmp(op, ">=") == 0 || strcmp(op, "||") == 0 ||
        strcmp(op, "&&") == 0) {
        node->type_info = TYPE_BOOL;
    } else {
        node->type_info = (ltype > rtype) ? ltype : rtype;
    }

    return node;
}

ASTNode *create_unary(const char *op, ASTNode *operand) {
    auto node = new ASTNode(ASTNodeType::AST_UNARY_OP);
    node->op = std::string(op);
    node->left = std::unique_ptr<ASTNode>(operand);

    if (strcmp(op, "!") == 0) {
        node->type_info = TYPE_BOOL;
    } else {
        node->type_info = operand ? operand->type_info : TYPE_INT;
    }

    return node;
}

ASTNode *create_pre_incdec(const char *op, const char *name) {
    auto node = new ASTNode(ASTNodeType::AST_PRE_INCDEC);
    node->op = std::string(op);
    node->name = std::string(name);
    return node;
}

ASTNode *create_post_incdec(const char *op, const char *name) {
    auto node = new ASTNode(ASTNodeType::AST_POST_INCDEC);
    node->op = std::string(op);
    node->name = std::string(name);
    return node;
}

ASTNode *create_array_ref(const char *name, ASTNode *index) {
    auto node = new ASTNode(ASTNodeType::AST_ARRAY_REF);
    node->name = std::string(name);
    node->array_index = std::unique_ptr<ASTNode>(index);
    return node;
}

ASTNode *create_func_call(const char *name, ASTNode *args) {
    auto node = new ASTNode(ASTNodeType::AST_FUNC_CALL);
    node->name = std::string(name);
    if (args) {
        node->arguments = std::move(args->arguments);
        delete args;
    }
    return node;
}

ASTNode *create_var_ref(const char *name) {
    auto node = new ASTNode(ASTNodeType::AST_VARIABLE);
    node->name = std::string(name);
    return node;
}

ASTNode *create_number(int64_t value, TypeInfo type) {
    auto node = new ASTNode(ASTNodeType::AST_NUMBER);
    node->int_value = value;
    node->type_info = type;

    // 範囲チェック
    bool out_of_range = false;
    switch (type) {
    case TYPE_TINY:
        if (value < -128 || value > 127)
            out_of_range = true;
        break;
    case TYPE_SHORT:
        if (value < -32768 || value > 32767)
            out_of_range = true;
        break;
    case TYPE_INT:
        if (value < -2147483648LL || value > 2147483647LL)
            out_of_range = true;
        break;
    case TYPE_LONG:
        // long型は基本的に範囲チェック不要（int64_tの範囲内）
        break;
    case TYPE_BOOL:
        node->int_value = (value != 0) ? 1 : 0;
        break;
    default:
        break;
    }

    if (out_of_range) {
        yyerror("型の範囲外の値を代入しようとしました");
        delete node;
        // パーサーを強制終了させる
        std::exit(1);
        return nullptr;
    }

    return node;
}

ASTNode *create_string_literal(const char *str) {
    auto node = new ASTNode(ASTNodeType::AST_STRING_LITERAL);
    node->str_value = parse_string_literal(str);
    node->type_info = TYPE_STRING;
    return node;
}

ASTNode *create_arg_list() {
    return new ASTNode(ASTNodeType::AST_STMT_LIST); // 一時的なリスト
}

ASTNode *create_array_literal(ASTNode *elements) {
    debug_msg(DebugMsgId::ARRAY_LITERAL_CALLED);
    auto node = new ASTNode(ASTNodeType::AST_STMT_LIST); // 配列リテラル用
    if (elements) {
        debug_msg(DebugMsgId::ARRAY_LITERAL_ELEMENTS,
                  elements->arguments.size());
        // argumentsをchildrenに移動
        for (auto &arg : elements->arguments) {
            node->children.push_back(std::move(arg));
        }
        delete elements;
    }
    debug_msg(DebugMsgId::ARRAY_LITERAL_COMPLETED);
    return node;
}

// 多次元配列用関数群
ASTNode *create_multidim_array_decl(const char *name, ASTNode *dimensions) {
    auto node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
    node->name = std::string(name);

    if (dimensions && !dimensions->array_dimensions.empty()) {
        node->array_dimensions = std::move(dimensions->array_dimensions);
        delete dimensions;
    }

    return node;
}

ASTNode *create_multidim_array_init(const char *name, ASTNode *dimensions,
                                    ASTNode *init) {
    auto node = create_multidim_array_decl(name, dimensions);

    if (init) {
        node->children = std::move(init->children);
        delete init;
    }

    return node;
}

ASTNode *create_multidim_array_ref(const char *name, ASTNode *indices) {
    auto node = new ASTNode(ASTNodeType::AST_ARRAY_REF);
    node->name = std::string(name);

    if (indices && !indices->array_indices.empty()) {
        node->array_indices = std::move(indices->array_indices);
        delete indices;
    }

    return node;
}

ASTNode *create_multidim_array_assign(const char *name, ASTNode *indices,
                                      ASTNode *expr) {
    auto node = new ASTNode(ASTNodeType::AST_ASSIGN);
    node->name = std::string(name);

    auto array_ref = create_multidim_array_ref(name, indices);
    node->left = std::unique_ptr<ASTNode>(array_ref);
    node->right = std::unique_ptr<ASTNode>(expr);

    return node;
}

ASTNode *create_dimension_list() {
    auto node = new ASTNode(ASTNodeType::AST_STMT_LIST);
    return node;
}

ASTNode *create_index_list() {
    auto node = new ASTNode(ASTNodeType::AST_STMT_LIST);
    return node;
}

ASTNode *create_nested_init_list() {
    auto node = new ASTNode(ASTNodeType::AST_STMT_LIST);
    return node;
}

void add_dimension(ASTNode *list, ASTNode *size_expr) {
    if (list) {
        list->array_dimensions.push_back(std::unique_ptr<ASTNode>(size_expr));
    }
}

void add_index(ASTNode *list, ASTNode *index_expr) {
    if (list) {
        list->array_indices.push_back(std::unique_ptr<ASTNode>(index_expr));
    }
}

void add_nested_initializer(ASTNode *list, ASTNode *init) {
    if (list && init) {
        list->children.push_back(std::unique_ptr<ASTNode>(init));
    }
}

// typedef用関数群
ASTNode *create_typedef_decl(const char *alias_name, ASTNode *base_type,
                             ASTNode *unused) {
    auto node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
    node->name = std::string(alias_name);

    if (base_type) {
        node->type_info = base_type->type_info;
        node->type_name = type_info_to_string(base_type->type_info);
        delete base_type;
    }

    return node;
}

ASTNode *create_typedef_array_decl(const char *alias_name, ASTNode *base_type,
                                   ASTNode *dimensions) {
    auto node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
    node->name = std::string(alias_name);

    if (base_type) {
        node->type_info = base_type->type_info;
        delete base_type;
    }

    if (dimensions && !dimensions->array_dimensions.empty()) {
        node->array_dimensions = std::move(dimensions->array_dimensions);
        delete dimensions;
    }

    return node;
}

ASTNode *create_typedef_type(const char *type_name) {
    auto node = new ASTNode(ASTNodeType::AST_TYPE_SPEC);
    node->type_name = std::string(type_name);

    // エイリアス解決を試行
    TypeInfo resolved_type = parse_type_from_string(type_name);
    node->type_info = resolved_type;

    return node;
}

ASTNode *create_typedef_array_var(const char *name, ASTNode *size_expr) {
    auto node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
    node->name = std::string(name);
    node->array_size_expr = std::unique_ptr<ASTNode>(size_expr);

    return node;
}

// ユーティリティ関数の実装
void add_statement(ASTNode *list, ASTNode *stmt) {
    if (list && stmt) {
        list->statements.push_back(std::unique_ptr<ASTNode>(stmt));
    }
}

void add_parameter(ASTNode *list, ASTNode *param) {
    if (list && param) {
        list->parameters.push_back(std::unique_ptr<ASTNode>(param));
    }
}

void add_argument(ASTNode *list, ASTNode *arg) {
    if (list && arg) {
        list->arguments.push_back(std::unique_ptr<ASTNode>(arg));
    }
}

void set_declaration_attributes(ASTNode *decl, ASTNode *decl_spec,
                                ASTNode *unused) {
    if (!decl)
        return;

    if (decl_spec) {
        decl->is_static = decl_spec->is_static;
        decl->is_const = decl_spec->is_const;
        decl->type_info = decl_spec->type_info;

        // 配列の場合、要素型も設定
        if (decl->node_type == ASTNodeType::AST_ARRAY_DECL &&
            decl_spec->type_info < TYPE_ARRAY_BASE) {
            // elem_type_info に相当する処理が必要な場合はここで
        }
    }
}

TypeInfo get_type_info(ASTNode *type_node) {
    return type_node ? type_node->type_info : TYPE_INT;
}

void delete_node(ASTNode *node) {
    delete node; // unique_ptrが自動的にクリーンアップ
}

// エラー処理
void yyerror(const char *s) {
    extern int yylineno;

    debug_msg(DebugMsgId::PARSER_ERROR);
    std::cerr << " ";
    if (current_filename) {
        std::cerr << "(" << current_filename << ":" << yylineno << ")";
    } else {
        std::cerr << "(行 " << yylineno << ")";
    }
    std::cerr << ": " << s << std::endl;

    // 該当行の内容を表示
    if (yylineno > 0 && yylineno <= static_cast<int>(file_lines.size())) {
        // 行番号の桁数を計算
        int line_num_width = std::to_string(yylineno).length();

        // 行内容を表示
        std::cerr << "    " << yylineno << " | " << file_lines[yylineno - 1]
                  << std::endl;

        // カレット(^)で問題箇所を指示
        // "    " (4文字) + 行番号 + " | " (3文字) の分だけインデント
        std::cerr << "    ";
        for (int i = 0; i < line_num_width; ++i) {
            std::cerr << " ";
        }
        std::cerr << " | ";

        for (size_t i = 0; i < file_lines[yylineno - 1].length(); ++i) {
            if (file_lines[yylineno - 1][i] == '\t') {
                std::cerr << '\t';
            } else {
                std::cerr << ' ';
            }
        }
        std::cerr << "^" << std::endl;
    }
}

ASTNode *create_decl_spec(ASTNode *storage_class, ASTNode *type_qualifier,
                          ASTNode *type_spec) {
    auto node = new ASTNode(ASTNodeType::AST_TYPE_SPEC);

    // type_specifierの情報をコピー
    if (type_spec) {
        node->type_info = type_spec->type_info;
    } else {
        node->type_info = TYPE_INT; // デフォルト
    }

    // storage_class の情報をコピー
    if (storage_class) {
        node->is_static = storage_class->is_static;
    } else {
        node->is_static = false;
    }

    // type_qualifier の情報をコピー
    if (type_qualifier) {
        node->is_const = type_qualifier->is_const;
    } else {
        node->is_const = false;
    }

    return node;
}
}
