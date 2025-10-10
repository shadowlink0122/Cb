// Variable Declaration Parser - 変数宣言解析を担当
// Phase 5-3-5: 変数宣言の専用パーサー
//
// このファイルは、変数宣言の解析を専門に担当します。
// DeclarationParserから分離して、ファイルサイズを管理します。
//
#include "variable_declaration_parser.h"
#include "../recursive_parser.h"
#include "src/common/debug.h"

VariableDeclarationParser::VariableDeclarationParser(RecursiveParser *parser)
    : parser_(parser) {}

/**
 * @brief 変数宣言を解析
 * @return 解析されたAST変数宣言ノード
 *
 * サポートする構文:
 * - 単純な変数: int x;
 * - 初期化付き: int x = 10;
 * - 複数宣言: int x = 1, y = 2, z = 3;
 * - 配列: int[5] arr;
 * - ポインタ: int* ptr;
 * - 参照: int& ref = x;
 * - const修飾子: const int x = 10;
 */
ASTNode *VariableDeclarationParser::parseVariableDeclaration() {
    std::string var_type = parser_->parseType();
    ParsedTypeInfo base_parsed_type = parser_->getLastParsedTypeInfo();
    var_type = base_parsed_type.full_type;

    if (debug_mode) {
        debug_log_line("[PARSER_TYPE_DEBUG] After parseType()");
        debug_log_line("  full_type: " + base_parsed_type.full_type);

        debug_log_line(std::string("  is_pointer: ") +
                       (base_parsed_type.is_pointer ? "true" : "false"));
        debug_log_line(std::string("  is_array: ") +
                       (base_parsed_type.is_array ? "true" : "false"));

        debug_log_line(
            "  base_type_info: " +
            std::to_string(static_cast<int>(base_parsed_type.base_type_info)));

        if (base_parsed_type.is_array) {
            debug_log_line("  array_info.base_type: " +
                           std::to_string(static_cast<int>(
                               base_parsed_type.array_info.base_type)));
        }
    }

    // ポインタ型の場合、型の直後のconstをチェック (int* const の場合)
    if (base_parsed_type.is_pointer && parser_->check(TokenType::TOK_CONST)) {
        base_parsed_type.is_pointer_const = true;
        parser_->advance();
    }

    // 変数名のリストを収集
    struct VariableInfo {
        std::string name;
        std::unique_ptr<ASTNode> init_expr;
        ArrayTypeInfo array_info;
        bool is_array;
        ParsedTypeInfo parsed_type;
        bool is_private;

        VariableInfo(const std::string &n, std::unique_ptr<ASTNode> expr,
                     const ArrayTypeInfo &arr_info, bool arr,
                     const ParsedTypeInfo &parsed, bool is_priv)
            : name(n), init_expr(std::move(expr)), array_info(arr_info),
              is_array(arr), parsed_type(parsed), is_private(is_priv) {}
    };
    std::vector<VariableInfo> variables;

    auto dimension_to_string = [](const ArrayDimension &dim) {
        if (!dim.size_expr.empty()) {
            return std::string("[") + dim.size_expr + "]";
        }
        if (!dim.is_dynamic && dim.size >= 0) {
            return std::string("[") + std::to_string(dim.size) + "]";
        }
        return std::string("[]");
    };

    do {
        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->error("Expected variable name");
        }

        std::string var_name = parser_->advance().value;
        std::unique_ptr<ASTNode> init_expr = nullptr;
        ParsedTypeInfo var_parsed = base_parsed_type;
        ArrayTypeInfo array_info = var_parsed.array_info;
        bool is_array = var_parsed.is_array;

        // 配列の角括弧をチェック
        if (parser_->check(TokenType::TOK_LBRACKET)) {
            is_array = true;
            if (array_info.base_type == TYPE_UNKNOWN) {
                array_info.base_type = var_parsed.is_pointer
                                           ? TYPE_POINTER
                                           : var_parsed.base_type_info;
            }

            while (parser_->check(TokenType::TOK_LBRACKET)) {
                parser_->advance();

                if (parser_->check(TokenType::TOK_NUMBER)) {
                    int size = std::stoi(parser_->advance().value);
                    array_info.dimensions.emplace_back(size, false, "");
                } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                    std::string size_name = parser_->advance().value;
                    array_info.dimensions.emplace_back(-1, true, size_name);
                } else {
                    array_info.dimensions.emplace_back(-1, true, "");
                }

                parser_->consume(TokenType::TOK_RBRACKET, "Expected ']'");
            }
        }

        if (is_array && array_info.base_type == TYPE_UNKNOWN) {
            array_info.base_type = var_parsed.is_pointer
                                       ? TYPE_POINTER
                                       : var_parsed.base_type_info;
        }

        // デバッグ出力: 配列の基本型を確認
        if (is_array && debug_mode) {
            debug_log_line("[PARSER_ARRAY_DEBUG] Variable: " + var_name);
            debug_log_line(std::string("  is_pointer: ") +
                           (var_parsed.is_pointer ? "true" : "false"));
            debug_log_line(
                "  base_type_info: " +
                std::to_string(static_cast<int>(var_parsed.base_type_info)));
            debug_log_line(
                "  array_info.base_type: " +
                std::to_string(static_cast<int>(array_info.base_type)));
            debug_log_line("  TYPE_POINTER: " +
                           std::to_string(static_cast<int>(TYPE_POINTER)));
        }

        var_parsed.is_array = is_array;
        var_parsed.array_info = array_info;

        std::string combined_full_type = base_parsed_type.full_type;
        if (is_array) {
            size_t base_dims = base_parsed_type.array_info.dimensions.size();
            size_t total_dims = array_info.dimensions.size();
            if (total_dims > base_dims) {
                for (size_t idx = base_dims; idx < total_dims; ++idx) {
                    combined_full_type +=
                        dimension_to_string(array_info.dimensions[idx]);
                }
            }
        }
        var_parsed.full_type = combined_full_type;

        if (parser_->match(TokenType::TOK_ASSIGN)) {
            init_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
        }

        variables.emplace_back(var_name, std::move(init_expr), array_info,
                               is_array, var_parsed, false);

    } while (parser_->match(TokenType::TOK_COMMA));

    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");

    // 単一変数の場合は従来通りAST_VAR_DECL、複数の場合はAST_MULTIPLE_VAR_DECL
    if (variables.size() == 1) {
        VariableInfo &var_info = variables[0];
        const ParsedTypeInfo &parsed = var_info.parsed_type;

        ASTNode *node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        node->name = var_info.name;
        node->type_name = parsed.full_type;
        node->original_type_name = parsed.original_type.empty()
                                       ? parsed.full_type
                                       : parsed.original_type;

        TypeInfo declared_type = parser_->resolveParsedTypeInfo(parsed);
        node->type_info = declared_type;

        node->is_pointer = parsed.is_pointer;
        node->pointer_depth = parsed.pointer_depth;
        node->pointer_base_type_name = parsed.base_type;
        node->pointer_base_type = parsed.base_type_info;
        node->is_reference = parsed.is_reference;
        node->is_unsigned = parsed.is_unsigned;
        node->is_pointer_const_qualifier = parsed.is_pointer_const;
        node->is_pointee_const_qualifier = parsed.is_const && parsed.is_pointer;

        if (parsed.is_array) {
            node->array_type_info = parsed.array_info;
            node->is_array = true;
        }

        if (var_info.init_expr) {
            node->init_expr = std::move(var_info.init_expr);
        }

        return node;
    } else {
        // 複数変数宣言の場合
        ASTNode *node = new ASTNode(ASTNodeType::AST_MULTIPLE_VAR_DECL);
        node->type_name = base_parsed_type.full_type;
        node->original_type_name = base_parsed_type.original_type.empty()
                                       ? base_parsed_type.full_type
                                       : base_parsed_type.original_type;
        node->type_info = parser_->resolveParsedTypeInfo(base_parsed_type);
        node->is_pointer = base_parsed_type.is_pointer;
        node->pointer_depth = base_parsed_type.pointer_depth;
        node->pointer_base_type_name = base_parsed_type.base_type;
        node->pointer_base_type = base_parsed_type.base_type_info;
        node->is_reference = base_parsed_type.is_reference;
        node->is_unsigned = base_parsed_type.is_unsigned;
        node->is_pointer_const_qualifier = base_parsed_type.is_pointer_const;
        node->is_pointee_const_qualifier =
            base_parsed_type.is_const && base_parsed_type.is_pointer;
        if (base_parsed_type.is_array) {
            node->array_type_info = base_parsed_type.array_info;
            node->is_array = true;
        }

        // 各変数を子ノードとして追加
        for (auto &var : variables) {
            ParsedTypeInfo &parsed = var.parsed_type;
            ASTNode *var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
            var_node->name = var.name;
            var_node->type_name = parsed.full_type;
            var_node->original_type_name = parsed.original_type.empty()
                                               ? parsed.full_type
                                               : parsed.original_type;
            var_node->type_info = parser_->resolveParsedTypeInfo(parsed);
            var_node->is_pointer = parsed.is_pointer;
            var_node->pointer_depth = parsed.pointer_depth;
            var_node->pointer_base_type_name = parsed.base_type;
            var_node->pointer_base_type = parsed.base_type_info;
            var_node->is_reference = parsed.is_reference;
            var_node->is_unsigned = parsed.is_unsigned;
            var_node->is_pointer_const_qualifier = parsed.is_pointer_const;
            var_node->is_pointee_const_qualifier =
                parsed.is_const && parsed.is_pointer;

            if (parsed.is_array) {
                var_node->array_type_info = parsed.array_info;
                var_node->is_array = true;
            }

            if (var.init_expr) {
                var_node->init_expr = std::move(var.init_expr);
            }

            node->children.push_back(std::unique_ptr<ASTNode>(var_node));
        }

        return node;
    }
}
