#include "statement_parser.h"
#include "../recursive_lexer.h"
#include "../recursive_parser.h"
#include <memory>
#include <stdexcept>

/**
 * @file statement_parser.cpp
 * @brief 文解析を担当するStatementParserクラスの実装
 * @note recursive_parser.cppから移行
 * @note Phase 5-4-2で完全実装を追加
 */

// ========================================
// コンストラクタ
// ========================================

StatementParser::StatementParser(RecursiveParser *parser) : parser_(parser) {}

// ========================================
// 文解析メインエントリ
// ========================================

/**
 * @brief 文を解析（メインエントリポイント）
 * @return 解析されたAST文ノード
 *
 * トークンの種類に応じて適切な解析メソッドを呼び出します
 */
ASTNode *StatementParser::parseStatement() {
    // 修飾子のチェック
    bool isStatic = false;
    if (parser_->check(TokenType::TOK_STATIC)) {
        debug_msg(DebugMsgId::PARSE_STATIC_MODIFIER,
                  parser_->current_token_.line, parser_->current_token_.column);
        isStatic = true;
        parser_->advance();
    }

    bool isConst = false;
    if (parser_->check(TokenType::TOK_CONST)) {
        debug_msg(DebugMsgId::PARSE_CONST_MODIFIER,
                  parser_->current_token_.line, parser_->current_token_.column);
        isConst = true;
        parser_->advance();
    }

    // デバッグ情報
    std::string token_type_str =
        std::to_string(static_cast<int>(parser_->current_token_.type));
    debug_msg(DebugMsgId::PARSE_CURRENT_TOKEN,
              parser_->current_token_.value.c_str(), token_type_str.c_str());

    // 宣言文の処理（typedef, struct, enum, interface, impl, main）
    ASTNode *decl = parseDeclarationStatement(isStatic, isConst);
    if (decl)
        return decl;

    // typedef型・構造体型・インターフェース型の変数宣言/関数定義
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        std::string type_name = parser_->current_token_.value;
        ASTNode *result =
            parseTypedefTypeStatement(type_name, isStatic, isConst);
        if (result)
            return result;
    }

    // unsigned修飾子のチェック
    bool isUnsigned = false;
    if (parser_->check(TokenType::TOK_UNSIGNED)) {
        isUnsigned = true;
        parser_->advance();
    }

    // 基本型の変数宣言/関数定義
    ASTNode *basicType = parseBasicTypeStatement(isStatic, isConst, isUnsigned);
    if (basicType)
        return basicType;

    // 制御フロー文
    ASTNode *controlFlow = parseControlFlowStatement();
    if (controlFlow)
        return controlFlow;

    // 式文または代入文
    return parseExpressionOrAssignmentStatement();
}

// 宣言文の処理
ASTNode *StatementParser::parseDeclarationStatement(bool isStatic,
                                                    bool isConst) {
    // main関数
    if (parser_->check(TokenType::TOK_MAIN)) {
        Token main_token = parser_->current_token_;
        parser_->advance();
        if (parser_->check(TokenType::TOK_LPAREN)) {
            return parser_->parseFunctionDeclarationAfterName("int",
                                                              main_token.value);
        } else {
            parser_->error("Expected '(' after main");
            return nullptr;
        }
    }

    // typedef宣言
    if (parser_->check(TokenType::TOK_TYPEDEF)) {
        debug_msg(DebugMsgId::PARSE_TYPEDEF_START,
                  parser_->current_token_.line);
        return parser_->parseTypedefDeclaration();
    }

    // struct宣言
    if (parser_->check(TokenType::TOK_STRUCT)) {
        debug_msg(DebugMsgId::PARSE_STRUCT_DECL_START,
                  parser_->current_token_.line);
        return parser_->parseStructDeclaration();
    }

    // enum宣言
    if (parser_->check(TokenType::TOK_ENUM)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START,
                  parser_->current_token_.line);
        return parser_->parseEnumDeclaration();
    }

    // interface宣言
    if (parser_->check(TokenType::TOK_INTERFACE)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START,
                  parser_->current_token_.line);
        return parser_->parseInterfaceDeclaration();
    }

    // impl宣言
    if (parser_->check(TokenType::TOK_IMPL)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START,
                  parser_->current_token_.line);
        return parser_->parseImplDeclaration();
    }

    return nullptr; // 宣言文ではない
}

// 制御フロー文の処理
ASTNode *StatementParser::parseControlFlowStatement() {
    if (parser_->check(TokenType::TOK_RETURN)) {
        return parser_->parseReturnStatement();
    }
    if (parser_->check(TokenType::TOK_ASSERT)) {
        return parser_->parseAssertStatement();
    }
    if (parser_->check(TokenType::TOK_BREAK)) {
        return parser_->parseBreakStatement();
    }
    if (parser_->check(TokenType::TOK_CONTINUE)) {
        return parser_->parseContinueStatement();
    }
    if (parser_->check(TokenType::TOK_IF)) {
        return parseIfStatement();
    }
    if (parser_->check(TokenType::TOK_FOR)) {
        return parseForStatement();
    }
    if (parser_->check(TokenType::TOK_WHILE)) {
        return parseWhileStatement();
    }
    if (parser_->check(TokenType::TOK_LBRACE)) {
        return parseCompoundStatement();
    }
    if (parser_->check(TokenType::TOK_PRINTLN)) {
        return parsePrintlnStatement();
    }
    if (parser_->check(TokenType::TOK_PRINT)) {
        return parsePrintStatement();
    }

    return nullptr; // 制御フロー文ではない
}

// 式文または代入文の処理
ASTNode *StatementParser::parseExpressionOrAssignmentStatement() {
    ASTNode *expr = parser_->parseExpression();

    if (parser_->check(TokenType::TOK_SEMICOLON)) {
        parser_->advance();
    }

    return expr;
}

// typedef型・構造体型・インターフェース型の変数宣言/関数定義
ASTNode *
StatementParser::parseTypedefTypeStatement(const std::string &type_name,
                                           bool isStatic, bool isConst) {
    // typedef型か構造体型かをチェック
    bool is_typedef =
        parser_->typedef_map_.find(type_name) != parser_->typedef_map_.end();
    bool is_struct_type = parser_->struct_definitions_.find(type_name) !=
                          parser_->struct_definitions_.end();
    bool is_interface_type = parser_->interface_definitions_.find(type_name) !=
                             parser_->interface_definitions_.end();
    bool is_union_type = parser_->union_definitions_.find(type_name) !=
                         parser_->union_definitions_.end();
    bool is_enum_type = parser_->enum_definitions_.find(type_name) !=
                        parser_->enum_definitions_.end();

    if (!is_typedef && !is_struct_type && !is_interface_type &&
        !is_union_type && !is_enum_type) {
        return nullptr; // この識別子は型ではない
    }

    // 先読みして関数定義かチェック
    RecursiveLexer temp_lexer = parser_->lexer_;
    Token temp_current = parser_->current_token_;

    parser_->advance(); // 型名をスキップ

    // 配列戻り値型のチェック: Type[...] identifier(...)
    while (parser_->check(TokenType::TOK_LBRACKET)) {
        parser_->advance(); // '['
        while (!parser_->check(TokenType::TOK_RBRACKET) &&
               !parser_->isAtEnd()) {
            parser_->advance();
        }
        if (parser_->check(TokenType::TOK_RBRACKET)) {
            parser_->advance(); // ']'
        }
    }

    // ポインタ/参照修飾子をスキップ
    while (parser_->check(TokenType::TOK_MUL) ||
           parser_->check(TokenType::TOK_BIT_AND)) {
        parser_->advance();
    }

    // 識別子が続くかチェック
    bool is_function = false;
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->advance(); // 識別子をスキップ

        // '(' があれば関数定義
        if (parser_->check(TokenType::TOK_LPAREN)) {
            is_function = true;
        }
    }

    // 元の位置に戻す
    parser_->lexer_ = temp_lexer;
    parser_->current_token_ = temp_current;

    if (is_function) {
        // 関数定義
        std::string return_type = parser_->advance().value; // 型名を取得

        // 配列戻り値の場合: Person[2] function_name()
        if (parser_->check(TokenType::TOK_LBRACKET)) {
            return_type += parser_->advance().value; // '['
            while (!parser_->check(TokenType::TOK_RBRACKET) &&
                   !parser_->isAtEnd()) {
                return_type += parser_->advance().value;
            }
            if (parser_->check(TokenType::TOK_RBRACKET)) {
                return_type += parser_->advance().value; // ']'
            }
        }

        std::string function_name = parser_->advance().value;
        debug_msg(DebugMsgId::PARSE_FUNCTION_DECL_FOUND, function_name.c_str(),
                  return_type.c_str());
        return parser_->parseFunctionDeclarationAfterName(return_type,
                                                          function_name);
    } else if (is_struct_type) {
        // 構造体変数宣言
        debug_msg(DebugMsgId::PARSE_STRUCT_VAR_DECL_FOUND, type_name.c_str());
        std::string struct_type = parser_->advance().value;

        // ポインタまたは参照チェック
        bool is_pointer = false;
        bool is_reference = false;
        int pointer_depth = 0;

        while (parser_->check(TokenType::TOK_MUL)) {
            is_pointer = true;
            pointer_depth++;
            parser_->advance();
        }

        if (parser_->check(TokenType::TOK_BIT_AND)) {
            is_reference = true;
            parser_->advance();
        }

        // ポインタまたは参照の場合
        if (is_pointer || is_reference) {
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error(
                    "Expected variable name after pointer/reference type");
                return nullptr;
            }

            std::string var_name = parser_->advance().value;

            ASTNode *var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
            var_node->name = var_name;
            var_node->type_name = struct_type;
            var_node->type_info = is_pointer ? TYPE_POINTER : TYPE_STRUCT;
            var_node->is_const = isConst;
            var_node->is_pointer = is_pointer;
            var_node->pointer_depth = pointer_depth;
            var_node->is_reference = is_reference;
            var_node->pointer_base_type = TYPE_STRUCT;
            var_node->pointer_base_type_name = struct_type;

            // 初期化式のチェック
            if (parser_->match(TokenType::TOK_ASSIGN)) {
                var_node->init_expr =
                    std::unique_ptr<ASTNode>(parser_->parseExpression());
            }

            parser_->consume(
                TokenType::TOK_SEMICOLON,
                "Expected ';' after pointer/reference variable declaration");
            return var_node;
        }

        // 配列チェック: Person[3] people; または Person people;
        if (parser_->check(TokenType::TOK_LBRACKET)) {
            // struct配列宣言
            debug_msg(DebugMsgId::PARSE_STRUCT_ARRAY_DECL, struct_type.c_str());
            parser_->advance(); // consume '['
            ASTNode *size_expr = parser_->parseExpression();
            parser_->consume(TokenType::TOK_RBRACKET,
                             "Expected ']' after array size");

            std::string var_name = parser_->advance().value;
            debug_msg(DebugMsgId::PARSE_STRUCT_ARRAY_VAR_NAME,
                      var_name.c_str());

            ASTNode *var_node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
            var_node->name = var_name;
            var_node->type_name = struct_type;
            var_node->type_info = TYPE_STRUCT;
            var_node->is_const = isConst;
            var_node->array_size_expr = std::unique_ptr<ASTNode>(size_expr);

            // array_type_infoを設定
            var_node->array_type_info.base_type = TYPE_STRUCT;
            var_node->array_type_info.dimensions.push_back({0});

            // 初期化式のチェック
            if (parser_->match(TokenType::TOK_ASSIGN)) {
                if (parser_->check(TokenType::TOK_LBRACKET)) {
                    var_node->init_expr =
                        std::unique_ptr<ASTNode>(parser_->parseArrayLiteral());
                } else {
                    var_node->init_expr =
                        std::unique_ptr<ASTNode>(parser_->parseExpression());
                }
            }

            parser_->consume(TokenType::TOK_SEMICOLON,
                             "Expected ';' after struct array declaration");
            return var_node;
        } else {
            // 通常のstruct変数宣言
            std::string var_name = parser_->advance().value;

            debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(),
                      struct_type.c_str());

            ASTNode *var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
            var_node->name = var_name;
            var_node->type_name = struct_type;
            var_node->type_info = TYPE_STRUCT;
            var_node->is_const = isConst;

            // 初期化式のチェック
            if (parser_->match(TokenType::TOK_ASSIGN)) {
                if (parser_->check(TokenType::TOK_LBRACE)) {
                    var_node->init_expr =
                        std::unique_ptr<ASTNode>(parser_->parseStructLiteral());
                } else {
                    var_node->init_expr =
                        std::unique_ptr<ASTNode>(parser_->parseExpression());
                }
            }

            parser_->consume(TokenType::TOK_SEMICOLON,
                             "Expected ';' after struct variable declaration");
            return var_node;
        }
    } else if (is_interface_type) {
        // interface変数宣言
        debug_msg(DebugMsgId::PARSE_STRUCT_VAR_DECL_FOUND, type_name.c_str());
        std::string interface_type = parser_->advance().value;

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

        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->error("Expected interface variable name");
            return nullptr;
        }

        std::string var_name = parser_->advance().value;

        debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(),
                  interface_type.c_str());

        ASTNode *var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        var_node->name = var_name;
        var_node->type_name = interface_type;
        var_node->type_info = TYPE_INTERFACE;

        // ポインタと参照の情報を設定
        if (pointer_depth > 0) {
            var_node->is_pointer = true;
            var_node->pointer_depth = pointer_depth;
            var_node->pointer_base_type_name = interface_type;
            var_node->pointer_base_type = TYPE_INTERFACE;

            for (int i = 0; i < pointer_depth; i++) {
                var_node->type_name += "*";
            }
        }

        if (is_reference) {
            var_node->is_reference = true;
            var_node->type_name += "&";
        }

        // 初期化式のチェック
        if (parser_->match(TokenType::TOK_ASSIGN)) {
            var_node->init_expr =
                std::unique_ptr<ASTNode>(parser_->parseExpression());
        }

        parser_->consume(TokenType::TOK_SEMICOLON,
                         "Expected ';' after interface variable declaration");

        return var_node;
    } else if (is_enum_type) {
        // enum型変数宣言
        debug_msg(DebugMsgId::PARSE_VAR_DECL, "", type_name.c_str());
        std::string enum_type = parser_->advance().value;

        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->error("Expected enum variable name");
            return nullptr;
        }

        std::string var_name = parser_->advance().value;

        ASTNode *var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        var_node->name = var_name;
        var_node->type_name = enum_type;
        var_node->type_info = TYPE_ENUM;
        var_node->is_const = isConst;

        // 初期化式のチェック
        if (parser_->match(TokenType::TOK_ASSIGN)) {
            var_node->init_expr =
                std::unique_ptr<ASTNode>(parser_->parseExpression());
        }

        parser_->consume(TokenType::TOK_SEMICOLON,
                         "Expected ';' after enum variable declaration");

        return var_node;
    } else {
        // typedef型変数宣言
        return parser_->parseTypedefVariableDeclaration();
    }
}

// 基本型の変数宣言/関数定義
ASTNode *StatementParser::parseBasicTypeStatement(bool isStatic, bool isConst,
                                                  bool isUnsigned) {
    // 基本型のチェック
    if (!parser_->check(TokenType::TOK_INT) &&
        !parser_->check(TokenType::TOK_LONG) &&
        !parser_->check(TokenType::TOK_SHORT) &&
        !parser_->check(TokenType::TOK_TINY) &&
        !parser_->check(TokenType::TOK_VOID) &&
        !parser_->check(TokenType::TOK_BOOL) &&
        !parser_->check(TokenType::TOK_STRING_TYPE) &&
        !parser_->check(TokenType::TOK_CHAR_TYPE) &&
        !parser_->check(TokenType::TOK_FLOAT) &&
        !parser_->check(TokenType::TOK_DOUBLE) &&
        !parser_->check(TokenType::TOK_BIG) &&
        !parser_->check(TokenType::TOK_QUAD)) {

        if (isUnsigned) {
            parser_->error("Expected type specifier after 'unsigned'");
        }
        return nullptr;
    }

    // 型名を取得
    std::string base_type_name;
    if (parser_->check(TokenType::TOK_INT))
        base_type_name = "int";
    else if (parser_->check(TokenType::TOK_LONG))
        base_type_name = "long";
    else if (parser_->check(TokenType::TOK_SHORT))
        base_type_name = "short";
    else if (parser_->check(TokenType::TOK_TINY))
        base_type_name = "tiny";
    else if (parser_->check(TokenType::TOK_VOID))
        base_type_name = "void";
    else if (parser_->check(TokenType::TOK_BOOL))
        base_type_name = "bool";
    else if (parser_->check(TokenType::TOK_STRING_TYPE))
        base_type_name = "string";
    else if (parser_->check(TokenType::TOK_CHAR_TYPE))
        base_type_name = "char";
    else if (parser_->check(TokenType::TOK_FLOAT))
        base_type_name = "float";
    else if (parser_->check(TokenType::TOK_DOUBLE))
        base_type_name = "double";
    else if (parser_->check(TokenType::TOK_BIG))
        base_type_name = "big";
    else if (parser_->check(TokenType::TOK_QUAD))
        base_type_name = "quad";

    parser_->advance(); // consume type

    // ポインタ修飾子のチェック
    int pointer_depth = 0;
    while (parser_->check(TokenType::TOK_MUL)) {
        pointer_depth++;
        parser_->advance();
    }

    // 参照修飾子のチェック
    bool is_reference = false;
    if (parser_->check(TokenType::TOK_BIT_AND)) {
        is_reference = true;
        parser_->advance();
    }

    TypeInfo base_type_info = parser_->getTypeInfoFromString(base_type_name);
    std::string type_name = base_type_name;

    // ポインタ記号を型名に追加
    if (pointer_depth > 0) {
        type_name += std::string(pointer_depth, '*');
    }

    // 参照記号を型名に追加
    if (is_reference) {
        type_name += "&";
    }

    if (isUnsigned) {
        switch (base_type_info) {
        case TYPE_TINY:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
        case TYPE_BIG:
        case TYPE_QUAD:
            break;
        default:
            parser_->error(
                "'unsigned' modifier can only be applied to numeric types");
            return nullptr;
        }
        type_name = "unsigned " + base_type_name;
    }

    // 参照型の場合、型情報は基底型から取得
    std::string type_for_info = type_name;
    if (is_reference && type_for_info.back() == '&') {
        type_for_info.pop_back(); // '&'を削除
    }

    TypeInfo declared_type_info = parser_->getTypeInfoFromString(type_for_info);

    // 配列型の場合: int[size][size2]... identifier
    if (parser_->check(TokenType::TOK_LBRACKET)) {
        return parseArrayDeclaration(base_type_name, type_name, base_type_info,
                                     declared_type_info, isStatic, isConst,
                                     isUnsigned, is_reference);
    }

    // 関数宣言か通常の変数宣言かチェック
    if (parser_->check(TokenType::TOK_IDENTIFIER) ||
        parser_->check(TokenType::TOK_MAIN)) {
        Token name_token = parser_->current_token_;
        parser_->advance(); // consume identifier/main

        if (parser_->check(TokenType::TOK_LPAREN)) {
            // これは関数定義
            return parser_->parseFunctionDeclarationAfterName(type_name,
                                                              name_token.value);
        } else {
            // 変数宣言: type identifier [, identifier2, ...] [= expr];
            return parseVariableDeclarationList(
                name_token.value, type_name, base_type_name, base_type_info,
                declared_type_info, pointer_depth, isStatic, isConst,
                isUnsigned, is_reference);
        }
    } else {
        parser_->error("Expected identifier after type");
        return nullptr;
    }
}

// 配列宣言の処理（多次元対応）
ASTNode *StatementParser::parseArrayDeclaration(
    const std::string &base_type_name, const std::string &type_name,
    TypeInfo base_type_info, TypeInfo declared_type_info, bool isStatic,
    bool isConst, bool isUnsigned, bool is_reference) {
    // 全ての配列次元を解析
    std::vector<std::string> array_sizes;

    while (parser_->check(TokenType::TOK_LBRACKET)) {
        parser_->advance(); // consume '['

        std::string size = "";
        if (parser_->check(TokenType::TOK_NUMBER)) {
            size = parser_->advance().value;
        } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
            // 変数名を配列サイズとして使用
            size = parser_->advance().value;
            // 簡単な算術式もサポート（n+1のような形）
            if (parser_->check(TokenType::TOK_PLUS)) {
                parser_->advance(); // consume '+'
                if (parser_->check(TokenType::TOK_NUMBER)) {
                    size += "+" + parser_->advance().value;
                }
            }
        } else {
            // 空の配列サイズ（動的配列）
            size = "";
        }
        array_sizes.push_back(size);

        parser_->consume(TokenType::TOK_RBRACKET, "Expected ']' in array type");
    }

    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected identifier after array type");
        return nullptr;
    }

    Token name_token = parser_->advance(); // consume identifier
    std::string var_name = name_token.value;

    // 関数宣言かチェック
    if (parser_->check(TokenType::TOK_LPAREN)) {
        // これは配列戻り値の関数宣言
        std::string return_type = type_name;
        for (const auto &size : array_sizes) {
            return_type += "[" + size + "]";
        }
        return parser_->parseFunctionDeclarationAfterName(return_type,
                                                          var_name);
    }

    ASTNode *node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
    node->name = var_name;

    // 型名構築（例: "int[2][3]"）
    std::string full_type_name = type_name;
    for (const auto &size : array_sizes) {
        full_type_name += "[" + size + "]";
    }
    node->type_name = full_type_name;

    // Set appropriate type_info for arrays
    node->type_info = base_type_info;

    // const修飾子を設定
    node->is_const = isConst;
    node->is_static = isStatic;
    node->is_unsigned = isUnsigned;
    node->is_reference = is_reference;

    // ArrayTypeInfoを構築
    std::vector<ArrayDimension> dimensions;
    for (const auto &size : array_sizes) {
        if (!size.empty()) {
            // 数値リテラルかどうかをチェック
            bool is_number = true;
            for (char c : size) {
                if (!std::isdigit(c)) {
                    is_number = false;
                    break;
                }
            }

            if (is_number) {
                int dim_size = std::stoi(size);
                dimensions.push_back(ArrayDimension(dim_size, false));
            } else {
                // 変数または式なので動的サイズとしてマーク
                dimensions.push_back(ArrayDimension(-1, true));
            }
        } else {
            // 動的サイズ
            dimensions.push_back(ArrayDimension(-1, true));
        }
    }
    node->array_type_info = ArrayTypeInfo(base_type_info, dimensions);

    // 配列次元をASTノードに設定
    for (const auto &size : array_sizes) {
        if (!size.empty()) {
            // 数値リテラルかどうかをチェック
            bool is_number = true;
            for (char c : size) {
                if (!std::isdigit(c)) {
                    is_number = false;
                    break;
                }
            }

            if (is_number) {
                // 純粋な数値リテラル
                ASTNode *size_expr = new ASTNode(ASTNodeType::AST_NUMBER);
                size_expr->int_value = std::stoll(size);
                node->array_dimensions.push_back(
                    std::unique_ptr<ASTNode>(size_expr));
            } else {
                // 変数または式
                if (size.find('+') != std::string::npos) {
                    // 簡単な加算式 (n+1) をパース
                    size_t plus_pos = size.find('+');
                    std::string var_name_str = size.substr(0, plus_pos);
                    std::string number_str = size.substr(plus_pos + 1);

                    ASTNode *add_expr = new ASTNode(ASTNodeType::AST_BINARY_OP);
                    add_expr->op = "+";

                    ASTNode *var_node = new ASTNode(ASTNodeType::AST_VARIABLE);
                    var_node->name = var_name_str;
                    add_expr->left = std::unique_ptr<ASTNode>(var_node);

                    ASTNode *num_node = new ASTNode(ASTNodeType::AST_NUMBER);
                    num_node->int_value = std::stoll(number_str);
                    add_expr->right = std::unique_ptr<ASTNode>(num_node);

                    node->array_dimensions.push_back(
                        std::unique_ptr<ASTNode>(add_expr));
                } else {
                    // 単純な変数
                    ASTNode *size_expr = new ASTNode(ASTNodeType::AST_VARIABLE);
                    size_expr->name = size;
                    node->array_dimensions.push_back(
                        std::unique_ptr<ASTNode>(size_expr));
                }
            }
        } else {
            // 動的サイズ
            node->array_dimensions.push_back(std::unique_ptr<ASTNode>(nullptr));
        }
    }

    // 1次元配列の場合は従来の方式も設定（互換性のため）
    if (array_sizes.size() == 1) {
        if (!array_sizes[0].empty()) {
            // 数値リテラルかチェック
            bool is_number = true;
            for (char c : array_sizes[0]) {
                if (!std::isdigit(c)) {
                    is_number = false;
                    break;
                }
            }
            if (is_number) {
                node->array_size = std::stoi(array_sizes[0]);
            } else {
                // 変数サイズなので実行時に決定
                node->array_size = -1;
            }
        } else {
            // 動的配列（TYPE[]）の場合
            node->array_size = 0;
        }
    }

    // 配列初期化をチェック int[SIZE] var = [...]
    if (parser_->check(TokenType::TOK_ASSIGN)) {
        parser_->advance(); // consume '='

        if (parser_->check(TokenType::TOK_LBRACKET)) {
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

            // サイズと要素数の検証（1次元配列の場合のみ）
            if (array_sizes.size() == 1 && !array_sizes[0].empty()) {
                bool is_number = true;
                for (char c : array_sizes[0]) {
                    if (!std::isdigit(c)) {
                        is_number = false;
                        break;
                    }
                }
                if (is_number) {
                    int declared_size = std::stoi(array_sizes[0]);
                    if (declared_size !=
                        static_cast<int>(array_literal->arguments.size())) {
                        parser_->error(
                            "Array literal size (" +
                            std::to_string(array_literal->arguments.size()) +
                            ") does not match declared size (" +
                            array_sizes[0] + ")");
                        return nullptr;
                    }
                }
            }

            node->init_expr = std::unique_ptr<ASTNode>(array_literal);
        } else {
            // 配列リテラル以外の式（配列スライス等）
            ASTNode *expr = parser_->parseExpression();
            node->init_expr = std::unique_ptr<ASTNode>(expr);
        }
    }

    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after array declaration");
    return node;
}

// 変数宣言リストの処理（カンマ区切り）
ASTNode *StatementParser::parseVariableDeclarationList(
    const std::string &first_var_name, const std::string &type_name,
    const std::string &base_type_name, TypeInfo base_type_info,
    TypeInfo declared_type_info, int pointer_depth, bool isStatic, bool isConst,
    bool isUnsigned, bool is_reference) {
    std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> variables;

    // 最初の変数を追加
    std::unique_ptr<ASTNode> init_expr = nullptr;
    if (parser_->match(TokenType::TOK_ASSIGN)) {
        init_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
    }
    variables.emplace_back(first_var_name, std::move(init_expr));

    // カンマで区切られた追加の変数をパース
    while (parser_->match(TokenType::TOK_COMMA)) {
        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->error("Expected variable name after ','");
            return nullptr;
        }

        std::string var_name = parser_->advance().value;
        std::unique_ptr<ASTNode> var_init = nullptr;

        if (parser_->match(TokenType::TOK_ASSIGN)) {
            var_init = std::unique_ptr<ASTNode>(parser_->parseExpression());
        }

        variables.emplace_back(var_name, std::move(var_init));
    }

    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");

    // 単一変数か複数変数かに応じてノードを作成
    if (variables.size() == 1) {
        ASTNode *node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        node->name = variables[0].first;
        node->type_name = type_name;
        node->is_const = isConst;
        node->is_static = isStatic;
        node->is_unsigned = isUnsigned;
        node->is_reference = is_reference;

        // ポインタ情報を設定
        if (pointer_depth > 0) {
            node->is_pointer = true;
            node->pointer_depth = pointer_depth;
            node->pointer_base_type_name = base_type_name;
            node->pointer_base_type = base_type_info;
            node->type_info = TYPE_POINTER;
        } else {
            // 型情報を設定
            node->type_info = declared_type_info;
        }

        if (variables[0].second) {
            node->init_expr = std::move(variables[0].second);
        }

        return node;
    } else {
        // 複数変数宣言
        ASTNode *node = new ASTNode(ASTNodeType::AST_MULTIPLE_VAR_DECL);
        node->type_name = type_name;
        node->is_unsigned = isUnsigned;
        node->is_reference = is_reference;

        // ポインタ情報を設定
        if (pointer_depth > 0) {
            node->is_pointer = true;
            node->pointer_depth = pointer_depth;
            node->pointer_base_type_name = base_type_name;
            node->pointer_base_type = base_type_info;
            node->type_info = TYPE_POINTER;
        } else {
            // 型情報を設定
            node->type_info = declared_type_info;
        }

        // 各変数を子ノードとして追加
        for (auto &var : variables) {
            ASTNode *var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
            var_node->name = var.first;
            var_node->type_name = type_name;
            var_node->is_const = isConst;
            var_node->is_reference = is_reference;
            var_node->is_static = isStatic;
            var_node->is_unsigned = isUnsigned;

            // ポインタ情報も設定
            if (pointer_depth > 0) {
                var_node->is_pointer = true;
                var_node->pointer_depth = pointer_depth;
                var_node->pointer_base_type_name = base_type_name;
                var_node->pointer_base_type = base_type_info;
                var_node->type_info = TYPE_POINTER;
            } else {
                var_node->type_info = node->type_info;
            }

            if (var.second) {
                var_node->init_expr = std::move(var.second);
            }

            node->children.push_back(std::unique_ptr<ASTNode>(var_node));
        }

        return node;
    }
}

ASTNode *StatementParser::parseCompoundStatement() {
    parser_->advance(); // consume '{'

    ASTNode *compound = new ASTNode(ASTNodeType::AST_COMPOUND_STMT);

    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        ASTNode *stmt = parser_->parseStatement();
        if (stmt) {
            compound->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }

    parser_->consume(TokenType::TOK_RBRACE, "Expected '}'");
    return compound;
}

// ========================================
// 制御構造
// ========================================

/**
 * @brief if文を解析
 * @return 解析されたASTif文ノード
 *
 * 構文:
 * - if (condition) statement
 * - if (condition) statement else statement
 * - if (condition) statement else if (condition) statement else statement
 */
ASTNode *StatementParser::parseIfStatement() {
    parser_->advance(); // consume 'if'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after if");

    ASTNode *if_node = new ASTNode(ASTNodeType::AST_IF_STMT);
    if_node->condition = std::unique_ptr<ASTNode>(parser_->parseExpression());

    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after if condition");

    // if本体をパース（then節はleftに格納してinterpreterと統一）
    if_node->left = std::unique_ptr<ASTNode>(parser_->parseStatement());

    // else節があるかチェック
    if (parser_->match(TokenType::TOK_ELSE)) {
        if_node->right = std::unique_ptr<ASTNode>(parser_->parseStatement());
    }

    return if_node;
}

/**
 * @brief for文を解析
 * @return 解析されたASTfor文ノード
 *
 * 構文: for (init; condition; update) statement
 *
 * サポートする形式:
 * - for (int i = 0; i < 10; i++) { ... }
 * - for (; condition; ) { ... }
 */
ASTNode *StatementParser::parseForStatement() {
    parser_->advance(); // consume 'for'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after for");

    ASTNode *for_node = new ASTNode(ASTNodeType::AST_FOR_STMT);

    // 初期化部分 (int i = 0;) - 文として扱う
    for_node->init_expr = std::unique_ptr<ASTNode>(parser_->parseStatement());

    // 条件部分 (i < 5) - 式として扱う
    for_node->condition = std::unique_ptr<ASTNode>(parser_->parseExpression());
    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after for condition");

    // 更新部分 - 一般的な式として処理（i++, i--, i=i+1など）
    for_node->update_expr =
        std::unique_ptr<ASTNode>(parser_->parseExpression());

    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after for update");

    // for本体
    for_node->body = std::unique_ptr<ASTNode>(parser_->parseStatement());

    return for_node;
}

/**
 * @brief while文を解析
 * @return 解析されたASTwhile文ノード
 *
 * 構文: while (condition) statement
 */
ASTNode *StatementParser::parseWhileStatement() {
    parser_->advance(); // consume 'while'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after while");

    ASTNode *while_node = new ASTNode(ASTNodeType::AST_WHILE_STMT);

    // 条件部分
    while_node->condition =
        std::unique_ptr<ASTNode>(parser_->parseExpression());

    parser_->consume(TokenType::TOK_RPAREN,
                     "Expected ')' after while condition");

    // while本体
    while_node->body = std::unique_ptr<ASTNode>(parser_->parseStatement());

    return while_node;
}

// ========================================
// ジャンプ文
// ========================================

/**
 * @brief return文を解析
 * @return 解析されたASTreturn文ノード
 *
 * 構文:
 * - return;
 * - return expression;
 */
ASTNode *StatementParser::parseReturnStatement() {
    parser_->advance(); // consume 'return'
    ASTNode *return_node = new ASTNode(ASTNodeType::AST_RETURN_STMT);

    // return値があるかチェック
    if (!parser_->check(TokenType::TOK_SEMICOLON)) {
        return_node->left =
            std::unique_ptr<ASTNode>(parser_->parseExpression());
    }

    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after return statement");
    return return_node;
}

/**
 * @brief break文を解析
 * @return 解析されたASTbreak文ノード
 *
 * 構文: break;
 * ループまたはswitchから脱出（現在はループのみサポート）
 */
ASTNode *StatementParser::parseBreakStatement() {
    parser_->advance(); // consume 'break'
    ASTNode *break_node = new ASTNode(ASTNodeType::AST_BREAK_STMT);
    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after break statement");
    return break_node;
}

/**
 * @brief continue文を解析
 * @return 解析されたASTcontinue文ノード
 *
 * 構文: continue;
 * ループの次の反復へスキップ
 */
ASTNode *StatementParser::parseContinueStatement() {
    parser_->advance(); // consume 'continue'
    ASTNode *continue_node = new ASTNode(ASTNodeType::AST_CONTINUE_STMT);
    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after continue statement");
    return continue_node;
}

// ========================================
// 出力・デバッグ
// ========================================

/**
 * @brief assert文を解析
 * @return 解析されたASTassert文ノード
 *
 * 構文: assert(condition);
 * 条件が偽の場合、プログラムを停止
 */
ASTNode *StatementParser::parseAssertStatement() {
    Token assert_token = parser_->advance(); // consume 'assert'

    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after assert");

    // 条件式をパース
    ASTNode *condition = parser_->parseExpression();

    parser_->consume(TokenType::TOK_RPAREN,
                     "Expected ')' after assert condition");
    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after assert statement");

    ASTNode *assert_node = new ASTNode(ASTNodeType::AST_ASSERT_STMT);
    assert_node->left = std::unique_ptr<ASTNode>(condition);
    assert_node->location.line = assert_token.line;

    return assert_node;
}

/**
 * @brief println文を解析
 * @return 解析されたAST出力文ノード
 *
 * 構文: println(arg1, arg2, ...);
 * 可変長引数をサポート、自動的に改行を追加
 */
ASTNode *StatementParser::parsePrintlnStatement() {
    parser_->advance(); // consume 'println'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after println");

    ASTNode *print_node = new ASTNode(ASTNodeType::AST_PRINTLN_STMT);

    // 複数の引数をパース
    if (!parser_->check(TokenType::TOK_RPAREN)) {
        do {
            ASTNode *arg = parser_->parseExpression();
            print_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
        } while (parser_->match(TokenType::TOK_COMMA));
    }

    parser_->consume(TokenType::TOK_RPAREN,
                     "Expected ')' after println arguments");
    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after println statement");
    return print_node;
}

/**
 * @brief print文を解析
 * @return 解析されたAST出力文ノード
 *
 * 構文: print(format, arg1, arg2, ...);
 * printf風のフォーマット指定子をサポート
 * - %d: 整数
 * - %lld: 長整数
 * - %u: 符号なし整数
 * - %s: 文字列
 * - %c: 文字
 * - %%: % のエスケープ
 */
ASTNode *StatementParser::parsePrintStatement() {
    parser_->advance(); // consume 'print'

    ASTNode *print_node = new ASTNode(ASTNodeType::AST_PRINT_STMT);

    // 引数をパース - 任意の式を受け入れる
    if (parser_->check(TokenType::TOK_LPAREN)) {
        // print(expression[, expression, ...]); 形式
        parser_->advance(); // consume '('

        // 複数の引数をパース
        if (!parser_->check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode *arg = parser_->parseExpression();
                print_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
            } while (parser_->match(TokenType::TOK_COMMA));
        }

        parser_->consume(TokenType::TOK_RPAREN,
                         "Expected ')' after print arguments");
    } else if (!parser_->check(TokenType::TOK_SEMICOLON)) {
        // print expression; 形式（括弧なし）
        print_node->left = std::unique_ptr<ASTNode>(parser_->parseExpression());
    } else {
        parser_->error("Expected expression after print");
        return nullptr;
    }

    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after print statement");
    return print_node;
}
