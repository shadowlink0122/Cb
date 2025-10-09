// Declaration Parser - 宣言解析を担当
// Phase 2-3: RecursiveParserへの委譲実装 + ドキュメント化
//
// このファイルは、変数、関数、typedefの宣言解析を担当します。
//
// 【サポートする宣言】:
// 1. 変数宣言: int x = 10;
// 2. 配列宣言: int[5] arr = [1, 2, 3, 4, 5];
// 3. 関数宣言: int add(int a, int b) { return a + b; }
// 4. typedef宣言: typedef MyInt = int;
// 5. 関数ポインタtypedef: typedef Callback = int(int, int);
//
#include "declaration_parser.h"
#include "../recursive_parser.h"
#include "src/common/debug.h"
#include "variable_declaration_parser.h"
#include <algorithm>

DeclarationParser::DeclarationParser(RecursiveParser *parser)
    : parser_(parser),
      variable_declaration_parser_(
          std::make_unique<VariableDeclarationParser>(parser)) {}

// ========================================
// 変数宣言
// ========================================

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
ASTNode *DeclarationParser::parseVariableDeclaration() {
    return variable_declaration_parser_->parseVariableDeclaration();
}

/**
 * @brief typedef付き変数宣言を解析
 * @return 解析されたAST変数宣言ノード
 *
 * 例: MyInt x = 10; (MyIntはtypedef)
 */
ASTNode *DeclarationParser::parseTypedefVariableDeclaration() {
    // typedef型名を取得
    std::string typedef_name = parser_->advance().value;
    std::string resolved_type = parser_->resolveTypedefChain(typedef_name);

    // ポインタ深度をチェック
    int pointer_depth = 0;
    while (parser_->check(TokenType::TOK_MUL)) {
        pointer_depth++;
        parser_->advance();
    }

    // 参照チェック
    bool is_reference = false;
    if (parser_->check(TokenType::TOK_BIT_AND)) {
        is_reference = true;
        parser_->advance();
    }

    // 変数名を取得
    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected variable name after typedef type");
        return nullptr;
    }

    std::string var_name = parser_->advance().value;

    // 変数宣言ノードを作成
    ASTNode *node = new ASTNode(ASTNodeType::AST_VAR_DECL);
    node->name = var_name;
    node->type_name = typedef_name; // 元のtypedef名を保持

    // ポインタと参照の情報を設定
    if (pointer_depth > 0) {
        node->is_pointer = true;
        node->pointer_depth = pointer_depth;
        node->pointer_base_type_name = typedef_name;
        node->type_info = TYPE_POINTER;

        // typedefの解決済み型を取得
        TypeInfo base_type_info = TYPE_UNKNOWN;
        if (!resolved_type.empty()) {
            // union型の場合
            if (parser_->union_definitions_.find(resolved_type) !=
                parser_->union_definitions_.end()) {
                base_type_info = TYPE_UNION;
            } else {
                base_type_info = parser_->getTypeInfoFromString(resolved_type);
            }
        }
        node->pointer_base_type = base_type_info;

        // ポインタの場合、type_nameに*を追加
        for (int i = 0; i < pointer_depth; i++) {
            node->type_name += "*";
        }
    }

    if (is_reference) {
        node->is_reference = true;
        node->type_name += "&";
    }

    // 実際の型から型情報を設定
    if (resolved_type.find("[") != std::string::npos) {
        // 配列型の場合
        std::string base_type =
            resolved_type.substr(0, resolved_type.find("["));
        node->type_info = parser_->getTypeInfoFromString(base_type);

        // 配列サイズ情報を解析
        std::vector<std::string> array_sizes;
        std::string remaining = resolved_type;
        size_t pos = 0;
        while ((pos = remaining.find("[", pos)) != std::string::npos) {
            size_t end_pos = remaining.find("]", pos);
            if (end_pos != std::string::npos) {
                std::string size = remaining.substr(pos + 1, end_pos - pos - 1);
                array_sizes.push_back(size);
                pos = end_pos + 1;
            } else {
                break;
            }
        }

        // ArrayTypeInfoを構築
        std::vector<ArrayDimension> dimensions;
        for (const auto &size : array_sizes) {
            if (!size.empty() &&
                std::all_of(size.begin(), size.end(), ::isdigit)) {
                int dim_size = std::stoi(size);
                dimensions.push_back(ArrayDimension(dim_size, false));
            } else if (!size.empty()) {
                // 定数識別子の可能性があるので、解決を試みる
                // 現在は動的配列として扱うが、後でランタイムで解決される
                dimensions.push_back(ArrayDimension(-1, true, size));
            } else {
                dimensions.push_back(ArrayDimension(-1, true));
            }
        }
        node->array_type_info = ArrayTypeInfo(node->type_info, dimensions);

        // デバッグ出力
        if (parser_->debug_mode_) {
            std::cerr << "DEBUG: Parser setting array_type_info for "
                      << node->name
                      << " with base_type=" << node->array_type_info.base_type
                      << std::endl;
        }
    } else {
        // 基本型の場合
        node->type_info = parser_->getTypeInfoFromString(resolved_type);
    }

    // 初期化式のチェック
    if (parser_->check(TokenType::TOK_ASSIGN)) {
        parser_->advance(); // consume '='

        if (parser_->check(TokenType::TOK_LBRACE)) {
            // 構造体リテラル初期化
            node->init_expr =
                std::unique_ptr<ASTNode>(parser_->parseStructLiteral());
        } else if (parser_->check(TokenType::TOK_LBRACKET)) {
            // 配列リテラル初期化
            parser_->advance(); // consume '['

            ASTNode *array_literal =
                new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);
            while (!parser_->check(TokenType::TOK_RBRACKET) &&
                   !parser_->isAtEnd()) {
                ASTNode *element = parser_->parseExpression();
                array_literal->arguments.push_back(
                    std::unique_ptr<ASTNode>(element));

                if (parser_->check(TokenType::TOK_COMMA)) {
                    parser_->advance(); // consume ','
                } else if (!parser_->check(TokenType::TOK_RBRACKET)) {
                    parser_->error("Expected ',' or ']' in array literal");
                    return nullptr;
                }
            }

            parser_->consume(TokenType::TOK_RBRACKET,
                             "Expected ']' after array literal");
            node->init_expr = std::unique_ptr<ASTNode>(array_literal);
        } else {
            // 通常の式
            ASTNode *expr = parser_->parseExpression();
            node->init_expr = std::unique_ptr<ASTNode>(expr);
        }
    }

    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after variable declaration");
    return node;
}

// ========================================
// 関数宣言
// ========================================

/**
 * @brief 関数宣言を解析
 * @return 解析されたAST関数宣言ノード
 *
 * 構文: return_type function_name(param1, param2, ...) { body }
 *
 * サポートする機能:
 * - 戻り値の型指定（void含む）
 * - パラメータリスト（値渡し、参照渡し、ポインタ）
 * - 関数本体の解析
 * - 再帰関数
 */
ASTNode *DeclarationParser::parseFunctionDeclaration() {
    // この時点で既にintとidentifierとlparenが確認済み（または解析開始状態）
    // 戻り値の型を解析（まだ消費されていない場合）
    std::string return_type;
    if (parser_->check(TokenType::TOK_INT)) {
        return_type = "int";
        parser_->advance(); // consume 'int'
    } else {
        parser_->error("Expected return type");
        return nullptr;
    }

    // 関数名を解析
    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected function name");
        return nullptr;
    }

    Token name_token = parser_->advance();
    std::string function_name = name_token.value;

    // '(' を期待
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after function name");

    // パラメータリスト（現在は空のみサポート）
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after parameters");

    // 関数本体の開始 '{'
    parser_->consume(TokenType::TOK_LBRACE,
                     "Expected '{' to start function body");

    // 関数本体のパース
    ASTNode *function_node = new ASTNode(ASTNodeType::AST_FUNC_DECL);
    function_node->name = function_name;

    // return_typeをTypeInfo enum値として設定
    if (return_type == "int") {
        function_node->return_types.push_back(TYPE_INT);
    } else {
        function_node->return_types.push_back(TYPE_UNKNOWN);
    }

    // 文の解析
    ASTNode *body_node = new ASTNode(ASTNodeType::AST_STMT_LIST);
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        ASTNode *stmt = parser_->parseStatement();
        if (stmt != nullptr) {
            body_node->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }

    // 関数本体の終了 '}'
    parser_->consume(TokenType::TOK_RBRACE,
                     "Expected '}' to end function body");

    // bodyフィールドに設定
    function_node->body = std::unique_ptr<ASTNode>(body_node);
    return function_node;
}

/**
 * @brief 関数名の後の部分を解析
 * @param return_type 戻り値の型
 * @param function_name 関数名
 * @return 解析されたAST関数宣言ノード
 *
 * 既に関数名まで解析済みの場合に使用します。
 * パラメータリストと関数本体を解析します。
 */
ASTNode *DeclarationParser::parseFunctionDeclarationAfterName(
    const std::string &return_type, const std::string &function_name) {
    // '(' を期待（すでにチェック済み）
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after function name");

    // 関数本体のパース
    ASTNode *function_node = new ASTNode(ASTNodeType::AST_FUNC_DECL);
    function_node->name = function_name;
    function_node->is_unsigned = (return_type.rfind("unsigned ", 0) == 0);
    if (function_node->return_type_name.empty()) {
        function_node->return_type_name = return_type;
    }

    // DEBUG: 関数作成をログ出力
    debug_msg(DebugMsgId::PARSE_FUNCTION_CREATED, function_name.c_str());

    // パラメータリストの解析
    if (!parser_->check(TokenType::TOK_RPAREN)) {
        do {
            // パラメータ型
            std::string param_type = parser_->parseType();
            ParsedTypeInfo param_parsed = parser_->getLastParsedTypeInfo();
            // パラメータ名
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected parameter name");
                return nullptr;
            }

            Token param_name = parser_->advance();

            // パラメータノードを作成
            ASTNode *param = new ASTNode(ASTNodeType::AST_PARAM_DECL);
            param->name = param_name.value;
            param->type_name = param_type;

            // 配列パラメータかチェック
            param->is_array = (param_type.find("[") != std::string::npos);

            param->is_pointer = param_parsed.is_pointer;
            param->pointer_depth = param_parsed.pointer_depth;
            param->pointer_base_type_name = param_parsed.base_type;
            param->pointer_base_type = param_parsed.base_type_info;
            param->is_reference = param_parsed.is_reference;
            param->is_unsigned = param_parsed.is_unsigned;
            param->is_const = param_parsed.is_const;
            if (param_parsed.is_array) {
                param->array_type_info = param_parsed.array_info;
                param->is_array = true;
            }

            // 型情報を設定（typedef解決を含む）
            std::string resolved_param_type =
                parser_->resolveTypedefChain(param_type);

            if (resolved_param_type.empty()) {
                resolved_param_type = param_type;
            }
            // 元のtypedef名を保持（interpreterで解決するため）
            // param->type_name = resolved_param_type;  //
            // この行をコメントアウト

            if (resolved_param_type.find("[") != std::string::npos) {
                // 配列パラメータの場合
                param->is_array = true;
                std::string base_type = resolved_param_type.substr(
                    0, resolved_param_type.find("["));
                if (base_type == "int") {
                    param->type_info =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
                } else if (base_type == "string") {
                    param->type_info =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
                } else if (base_type == "bool") {
                    param->type_info =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL);
                } else if (base_type == "long") {
                    param->type_info =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_LONG);
                } else if (base_type == "short") {
                    param->type_info =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_SHORT);
                } else if (base_type == "tiny") {
                    param->type_info =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_TINY);
                } else if (base_type == "char") {
                    param->type_info =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_CHAR);
                } else {
                    param->type_info = TYPE_UNKNOWN;
                }
            } else if (resolved_param_type == "int") {
                param->type_info = TYPE_INT;
            } else if (resolved_param_type == "long") {
                param->type_info = TYPE_LONG;
            } else if (resolved_param_type == "short") {
                param->type_info = TYPE_SHORT;
            } else if (resolved_param_type == "tiny") {
                param->type_info = TYPE_TINY;
            } else if (resolved_param_type == "bool") {
                param->type_info = TYPE_BOOL;
            } else if (resolved_param_type == "string") {
                param->type_info = TYPE_STRING;
            } else if (parser_->struct_definitions_.find(resolved_param_type) !=
                           parser_->struct_definitions_.end() ||
                       parser_->struct_definitions_.find(param_type) !=
                           parser_->struct_definitions_.end() ||
                       (resolved_param_type.substr(0, 7) == "struct " &&
                        parser_->struct_definitions_.find(
                            resolved_param_type.substr(7)) !=
                            parser_->struct_definitions_.end())) {
                // struct型の場合（解決後の型名、元のtypedef名、または"struct
                // Name"形式で検索）
                param->type_info = TYPE_STRUCT;
            } else if (parser_->enum_definitions_.find(resolved_param_type) !=
                           parser_->enum_definitions_.end() ||
                       parser_->enum_definitions_.find(param_type) !=
                           parser_->enum_definitions_.end()) {
                // enum型の場合
                param->type_info = TYPE_ENUM;
            } else if (parser_->union_definitions_.find(resolved_param_type) !=
                           parser_->union_definitions_.end() ||
                       parser_->union_definitions_.find(param_type) !=
                           parser_->union_definitions_.end()) {
                // ユニオン型の場合
                param->type_info = TYPE_UNION;
            } else {
                param->type_info = TYPE_UNKNOWN;
            }

            if (param->type_info == TYPE_UNKNOWN) {
                param->type_info = parser_->getTypeInfoFromString(param_type);
            }

            function_node->parameters.push_back(
                std::unique_ptr<ASTNode>(param));
        } while (parser_->match(TokenType::TOK_COMMA));
    }

    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after parameters");

    // return_typeをTypeInfo enum値として設定
    if (return_type.find("[") != std::string::npos) {
        // 配列戻り値型
        std::string base_type = return_type.substr(0, return_type.find("["));
        if (base_type == "int") {
            function_node->return_types.push_back(
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT));
        } else if (base_type == "string") {
            function_node->return_types.push_back(
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING));
        } else if (base_type == "bool") {
            function_node->return_types.push_back(
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL));
        } else if (base_type == "long") {
            function_node->return_types.push_back(
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_LONG));
        } else if (base_type == "short") {
            function_node->return_types.push_back(
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_SHORT));
        } else if (base_type == "tiny") {
            function_node->return_types.push_back(
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_TINY));
        } else if (base_type == "char") {
            function_node->return_types.push_back(
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_CHAR));
        } else {
            function_node->return_types.push_back(TYPE_UNKNOWN);
        }
        function_node->is_array_return = true;
        function_node->return_type_name = return_type;
    } else if (return_type == "int") {
        function_node->return_types.push_back(TYPE_INT);
    } else if (return_type == "long") {
        function_node->return_types.push_back(TYPE_LONG);
    } else if (return_type == "short") {
        function_node->return_types.push_back(TYPE_SHORT);
    } else if (return_type == "tiny") {
        function_node->return_types.push_back(TYPE_TINY);
    } else if (return_type == "void") {
        function_node->return_types.push_back(TYPE_VOID);
    } else if (return_type == "bool") {
        function_node->return_types.push_back(TYPE_BOOL);
    } else if (return_type == "string") {
        function_node->return_types.push_back(TYPE_STRING);
    } else {
        // typedef型の可能性があるので、解決を試行
        std::string resolved_type = parser_->resolveTypedefChain(return_type);
        if (!resolved_type.empty() && resolved_type != return_type) {
            // typedef型が解決された場合、再帰的に処理
            if (resolved_type.find("[") != std::string::npos) {
                // 配列戻り値型
                std::string base_type =
                    resolved_type.substr(0, resolved_type.find("["));
                if (base_type == "int") {
                    function_node->return_types.push_back(
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT));
                } else if (base_type == "string") {
                    function_node->return_types.push_back(
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING));
                } else if (base_type == "bool") {
                    function_node->return_types.push_back(
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL));
                } else if (base_type == "long") {
                    function_node->return_types.push_back(
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_LONG));
                } else if (base_type == "short") {
                    function_node->return_types.push_back(
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_SHORT));
                } else if (base_type == "tiny") {
                    function_node->return_types.push_back(
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_TINY));
                } else if (base_type == "char") {
                    function_node->return_types.push_back(
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_CHAR));
                } else {
                    function_node->return_types.push_back(TYPE_UNKNOWN);
                }
                function_node->is_array_return = true;
                function_node->return_type_name = resolved_type;
            } else if (resolved_type == "int") {
                function_node->return_types.push_back(TYPE_INT);
            } else if (resolved_type == "long") {
                function_node->return_types.push_back(TYPE_LONG);
            } else if (resolved_type == "short") {
                function_node->return_types.push_back(TYPE_SHORT);
            } else if (resolved_type == "tiny") {
                function_node->return_types.push_back(TYPE_TINY);
            } else if (resolved_type == "bool") {
                function_node->return_types.push_back(TYPE_BOOL);
            } else if (resolved_type == "string") {
                function_node->return_types.push_back(TYPE_STRING);
            } else {
                function_node->return_types.push_back(TYPE_UNKNOWN);
            }
            if (!function_node->return_types.empty() &&
                function_node->return_types.back() == TYPE_UNKNOWN) {
                function_node->return_types.back() =
                    parser_->getTypeInfoFromString(resolved_type);
            }
        } else {
            // typedef型でない場合、getTypeInfoFromStringを使って型を判定
            TypeInfo type_info = parser_->getTypeInfoFromString(return_type);
            function_node->return_types.push_back(type_info);
        }
    }

    // 関数本体の開始 '{'
    parser_->consume(TokenType::TOK_LBRACE,
                     "Expected '{' to start function body");

    // 文のリストノードを作成
    ASTNode *body_node = new ASTNode(ASTNodeType::AST_STMT_LIST);

    // 文の解析
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        ASTNode *stmt = parser_->parseStatement();
        if (stmt != nullptr) {
            body_node->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }

    // 関数本体の終了 '}'
    parser_->consume(TokenType::TOK_RBRACE,
                     "Expected '}' to end function body");

    // bodyフィールドに設定
    function_node->body = std::unique_ptr<ASTNode>(body_node);

    return function_node;
}

// ========================================
// typedef宣言
// ========================================

/**
 * @brief typedef宣言を解析
 * @return 解析されたAST typedef宣言ノード
 *
 * サポートする構文:
 * - 型エイリアス: typedef MyInt = int;
 * - 配列型エイリアス: typedef IntArray = int[10];
 * - Union型: typedef Status = 200 | 404 | 500;
 * - 構造体typedef: typedef struct Point { ... } Point;
 * - enum typedef: typedef enum Color { ... } Color;
 */
ASTNode *DeclarationParser::parseTypedefDeclaration() {
    // typedef <type> <alias>; または typedef struct {...} <alias>; または
    // typedef enum {...} <alias>; または typedef union (TypeScript-like literal
    // types) または 関数ポインタtypedef
    parser_->consume(TokenType::TOK_TYPEDEF, "Expected 'typedef'");

    // typedef struct の場合
    if (parser_->check(TokenType::TOK_STRUCT)) {
        return parser_->parseStructTypedefDeclaration();
    }

    // typedef enum の場合
    if (parser_->check(TokenType::TOK_ENUM)) {
        return parser_->parseEnumTypedefDeclaration();
    }

    // 関数ポインタtypedefの場合
    if (parser_->isFunctionPointerTypedef()) {
        return parser_->parseFunctionPointerTypedefDeclaration();
    }

    // Check for both old and new typedef syntaxes
    // Old syntax: typedef TYPE ALIAS;
    // New syntax: typedef ALIAS = TYPE | TYPE2 | ...;

    // Check if this is new union syntax: typedef ALIAS = TYPE | TYPE2 | ...
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        // Temporarily save current token to check for new syntax
        Token saved_identifier = parser_->current_token_;
        parser_->advance(); // consume identifier

        if (parser_->check(TokenType::TOK_ASSIGN)) {
            // New syntax: typedef ALIAS = TYPE | TYPE2 | ...
            std::string alias_name = saved_identifier.value;

            // Union typedef declaration
            parser_->consume(TokenType::TOK_ASSIGN,
                             "Expected '=' after union typedef alias name");

            UnionDefinition union_def;
            union_def.name = alias_name;

            // Parse first value
            if (!parser_->parseUnionValue(union_def)) {
                parser_->error("Expected value after '=' in union typedef");
                return nullptr;
            }

            // Check if this is actually a union (has '|' separator)
            bool is_actual_union = false;

            // Parse additional values separated by '|'
            while (parser_->check(TokenType::TOK_PIPE)) {
                is_actual_union = true;
                parser_->advance(); // consume '|'
                if (!parser_->parseUnionValue(union_def)) {
                    parser_->error("Expected value after '|' in union typedef");
                    return nullptr;
                }
            }

            parser_->consume(TokenType::TOK_SEMICOLON,
                             "Expected ';' after typedef declaration");

            // If it's not an actual union (no '|' found), treat as regular
            // typedef
            if (!is_actual_union) {
                // This is a single-type alias like: typedef StringOnly =
                // string; Treat as regular typedef, not union

                // Check for single basic type
                if (union_def.allowed_types.size() == 1 &&
                    union_def.allowed_custom_types.empty() &&
                    union_def.allowed_array_types.empty() &&
                    !union_def.has_literal_values) {

                    TypeInfo single_type = union_def.allowed_types[0];
                    std::string type_name_str;
                    switch (single_type) {
                    case TYPE_INT:
                        type_name_str = "int";
                        break;
                    case TYPE_LONG:
                        type_name_str = "long";
                        break;
                    case TYPE_SHORT:
                        type_name_str = "short";
                        break;
                    case TYPE_TINY:
                        type_name_str = "tiny";
                        break;
                    case TYPE_BOOL:
                        type_name_str = "bool";
                        break;
                    case TYPE_STRING:
                        type_name_str = "string";
                        break;
                    case TYPE_CHAR:
                        type_name_str = "char";
                        break;
                    default:
                        type_name_str = "unknown";
                        break;
                    }

                    // Register as regular typedef
                    parser_->typedef_map_[alias_name] = type_name_str;

                    // Create regular typedef AST node
                    ASTNode *node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
                    node->name = alias_name;
                    node->type_name = type_name_str;
                    node->type_info = single_type;

                    parser_->setLocation(node, parser_->current_token_);
                    return node;
                }
                // Check for single custom type - treat as union to preserve
                // custom type validation
                else if (union_def.allowed_custom_types.size() == 1 &&
                         union_def.allowed_types.empty() &&
                         union_def.allowed_array_types.empty() &&
                         !union_def.has_literal_values) {

                    // Single custom type should be treated as union for type
                    // validation This preserves the semantic that only the
                    // specific custom type is allowed (not any type that
                    // resolves to the same basic type)

                    // Store union definition for type checking
                    parser_->union_definitions_[alias_name] = union_def;

                    // Create union typedef AST node
                    ASTNode *node =
                        new ASTNode(ASTNodeType::AST_UNION_TYPEDEF_DECL);
                    node->name = alias_name;
                    node->type_info = TYPE_UNION;
                    node->union_name = alias_name;
                    node->union_definition = union_def;

                    parser_->setLocation(node, parser_->current_token_);
                    return node;
                }
            }

            // Store union definition
            parser_->union_definitions_[alias_name] = union_def;

            // Create AST node
            ASTNode *node = new ASTNode(ASTNodeType::AST_UNION_TYPEDEF_DECL);
            node->name = alias_name;
            node->type_info = TYPE_UNION;
            node->union_name = alias_name;
            node->union_definition = union_def;

            parser_->setLocation(node, parser_->current_token_);

            return node;
        } else {
            // This is old syntax: typedef TYPE ALIAS;
            // The identifier we consumed is actually the base type
            std::string base_type_name = saved_identifier.value;
            TypeInfo base_type = TYPE_UNKNOWN;

            // Check if it's a known typedef type
            if (parser_->typedef_map_.find(base_type_name) !=
                parser_->typedef_map_.end()) {
                std::string resolved_type =
                    parser_->resolveTypedefChain(base_type_name);
                if (resolved_type.empty()) {
                    parser_->error("Unknown typedef type: " + base_type_name);
                    throw std::runtime_error("Unknown typedef type: " +
                                             base_type_name);
                }
                base_type_name = resolved_type;
                base_type = parser_->getTypeInfoFromString(
                    parser_->extractBaseType(resolved_type));
            }
            // Check if it's a struct type
            else if (parser_->struct_definitions_.find(base_type_name) !=
                     parser_->struct_definitions_.end()) {
                base_type = TYPE_STRUCT;
            }
            // Check if it's an enum type
            else if (parser_->enum_definitions_.find(base_type_name) !=
                     parser_->enum_definitions_.end()) {
                base_type = TYPE_INT; // enums are treated as int internally
            } else {
                parser_->error("Unknown type: " + base_type_name);
                throw std::runtime_error("Unknown type: " + base_type_name);
            }

            // Now parse the alias name
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected identifier for typedef alias");
                return nullptr;
            }

            std::string alias_name = parser_->current_token_.value;
            parser_->advance();

            parser_->consume(TokenType::TOK_SEMICOLON,
                             "Expected ';' after typedef declaration");

            // Add typedef mapping
            parser_->typedef_map_[alias_name] = base_type_name;

            // Create AST node
            ASTNode *node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
            node->name = alias_name;
            node->type_info = base_type;
            node->type_name = base_type_name;

            parser_->setLocation(node, parser_->current_token_);

            return node;
        }
    }

    // Old syntax: typedef TYPE ALIAS;
    // Parse base type first
    TypeInfo base_type = TYPE_UNKNOWN;
    std::string base_type_name;

    if (parser_->check(TokenType::TOK_INT)) {
        base_type = TYPE_INT;
        base_type_name = "int";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_LONG)) {
        base_type = TYPE_LONG;
        base_type_name = "long";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_SHORT)) {
        base_type = TYPE_SHORT;
        base_type_name = "short";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_TINY)) {
        base_type = TYPE_TINY;
        base_type_name = "tiny";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_BOOL)) {
        base_type = TYPE_BOOL;
        base_type_name = "bool";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_FLOAT)) {
        base_type = TYPE_FLOAT;
        base_type_name = "float";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_DOUBLE)) {
        base_type = TYPE_DOUBLE;
        base_type_name = "double";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_BIG)) {
        base_type = TYPE_BIG;
        base_type_name = "big";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_QUAD)) {
        base_type = TYPE_QUAD;
        base_type_name = "quad";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_STRING_TYPE)) {
        base_type = TYPE_STRING;
        base_type_name = "string";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_CHAR_TYPE)) {
        base_type = TYPE_CHAR;
        base_type_name = "char";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_VOID)) {
        base_type = TYPE_VOID;
        base_type_name = "void";
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        // 既存のstruct/enum型またはtypedef型を参照する場合
        std::string identifier = parser_->advance().value;

        // struct定義が存在するかチェック
        if (parser_->struct_definitions_.find(identifier) !=
            parser_->struct_definitions_.end()) {
            base_type = TYPE_STRUCT;
            base_type_name = identifier;
        }
        // enum定義が存在するかチェック
        else if (parser_->enum_definitions_.find(identifier) !=
                 parser_->enum_definitions_.end()) {
            base_type = TYPE_INT; // enumは内部的にintとして扱う
            base_type_name = identifier;
        }
        // 既存のtypedef型を参照する場合
        else if (parser_->typedef_map_.find(identifier) !=
                 parser_->typedef_map_.end()) {
            std::string resolved_type =
                parser_->resolveTypedefChain(identifier);
            if (resolved_type.empty()) {
                parser_->error("Unknown typedef type: " + identifier);
                throw std::runtime_error("Unknown typedef type: " + identifier);
            }
            base_type_name = resolved_type;
            base_type = parser_->getTypeInfoFromString(
                parser_->extractBaseType(resolved_type));
        } else {
            parser_->error("Unknown type: " + identifier);
            throw std::runtime_error("Unknown type: " + identifier);
        }
    } else {
        parser_->error("Expected type after typedef");
        return nullptr;
    }

    // Check for array type specification: TYPE[size], TYPE[SIZE_CONSTANT], or
    // multidimensional TYPE[size1][size2]...
    while (parser_->check(TokenType::TOK_LBRACKET)) {
        parser_->advance(); // consume '['

        std::string array_size;
        if (parser_->check(TokenType::TOK_NUMBER)) {
            array_size = parser_->current_token_.value;
            parser_->advance(); // consume array size
        } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
            // Allow identifier (like const variable name) as array size
            array_size = parser_->current_token_.value;
            parser_->advance(); // consume identifier
        } else {
            parser_->error("Expected array size in typedef");
            return nullptr;
        }

        parser_->consume(TokenType::TOK_RBRACKET,
                         "Expected ']' after array size");

        // Append array dimension to type name
        base_type_name = base_type_name + "[" + array_size + "]";
    }

    // Now parse the alias name
    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected identifier for typedef alias");
        return nullptr;
    }

    std::string alias_name = parser_->current_token_.value;
    parser_->advance();

    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after typedef declaration");

    // Add typedef mapping
    parser_->typedef_map_[alias_name] = base_type_name;

    // Create AST node
    ASTNode *node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
    node->name = alias_name;
    node->type_info = base_type;
    node->type_name = base_type_name;

    parser_->setLocation(node, parser_->current_token_);

    return node;
}

/**
 * @brief 関数ポインタtypedef宣言を解析
 * @return 解析されたAST関数ポインタtypedef宣言ノード
 *
 * 構文: typedef Callback = return_type(param1_type, param2_type, ...);
 *
 * 例:
 * - typedef IntFunc = int(int, int);
 * - typedef VoidFunc = void();
 *
 * 用途:
 * - コールバック関数の型定義
 * - 関数ポインタの型安全性向上
 */
ASTNode *DeclarationParser::parseFunctionPointerTypedefDeclaration() {
    // 戻り値型の解析
    std::string return_type_str = parser_->parseType();
    TypeInfo return_type = parser_->getTypeInfoFromString(return_type_str);

    // '(' の消費
    parser_->consume(TokenType::TOK_LPAREN,
                     "Expected '(' in function pointer typedef");

    // '*' の消費
    parser_->consume(TokenType::TOK_MUL,
                     "Expected '*' in function pointer typedef");

    // 関数ポインタ型名の取得
    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected identifier in function pointer typedef");
        return nullptr;
    }
    std::string typedef_name = parser_->current_token_.value;
    parser_->advance();

    // ')' の消費
    parser_->consume(TokenType::TOK_RPAREN,
                     "Expected ')' after function pointer name");

    // '(' の消費（パラメータリスト開始）
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' for parameter list");

    // パラメータリストの解析
    std::vector<TypeInfo> param_types;
    std::vector<std::string> param_type_names;
    std::vector<std::string> param_names;

    if (!parser_->check(TokenType::TOK_RPAREN)) {
        do {
            // パラメータ型の解析
            std::string param_type_str = parser_->parseType();
            TypeInfo param_type =
                parser_->getTypeInfoFromString(param_type_str);
            param_types.push_back(param_type);
            param_type_names.push_back(param_type_str);

            // パラメータ名は省略可能
            if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                param_names.push_back(parser_->current_token_.value);
                parser_->advance();
            } else {
                param_names.push_back(""); // 匿名パラメータ
            }

            if (parser_->check(TokenType::TOK_COMMA)) {
                parser_->advance();
            } else {
                break;
            }
        } while (true);
    }

    // ')' の消費（パラメータリスト終了）
    parser_->consume(TokenType::TOK_RPAREN,
                     "Expected ')' after parameter list");

    // ';' の消費
    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after function pointer typedef");

    // FunctionPointerTypeInfo の作成
    FunctionPointerTypeInfo fp_type_info(return_type, return_type_str,
                                         param_types, param_type_names,
                                         param_names);

    // 関数ポインタtypedefマップに登録
    parser_->function_pointer_typedefs_[typedef_name] = fp_type_info;

    // typedef マップにも登録（型名として認識させる）
    parser_->typedef_map_[typedef_name] = "function_pointer:" + typedef_name;

    // ASTノードの作成
    ASTNode *node = new ASTNode(ASTNodeType::AST_FUNCTION_POINTER_TYPEDEF);
    node->name = typedef_name;
    node->type_info = TYPE_FUNCTION_POINTER;
    node->is_function_pointer = true;
    node->function_pointer_type = fp_type_info;

    parser_->setLocation(node, parser_->current_token_);

    return node;
}
