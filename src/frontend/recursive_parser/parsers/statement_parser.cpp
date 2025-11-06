#include "statement_parser.h"
#include "../recursive_lexer.h"
#include "../recursive_parser.h"
#include <cstdio>
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
    // export修飾子のチェック（最初にチェック）
    bool isExported = false;
    bool isDefaultExport = false;
    if (parser_->check(TokenType::TOK_EXPORT)) {
        isExported = true;
        parser_->advance();

        // default exportのチェック
        if (parser_->check(TokenType::TOK_DEFAULT)) {
            isDefaultExport = true;
            parser_->advance();
        }
    }

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

    // 宣言文の処理（typedef, struct, enum, interface, impl, main, import）
    ASTNode *decl = parseDeclarationStatement(isStatic, isConst, isExported);
    if (decl) {
        if (isDefaultExport) {
            decl->is_default_export = true;
        }
        return decl;
    }

    // typedef型・構造体型・インターフェース型の変数宣言/関数定義
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        std::string type_name = parser_->current_token_.value;
        ASTNode *result =
            parseTypedefTypeStatement(type_name, isStatic, isConst);
        if (result) {
            // exportフラグが設定されている場合、関数宣言・変数宣言にフラグを設定
            if (isExported &&
                (result->node_type == ASTNodeType::AST_FUNC_DECL ||
                 result->node_type == ASTNodeType::AST_VAR_DECL)) {
                result->is_exported = true;
                if (isDefaultExport) {
                    result->is_default_export = true;
                }
            }
            return result;
        }
    }

    // unsigned修飾子のチェック
    bool isUnsigned = false;
    if (parser_->check(TokenType::TOK_UNSIGNED)) {
        isUnsigned = true;
        parser_->advance();
    }

    // 基本型の変数宣言/関数定義
    ASTNode *basicType = parseBasicTypeStatement(isStatic, isConst, isUnsigned);
    if (basicType) {
        // exportフラグが設定されている場合、関数宣言・変数宣言にフラグを設定
        if (isExported && (basicType->node_type == ASTNodeType::AST_FUNC_DECL ||
                           basicType->node_type == ASTNodeType::AST_VAR_DECL)) {
            basicType->is_exported = true;
            if (isDefaultExport) {
                basicType->is_default_export = true;
            }
        }
        return basicType;
    }

    // 制御フロー文
    ASTNode *controlFlow = parseControlFlowStatement();
    if (controlFlow)
        return controlFlow;

    // 式文または代入文
    return parseExpressionOrAssignmentStatement();
}

// 宣言文の処理
ASTNode *StatementParser::parseDeclarationStatement(bool isStatic, bool isConst,
                                                    bool isExported) {
    // import文
    if (parser_->check(TokenType::TOK_IMPORT)) {
        return parseImportStatement();
    }

    // main関数
    if (parser_->check(TokenType::TOK_MAIN)) {
        Token main_token = parser_->current_token_;
        parser_->advance();

        // main関数は型パラメータを持たない（将来的にはint
        // main<T>()も可能だが、現在は非対応）

        if (parser_->check(TokenType::TOK_LPAREN)) {
            ASTNode *func = parser_->parseFunctionDeclarationAfterName(
                "int", main_token.value);
            if (func && isExported) {
                func->is_exported = true;
            }
            return func;
        } else {
            parser_->error("Expected '(' after main");
            return nullptr;
        }
    }

    // typedef宣言
    if (parser_->check(TokenType::TOK_TYPEDEF)) {
        debug_msg(DebugMsgId::PARSE_TYPEDEF_START,
                  parser_->current_token_.line);
        ASTNode *typedef_node = parser_->parseTypedefDeclaration();
        if (typedef_node && isExported) {
            typedef_node->is_exported = true;
        }
        return typedef_node;
    }

    // struct宣言
    if (parser_->check(TokenType::TOK_STRUCT)) {
        debug_msg(DebugMsgId::PARSE_STRUCT_DECL_START,
                  parser_->current_token_.line);
        ASTNode *struct_node = parser_->parseStructDeclaration();
        if (struct_node && isExported) {
            struct_node->is_exported = true;
        }
        return struct_node;
    }

    // enum宣言
    if (parser_->check(TokenType::TOK_ENUM)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START,
                  parser_->current_token_.line);
        ASTNode *enum_node = parser_->parseEnumDeclaration();
        if (enum_node && isExported) {
            enum_node->is_exported = true;
        }
        return enum_node;
    }

    // interface宣言
    if (parser_->check(TokenType::TOK_INTERFACE)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START,
                  parser_->current_token_.line);
        ASTNode *interface_node = parser_->parseInterfaceDeclaration();
        if (interface_node && isExported) {
            interface_node->is_exported = true;
        }
        return interface_node;
    }

    // impl宣言
    if (parser_->check(TokenType::TOK_IMPL)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START,
                  parser_->current_token_.line);
        ASTNode *impl_node = parser_->parseImplDeclaration();
        if (impl_node && isExported) {
            impl_node->is_exported = true;
        }
        return impl_node;
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
    if (parser_->check(TokenType::TOK_DEFER)) {
        return parseDeferStatement();
    }
    if (parser_->check(TokenType::TOK_SWITCH)) {
        return parseSwitchStatement();
    }
    if (parser_->check(TokenType::TOK_MATCH)) {
        return parseMatchStatement();
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

    // v0.11.0: ジェネリック型のチェック（struct/enum）
    // 次のトークンが < なら、ジェネリック型の可能性がある
    if (parser_->check(TokenType::TOK_LT)) {
        // 構造体またはenumのジェネリック定義をチェック
        auto struct_it = parser_->struct_definitions_.find(type_name);
        auto enum_it = parser_->enum_definitions_.find(type_name);

        if (struct_it != parser_->struct_definitions_.end() &&
            struct_it->second.is_generic) {
            is_struct_type = true;
        } else if (enum_it != parser_->enum_definitions_.end() &&
                   enum_it->second.is_generic) {
            is_enum_type = true;
        }
    }

    // 既知の型でない場合、先読みして型宣言のパターンかチェック
    // パターン: TypeName identifier; または TypeName identifier = ...;
    // v0.11.0: TypeName identifier<T>(...) もチェック（ジェネリック関数）
    bool looks_like_type_declaration = false;
    if (!is_typedef && !is_struct_type && !is_interface_type &&
        !is_union_type && !is_enum_type) {
        // 先読みで次のトークンが識別子かチェック
        RecursiveLexer temp_lexer = parser_->lexer_;
        Token temp_current = parser_->current_token_;

        parser_->advance(); // 型名候補をスキップ

        // ポインタ修飾子をスキップ
        while (parser_->check(TokenType::TOK_MUL) ||
               parser_->check(TokenType::TOK_BIT_AND)) {
            parser_->advance();
        }

        // 次が識別子で、その後に ; または = または < または (
        // があれば型宣言の可能性が高い
        if (parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->advance(); // 識別子をスキップ

            // v0.11.0: <があれば型パラメータ付き関数の可能性
            if (parser_->check(TokenType::TOK_LT)) {
                // '<' から '>' までスキップ
                parser_->advance();
                while (!parser_->check(TokenType::TOK_GT) &&
                       !parser_->check(TokenType::TOK_EOF)) {
                    parser_->advance();
                }
                if (parser_->check(TokenType::TOK_GT)) {
                    parser_->advance();
                }
            }

            if (parser_->check(TokenType::TOK_SEMICOLON) ||
                parser_->check(TokenType::TOK_ASSIGN) ||
                parser_->check(TokenType::TOK_LPAREN)) {
                looks_like_type_declaration = true;
                // 実行時に型が解決される可能性があるため、型として扱う
                is_struct_type = true; // 構造体型として仮定
            }
        }

        // 元の位置に戻す
        parser_->lexer_ = temp_lexer;
        parser_->current_token_ = temp_current;
    }

    if (!is_typedef && !is_struct_type && !is_interface_type &&
        !is_union_type && !is_enum_type && !looks_like_type_declaration) {
        return nullptr; // この識別子は型ではない
    }

    // v0.11.0: 先読みして関数定義かチェック（ジェネリック対応版）
    // ジェネリック関数の戻り値型に型パラメータが含まれる場合
    // （例: Box<T> make_box<T>(...)）、型パラメータを事前に収集する
    RecursiveLexer temp_lexer = parser_->lexer_;
    Token temp_current = parser_->current_token_;

    parser_->advance(); // 型名をスキップ

    // v0.11.0: 戻り値型がジェネリック型の場合、型引数をスキップ
    // 例: Box<T> の <T> 部分
    if (parser_->check(TokenType::TOK_LT)) {
        int depth = 1;
        parser_->advance(); // '<' をスキップ

        while (depth > 0 && !parser_->check(TokenType::TOK_EOF)) {
            if (parser_->check(TokenType::TOK_LT)) {
                depth++;
            } else if (parser_->check(TokenType::TOK_GT)) {
                depth--;
            }
            if (depth > 0) {
                parser_->advance();
            }
        }

        if (parser_->check(TokenType::TOK_GT)) {
            parser_->advance(); // '>' をスキップ
        }
    }

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

    // 識別子が続くかチェック + 型パラメータの先読み
    bool is_function = false;
    std::vector<std::string> lookahead_type_params;
    bool has_lookahead_type_params = false;

    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->advance(); // 識別子をスキップ

        // v0.11.0: '<' があれば型パラメータ付き関数の可能性
        // 型パラメータを収集（戻り値型のパース前にスタックにプッシュするため）
        if (parser_->check(TokenType::TOK_LT)) {
            has_lookahead_type_params = true;
            parser_->advance(); // '<' をスキップ

            // 型パラメータを収集
            while (!parser_->check(TokenType::TOK_GT) &&
                   !parser_->check(TokenType::TOK_EOF)) {
                if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                    lookahead_type_params.push_back(
                        parser_->current_token_.value);
                    parser_->advance();

                    if (parser_->check(TokenType::TOK_COMMA)) {
                        parser_->advance(); // ',' をスキップ
                    }
                } else {
                    parser_->advance(); // その他のトークンをスキップ
                }
            }

            if (parser_->check(TokenType::TOK_GT)) {
                parser_->advance(); // '>' をスキップ
            }
        }

        // '(' があるかチェック
        if (parser_->check(TokenType::TOK_LPAREN)) {
            // 次のトークンを見て、関数定義かコンストラクタ呼び出しかを判断
            parser_->advance(); // '(' をスキップ

            // ')' なら引数なしのコンストラクタ or 引数なしの関数宣言
            // 型名（int, void等）なら関数宣言
            // それ以外（数値、文字列、変数名等）ならコンストラクタ呼び出し
            if (parser_->check(TokenType::TOK_RPAREN)) {
                // 空の括弧 - デフォルトでは関数宣言と見なす
                // ただし、後続に';'があればコンストラクタ呼び出しの可能性
                parser_->advance(); // ')' をスキップ
                if (parser_->check(TokenType::TOK_SEMICOLON)) {
                    // Point p(); のような構文 - コンストラクタ呼び出しの可能性
                    is_function = false;
                } else if (parser_->check(TokenType::TOK_LBRACE)) {
                    // Point p() { ... } - 関数定義
                    is_function = true;
                } else {
                    is_function = true; // デフォルトは関数
                }
            } else if (parser_->check(TokenType::TOK_INT) ||
                       parser_->check(TokenType::TOK_VOID) ||
                       parser_->check(TokenType::TOK_FLOAT) ||
                       parser_->check(TokenType::TOK_DOUBLE) ||
                       parser_->check(TokenType::TOK_STRING_TYPE) ||
                       parser_->check(TokenType::TOK_BOOL) ||
                       parser_->check(TokenType::TOK_LONG) ||
                       parser_->check(TokenType::TOK_SHORT) ||
                       parser_->check(TokenType::TOK_TINY) ||
                       parser_->check(TokenType::TOK_CONST) ||
                       parser_->check(TokenType::TOK_UNSIGNED) ||
                       parser_->check(TokenType::TOK_IDENTIFIER)) {
                // 型名が続く場合は関数定義
                // TOK_IDENTIFIERはtypedef型やstruct型の可能性がある
                is_function = true;
            } else if (parser_->check(TokenType::TOK_NUMBER) ||
                       parser_->check(TokenType::TOK_STRING) ||
                       parser_->check(TokenType::TOK_TRUE) ||
                       parser_->check(TokenType::TOK_FALSE)) {
                // リテラルが続く場合はコンストラクタ呼び出し
                is_function = false;
            } else {
                // その他の場合は関数定義とみなす（デフォルト動作）
                // これにより、曖昧なケースでは既存の動作を維持
                is_function = true;
            }
        }
    }

    // 元の位置に戻す
    parser_->lexer_ = temp_lexer;
    parser_->current_token_ = temp_current;

    if (is_function) {
        // v0.11.0: 型パラメータを事前にスタックにプッシュ
        // これにより、戻り値型に型パラメータ（例: Box<T>）を使用できる
        if (has_lookahead_type_params && !lookahead_type_params.empty()) {
            parser_->type_parameter_stack_.push_back(lookahead_type_params);
        }

        // 関数定義 - parseType()を使ってconst修飾子を含む完全な型情報を取得
        std::string return_type =
            parser_->parseType(); // const修飾子を含む完全な型
        ParsedTypeInfo return_type_info = parser_->getLastParsedTypeInfo();

        std::string function_name = parser_->advance().value;
        debug_msg(DebugMsgId::PARSE_FUNCTION_DECL_FOUND, function_name.c_str(),
                  return_type.c_str());

        // v0.11.0: 型パラメータのパース <T> または <T1, T2>
        // v0.11.0 Phase 1a: 複数インターフェース境界のサポート <T, A: Allocator
        // + Clone>
        std::vector<std::string> type_parameters;
        std::unordered_map<std::string, std::vector<std::string>>
            interface_bounds;
        bool is_generic = false;

        if (parser_->check(TokenType::TOK_LT)) {
            is_generic = true;
            parser_->advance(); // '<' を消費

            // 型パラメータのリストを解析
            do {
                if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                    parser_->error("Expected type parameter name after '<'");
                    // スタックのクリーンアップ
                    if (has_lookahead_type_params) {
                        parser_->type_parameter_stack_.pop_back();
                    }
                    return nullptr;
                }

                std::string param_name = parser_->current_token_.value;
                type_parameters.push_back(param_name);
                parser_->advance();

                // インターフェース境界のチェック: A: Allocator または A:
                // Allocator + Clone
                if (parser_->check(TokenType::TOK_COLON)) {
                    parser_->advance(); // ':' を消費

                    std::vector<std::string> bounds;
                    do {
                        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                            parser_->error("Expected interface name after ':' "
                                           "or '+' in type parameter bound");
                            if (has_lookahead_type_params) {
                                parser_->type_parameter_stack_.pop_back();
                            }
                            return nullptr;
                        }

                        bounds.push_back(parser_->current_token_.value);
                        parser_->advance();

                        // '+' があれば次のインターフェース境界を読む
                        if (parser_->check(TokenType::TOK_PLUS)) {
                            parser_->advance(); // '+' を消費
                        } else {
                            break;
                        }
                    } while (true);

                    interface_bounds[param_name] = bounds;
                }

                if (parser_->check(TokenType::TOK_COMMA)) {
                    parser_->advance(); // ',' を消費
                } else {
                    break;
                }
            } while (true);

            if (!parser_->check(TokenType::TOK_GT)) {
                parser_->error("Expected '>' after type parameters");
                // スタックのクリーンアップ
                if (has_lookahead_type_params) {
                    parser_->type_parameter_stack_.pop_back();
                }
                return nullptr;
            }
            parser_->advance(); // '>' を消費

            // 先読みでプッシュしたスタックを、実際にパースした型パラメータで更新
            if (has_lookahead_type_params) {
                parser_->type_parameter_stack_.pop_back();
                parser_->type_parameter_stack_.push_back(type_parameters);
            } else {
                // 先読みで検出できなかった場合（通常の非ジェネリック戻り値型）
                parser_->type_parameter_stack_.push_back(type_parameters);
            }
        } else if (has_lookahead_type_params) {
            // 先読みで検出したが実際にはなかった場合（エラー状態）
            // スタックをクリーンアップ
            parser_->type_parameter_stack_.pop_back();
        }

        ASTNode *func_node = parser_->parseFunctionDeclarationAfterName(
            return_type, function_name);

        // 型パラメータスタックをポップ
        if (is_generic) {
            parser_->type_parameter_stack_.pop_back();
        }

        // 型パラメータ情報を設定
        if (func_node && is_generic) {
            func_node->is_generic = true;
            func_node->type_parameters = type_parameters;
            func_node->interface_bounds = interface_bounds;
        }

        // 戻り値のconst情報を設定
        if (func_node && return_type_info.is_pointer) {
            func_node->is_pointee_const_qualifier =
                return_type_info.is_pointee_const;
        }
        return func_node;
    } else if (is_struct_type) {
        ASTNode *node = parser_->parseVariableDeclaration();
        applyDeclarationModifiers(node, isConst, isStatic);
        return node;
    } else if (is_interface_type) {
        // interface変数宣言
        debug_msg(DebugMsgId::PARSE_STRUCT_VAR_DECL_FOUND, type_name.c_str());
        std::string interface_type = parser_->advance().value;

        // ポインタ深度をチェック
        int pointer_depth = 0;
        bool is_pointer_const = false;
        while (parser_->check(TokenType::TOK_MUL)) {
            pointer_depth++;
            parser_->advance();
        }

        // ポインタの後のconst修飾子をチェック (T* const)
        if (pointer_depth > 0 && parser_->check(TokenType::TOK_CONST)) {
            is_pointer_const = true;
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

            // constポインタ情報を設定
            var_node->is_pointer_const_qualifier = is_pointer_const;
            if (isConst) {
                var_node->is_pointee_const_qualifier = true;
            }

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

        // v0.11.0: ジェネリックenum対応
        // ここではまだtype_nameしか分かっていないので、
        // 先に型全体をparseType()で解析する
        // まず、type_nameをスキップ（既にIDENTIFIERトークンにいる）
        parser_->advance(); // 'Option'をスキップ

        std::string enum_type = type_name;

        // ジェネリック型かチェック
        if (parser_->check(TokenType::TOK_LT)) {
            // parseType()的に型引数を解析
            parser_->advance(); // '<'

            parser_->type_parameter_stack_.push_back({});

            std::vector<std::string> type_arguments;
            do {
                std::string arg = parser_->parseType();
                if (arg.empty()) {
                    parser_->error("Expected type argument");
                    return nullptr;
                }
                type_arguments.push_back(arg);

                if (parser_->check(TokenType::TOK_COMMA)) {
                    parser_->advance();
                } else {
                    break;
                }
            } while (true);

            parser_->consume(TokenType::TOK_GT,
                             "Expected '>' after type arguments");
            parser_->type_parameter_stack_.pop_back();

            // インスタンス化
            parser_->instantiateGenericEnum(type_name, type_arguments);

            // インスタンス化された型名を生成
            enum_type = type_name;
            for (const auto &arg : type_arguments) {
                enum_type += "_" + arg;
            }
        }

        // ポインタ修飾子のチェック
        int pointer_depth = 0;
        bool is_pointer_const = false;
        while (parser_->check(TokenType::TOK_MUL)) {
            pointer_depth++;
            parser_->advance();

            // ポインタ自体のconst修飾子チェック（* const）
            if (parser_->check(TokenType::TOK_CONST)) {
                is_pointer_const = true;
                parser_->advance();
            }
        }

        // 参照修飾子のチェック
        bool is_reference = false;
        if (parser_->check(TokenType::TOK_AND)) {
            is_reference = true;
            parser_->advance();
        }

        // 配列の次元チェック（例: Color[5]）
        std::vector<ArrayDimension> dimensions;
        bool is_array = false;
        while (parser_->check(TokenType::TOK_LBRACKET)) {
            is_array = true;
            parser_->advance(); // consume '['

            if (parser_->check(TokenType::TOK_RBRACKET)) {
                // 動的配列 []
                dimensions.push_back(ArrayDimension(-1, true));
                parser_->advance(); // consume ']'
            } else {
                // サイズ指定あり [N]
                ASTNode *size_expr = parser_->parseExpression();
                if (size_expr->node_type == ASTNodeType::AST_NUMBER) {
                    int size = static_cast<int>(size_expr->int_value);
                    dimensions.push_back(ArrayDimension(size, false));
                } else if (size_expr->node_type == ASTNodeType::AST_VARIABLE) {
                    // 定数識別子による指定
                    dimensions.push_back(
                        ArrayDimension(-1, true, size_expr->name));
                } else {
                    dimensions.push_back(ArrayDimension(-1, true));
                }
                delete size_expr;

                parser_->consume(TokenType::TOK_RBRACKET,
                                 "Expected ']' after array size");
            }
        }

        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->error("Expected enum variable name");
            return nullptr;
        }

        // 最初の変数名
        std::string first_var_name = parser_->advance().value;

        // 複数変数宣言のためのベクター
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

        parser_->consume(TokenType::TOK_SEMICOLON,
                         "Expected ';' after enum variable declaration");

        // 単一変数か複数変数かに応じてノードを作成
        if (variables.size() == 1) {
            ASTNode *var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
            var_node->name = variables[0].first;
            var_node->type_name = enum_type;
            var_node->is_const = isConst;

            // 配列情報の設定
            if (is_array) {
                var_node->is_array = true;
                var_node->array_type_info =
                    ArrayTypeInfo(TYPE_ENUM, dimensions);
                var_node->type_info =
                    static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_ENUM);

                // type_nameに配列次元を追加
                for (const auto &dim : dimensions) {
                    var_node->type_name += "[";
                    if (!dim.is_dynamic && dim.size >= 0) {
                        var_node->type_name += std::to_string(dim.size);
                    }
                    var_node->type_name += "]";
                }
            }
            // ポインタと参照の情報を設定
            else if (pointer_depth > 0) {
                var_node->is_pointer = true;
                var_node->pointer_depth = pointer_depth;
                var_node->pointer_base_type_name = enum_type;
                var_node->pointer_base_type = TYPE_ENUM;
                var_node->type_info = TYPE_POINTER;

                // constポインタ情報を設定
                var_node->is_pointer_const_qualifier = is_pointer_const;
                if (isConst) {
                    var_node->is_pointee_const_qualifier = true;
                }

                for (int i = 0; i < pointer_depth; i++) {
                    var_node->type_name += "*";
                }
            } else {
                var_node->type_info = TYPE_ENUM;
            }

            if (is_reference) {
                var_node->is_reference = true;
                var_node->type_name += "&";
            }

            // 初期化式を設定
            if (variables[0].second) {
                var_node->init_expr = std::move(variables[0].second);
            }

            return var_node;
        } else {
            // 複数変数宣言: AST_MULTIPLE_VAR_DECLノードを作成
            ASTNode *multi_node =
                new ASTNode(ASTNodeType::AST_MULTIPLE_VAR_DECL);
            multi_node->type_name = enum_type;
            multi_node->is_const = isConst;
            multi_node->is_reference = is_reference;

            // ポインタ情報を設定
            if (pointer_depth > 0) {
                multi_node->is_pointer = true;
                multi_node->pointer_depth = pointer_depth;
                multi_node->pointer_base_type_name = enum_type;
                multi_node->pointer_base_type = TYPE_ENUM;
            }

            // 配列情報を設定
            if (is_array) {
                multi_node->is_array = true;
                multi_node->array_type_info =
                    ArrayTypeInfo(TYPE_ENUM, dimensions);
            }

            // 各変数をvar_declarations_に追加
            for (auto &var_pair : variables) {
                ASTNode *var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                var_node->name = var_pair.first;
                var_node->type_name = enum_type;
                var_node->is_const = isConst;
                var_node->is_reference = is_reference;

                // ポインタ情報をコピー
                if (pointer_depth > 0) {
                    var_node->is_pointer = true;
                    var_node->pointer_depth = pointer_depth;
                    var_node->pointer_base_type_name = enum_type;
                    var_node->pointer_base_type = TYPE_ENUM;
                    var_node->type_info = TYPE_POINTER;

                    var_node->is_pointer_const_qualifier = is_pointer_const;
                    if (isConst) {
                        var_node->is_pointee_const_qualifier = true;
                    }

                    for (int i = 0; i < pointer_depth; i++) {
                        var_node->type_name += "*";
                    }
                } else if (is_array) {
                    var_node->is_array = true;
                    var_node->array_type_info =
                        ArrayTypeInfo(TYPE_ENUM, dimensions);
                    var_node->type_info =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_ENUM);

                    for (const auto &dim : dimensions) {
                        var_node->type_name += "[";
                        if (!dim.is_dynamic && dim.size >= 0) {
                            var_node->type_name += std::to_string(dim.size);
                        }
                        var_node->type_name += "]";
                    }
                } else {
                    var_node->type_info = TYPE_ENUM;
                }

                // 初期化式を移動
                if (var_pair.second) {
                    var_node->init_expr = std::move(var_pair.second);
                }

                multi_node->children.push_back(
                    std::unique_ptr<ASTNode>(var_node));
            }

            return multi_node;
        }
    } else {
        ASTNode *node = parser_->parseVariableDeclaration();
        applyDeclarationModifiers(node, isConst, isStatic);
        return node;
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
                                     isUnsigned, is_reference, pointer_depth);
    }

    // ポインタ型の後のconst修飾子をチェック (T* const の場合)
    bool is_pointer_const = false;
    if (pointer_depth > 0 && parser_->check(TokenType::TOK_CONST)) {
        parser_->advance(); // consume const
        is_pointer_const = true;
    }

    // 関数宣言か通常の変数宣言かチェック
    if (parser_->check(TokenType::TOK_IDENTIFIER) ||
        parser_->check(TokenType::TOK_MAIN) ||
        parser_->check(TokenType::TOK_UNDERSCORE)) {
        Token name_token = parser_->current_token_;
        parser_->advance(); // consume identifier/main/underscore

        // v0.11.0: 型パラメータのチェック <T> または <T1, T2>
        // v0.11.0 Phase 1a: 複数インターフェース境界のサポート <T, A: Allocator
        // + Clone>
        std::vector<std::string> type_parameters;
        std::unordered_map<std::string, std::vector<std::string>>
            interface_bounds;
        bool is_generic = false;

        if (parser_->check(TokenType::TOK_LT)) {
            is_generic = true;
            parser_->advance(); // '<' を消費

            // 型パラメータのリストを解析
            do {
                if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                    parser_->error("Expected type parameter name after '<'");
                    return nullptr;
                }

                std::string param_name = parser_->current_token_.value;
                type_parameters.push_back(param_name);
                parser_->advance();

                // インターフェース境界のチェック
                if (parser_->check(TokenType::TOK_COLON)) {
                    parser_->advance(); // ':' を消費

                    std::vector<std::string> bounds;
                    do {
                        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                            parser_->error("Expected interface name after ':' "
                                           "or '+' in type parameter bound");
                            return nullptr;
                        }

                        bounds.push_back(parser_->current_token_.value);
                        parser_->advance();

                        if (parser_->check(TokenType::TOK_PLUS)) {
                            parser_->advance(); // '+' を消費
                        } else {
                            break;
                        }
                    } while (true);

                    interface_bounds[param_name] = bounds;
                }

                if (parser_->check(TokenType::TOK_COMMA)) {
                    parser_->advance(); // ',' を消費
                } else {
                    break;
                }
            } while (true);

            if (!parser_->check(TokenType::TOK_GT)) {
                parser_->error("Expected '>' after type parameters");
                return nullptr;
            }
            parser_->advance(); // '>' を消費
        }

        if (parser_->check(TokenType::TOK_LPAREN)) {
            // これは関数定義
            // constがポインタの指し先に適用される場合、戻り値型の名前に含める
            std::string full_return_type = type_name;
            if (isConst && pointer_depth > 0) {
                full_return_type = "const " + full_return_type;
            }
            ASTNode *func_node = parser_->parseFunctionDeclarationAfterName(
                full_return_type, name_token.value);

            // 型パラメータ情報を設定
            if (func_node && is_generic) {
                func_node->is_generic = true;
                func_node->type_parameters = type_parameters;
                func_node->interface_bounds = interface_bounds;
            }

            // 戻り値のconst情報を設定
            if (func_node && isConst && pointer_depth > 0) {
                func_node->is_pointee_const_qualifier = true;
            }
            return func_node;
        } else {
            // 変数宣言: type identifier [, identifier2, ...] [= expr];
            return parseVariableDeclarationList(
                name_token.value, type_name, base_type_name, base_type_info,
                declared_type_info, pointer_depth, isStatic, isConst,
                isUnsigned, is_reference, is_pointer_const);
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
    bool isConst, bool isUnsigned, bool is_reference, int pointer_depth) {
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

    // ポインタ配列の場合、ポインタフラグを設定
    if (pointer_depth > 0) {
        node->is_pointer = true;
        node->pointer_depth = pointer_depth;
        node->pointer_base_type = base_type_info;
        node->pointer_base_type_name = base_type_name;
    }

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
    bool isUnsigned, bool is_reference, bool is_pointer_const) {
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
            // const修飾子を設定
            if (isConst) {
                // 型の前のconst: const T* → 指し先がconst
                node->is_pointee_const_qualifier = true;
            }
            if (is_pointer_const) {
                // ポインタの後のconst: T* const → ポインタ自体がconst
                node->is_pointer_const_qualifier = true;
            }
        } else {
            // 型情報を設定
            node->type_info = declared_type_info;
            // 非ポインタ型の場合のみis_constを設定
            node->is_const = isConst;
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
                // ポインタ型の場合、isConstは指し先のconstを意味する
                if (isConst) {
                    var_node->is_pointee_const_qualifier = true;
                }
            } else {
                var_node->type_info = node->type_info;
                // 非ポインタ型の場合のみis_constを設定
                var_node->is_const = isConst;
            }

            if (var.second) {
                var_node->init_expr = std::move(var.second);
            }

            node->children.push_back(std::unique_ptr<ASTNode>(var_node));
        }

        return node;
    }
}

void StatementParser::applyDeclarationModifiers(ASTNode *node, bool isConst,
                                                bool isStatic) {
    if (!node) {
        return;
    }

    auto apply_to_single = [&](ASTNode *target) {
        if (!target) {
            return;
        }
        if (isStatic) {
            target->is_static = true;
        }
        if (isConst) {
            if (target->is_pointer && !target->is_reference) {
                target->is_pointee_const_qualifier = true;
            } else {
                target->is_const = true;
            }
        }
    };

    apply_to_single(node);

    if (node->node_type == ASTNodeType::AST_MULTIPLE_VAR_DECL) {
        for (auto &child : node->children) {
            apply_to_single(child.get());
        }
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
    ASTNode *condition_node = parser_->parseExpression();
    if_node->condition = std::unique_ptr<ASTNode>(condition_node);
    if (parser_->debug_mode_) {
        std::fprintf(
            stderr, "[IF_DEBUG] condition parsed node=%p type=%d\n",
            static_cast<void *>(condition_node),
            condition_node ? static_cast<int>(condition_node->node_type) : -1);
        std::fflush(stderr);
    }

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

/**
 * @brief defer文を解析
 * @return 解析されたASTdefer文ノード
 *
 * 構文: defer statement;
 * スコープ終了時に実行される文を登録（LIFO順）
 */
ASTNode *StatementParser::parseDeferStatement() {
    parser_->advance(); // consume 'defer'
    ASTNode *defer_node = new ASTNode(ASTNodeType::AST_DEFER_STMT);

    // defer対象の文を解析
    defer_node->body = std::unique_ptr<ASTNode>(parser_->parseStatement());

    return defer_node;
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

/**
 * @brief switch文を解析
 * @return 解析されたASTswitch文ノード
 *
 * 構文:
 * switch (expr) {
 *     case (value1) { stmt1; }
 *     case (value2 || value3) { stmt2; }
 *     case (10...20) { stmt3; }
 *     else { stmt4; }
 * }
 */
ASTNode *StatementParser::parseSwitchStatement() {
    Token switch_token = parser_->advance(); // consume 'switch'
    ASTNode *switch_node = new ASTNode(ASTNodeType::AST_SWITCH_STMT);
    switch_node->location.line = switch_token.line;
    switch_node->location.column = switch_token.column;

    // switch対象の式を解析
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after switch");
    switch_node->switch_expr =
        std::unique_ptr<ASTNode>(parser_->parseExpression());
    parser_->consume(TokenType::TOK_RPAREN,
                     "Expected ')' after switch expression");

    // switch本体（case節のリスト）
    parser_->consume(TokenType::TOK_LBRACE,
                     "Expected '{' after switch expression");

    // case節を解析
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        if (parser_->check(TokenType::TOK_CASE)) {
            switch_node->cases.push_back(
                std::unique_ptr<ASTNode>(parseCaseClause()));
        } else if (parser_->check(TokenType::TOK_ELSE)) {
            // else節（default相当）
            parser_->advance(); // consume 'else'
            if (!parser_->check(TokenType::TOK_LBRACE)) {
                parser_->error("Expected '{' after else in switch");
                break;
            }
            switch_node->else_body =
                std::unique_ptr<ASTNode>(parseCompoundStatement());
            break; // elseは最後なので終了
        } else {
            parser_->error("Expected 'case' or 'else' in switch body");
            break;
        }
    }

    parser_->consume(TokenType::TOK_RBRACE, "Expected '}' after switch body");
    return switch_node;
}

/**
 * @brief case節を解析
 * @return 解析されたASTcase節ノード
 *
 * 構文:
 * case (value) { body }
 * case (value1 || value2) { body }
 * case (start...end) { body }
 */
ASTNode *StatementParser::parseCaseClause() {
    Token case_token = parser_->advance(); // consume 'case'
    ASTNode *case_node = new ASTNode(ASTNodeType::AST_CASE_CLAUSE);
    case_node->location.line = case_token.line;
    case_node->location.column = case_token.column;

    // case条件を解析
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after case");

    // OR結合された値または範囲式を解析
    do {
        ASTNode *value = parseCaseValue();
        case_node->case_values.push_back(std::unique_ptr<ASTNode>(value));
    } while (parser_->match(TokenType::TOK_OR)); // || で結合

    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after case value");

    // case本体を解析（parseCompoundStatementが{を消費するので、ここでは消費しない）
    if (!parser_->check(TokenType::TOK_LBRACE)) {
        parser_->error("Expected '{' after case condition");
        return case_node;
    }
    case_node->case_body = std::unique_ptr<ASTNode>(parseCompoundStatement());

    return case_node;
}

/**
 * @brief case値（範囲式を含む）を解析
 * @return 解析されたAST値ノードまたは範囲式ノード
 *
 * Note: parseComparison()を使用することで、論理OR演算子(||)を
 *       case値の区切りとして使用できるようにしています
 */
ASTNode *StatementParser::parseCaseValue() {
    ASTNode *start = parser_->parseComparison();

    // 範囲演算子（...）をチェック
    if (parser_->check(TokenType::TOK_RANGE)) {
        parser_->advance(); // consume '...'
        ASTNode *end = parser_->parseComparison();

        ASTNode *range_node = new ASTNode(ASTNodeType::AST_RANGE_EXPR);
        range_node->range_start = std::unique_ptr<ASTNode>(start);
        range_node->range_end = std::unique_ptr<ASTNode>(end);
        return range_node;
    }

    return start;
}

/**
 * @brief import文を解析
 * @return 解析されたASTインポート文ノード
 *
 * 構文: import module.path.name;
 *
 * import文は外部モジュールから関数や定数をインポートします。
 * サポートされる形式:
 * - import stdlib.math.basic;
 * - import stdlib.math.basic as math;
 * - import stdlib.math.basic { func1, func2 };
 * - import stdlib.math.basic { func1 as f1, func2 };
 */
ASTNode *StatementParser::parseImportStatement() {
    Token import_token = parser_->advance(); // consume 'import'

    ASTNode *import_node = new ASTNode(ASTNodeType::AST_IMPORT_STMT);
    import_node->location.line = import_token.line;
    import_node->location.column = import_token.column;

    std::string module_path;

    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        // ドット記法のモジュールパス
        module_path = parser_->current_token_.value;
        parser_->advance();

        // ドット記法で続くパスを結合
        while (parser_->check(TokenType::TOK_DOT)) {
            parser_->advance(); // consume '.'

            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected identifier after '.' in import path");
                delete import_node;
                return nullptr;
            }

            module_path += ".";
            module_path += parser_->current_token_.value;
            parser_->advance();
        }
    } else {
        parser_->error("Expected module path after 'import'");
        delete import_node;
        return nullptr;
    }

    import_node->import_path = module_path;

    // asキーワードでモジュール全体のエイリアスをチェック
    if (parser_->check(TokenType::TOK_IDENTIFIER) &&
        parser_->current_token_.value == "as") {
        parser_->advance(); // consume 'as'

        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->error("Expected identifier after 'as'");
            delete import_node;
            return nullptr;
        }

        std::string alias = parser_->current_token_.value;
        parser_->advance();

        // モジュール全体のエイリアス
        import_node->import_aliases["*"] = alias;
    }
    // 中括弧で個別インポートをチェック
    else if (parser_->check(TokenType::TOK_LBRACE)) {
        parser_->advance(); // consume '{'

        // 個別項目をパース
        while (!parser_->check(TokenType::TOK_RBRACE) &&
               !parser_->check(TokenType::TOK_EOF)) {
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected identifier in import list");
                delete import_node;
                return nullptr;
            }

            std::string item_name = parser_->current_token_.value;
            parser_->advance();

            // 個別項目のエイリアスをチェック
            if (parser_->check(TokenType::TOK_IDENTIFIER) &&
                parser_->current_token_.value == "as") {
                parser_->advance(); // consume 'as'

                if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                    parser_->error("Expected identifier after 'as'");
                    delete import_node;
                    return nullptr;
                }

                std::string alias = parser_->current_token_.value;
                parser_->advance();

                import_node->import_items.push_back(item_name);
                import_node->import_aliases[item_name] = alias;
            } else {
                import_node->import_items.push_back(item_name);
            }

            // カンマをチェック
            if (parser_->check(TokenType::TOK_COMMA)) {
                parser_->advance();
            } else if (!parser_->check(TokenType::TOK_RBRACE)) {
                parser_->error("Expected ',' or '}' in import list");
                delete import_node;
                return nullptr;
            }
        }

        if (!parser_->check(TokenType::TOK_RBRACE)) {
            parser_->error("Expected '}' after import list");
            delete import_node;
            return nullptr;
        }
        parser_->advance(); // consume '}'
    }

    // セミコロンを消費
    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after import statement");

    // 名前フィールドにもパスを設定（後方互換性のため）
    import_node->name = module_path;

    // v0.11.0: パース時にimportを処理し、型定義を取り込む
    // これにより、import後の変数宣言で型が認識される
    try {
        parser_->processImport(module_path, import_node->import_items);
    } catch (const std::exception &e) {
        parser_->error("Import failed: " + std::string(e.what()));
    }

    return import_node;
}

/**
 * @brief match文を解析
 * @return 解析されたASTmatch文ノード
 *
 * 構文:
 * match (expr) {
 *     VariantName(binding) => { body }
 *     VariantName => { body }
 *     _ => { body }
 * }
 */
ASTNode *StatementParser::parseMatchStatement() {
    Token match_token = parser_->advance(); // consume 'match'
    ASTNode *match_node = new ASTNode(ASTNodeType::AST_MATCH_STMT);
    match_node->location.line = match_token.line;
    match_node->location.column = match_token.column;

    // match対象の式を解析
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after match");
    match_node->match_expr =
        std::unique_ptr<ASTNode>(parser_->parseExpression());
    parser_->consume(TokenType::TOK_RPAREN,
                     "Expected ')' after match expression");

    // match本体（arm節のリスト）
    parser_->consume(TokenType::TOK_LBRACE,
                     "Expected '{' after match expression");

    // match arm を解析
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        MatchArm arm = parseMatchArm();
        match_node->match_arms.push_back(std::move(arm));
    }

    parser_->consume(TokenType::TOK_RBRACE, "Expected '}' after match body");
    return match_node;
}

/**
 * @brief match armを解析
 * @return 解析されたMatchArm構造体
 *
 * 構文:
 * VariantName(binding) => { body }
 * VariantName => { body }
 * _ => { body }
 */
MatchArm StatementParser::parseMatchArm() {
    MatchArm arm;

    // パターンを解析
    if (parser_->check(TokenType::TOK_UNDERSCORE)) {
        // ワイルドカードパターン
        arm.pattern_type = PatternType::PATTERN_WILDCARD;
        parser_->advance(); // consume '_'
    } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        // Enumバリアントパターン
        arm.pattern_type = PatternType::PATTERN_ENUM_VARIANT;
        arm.variant_name = parser_->current_token_.value;
        parser_->advance(); // consume variant name

        // バインディングをチェック（オプション）
        if (parser_->check(TokenType::TOK_LPAREN)) {
            parser_->advance(); // consume '('

            // バインディング変数を解析（識別子または_）
            if (parser_->check(TokenType::TOK_UNDERSCORE)) {
                // ワイルドカードバインディング（値を無視）
                arm.bindings.push_back("_");
                parser_->advance(); // consume '_'
            } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                arm.bindings.push_back(parser_->current_token_.value);
                parser_->advance(); // consume binding name
            }

            parser_->consume(TokenType::TOK_RPAREN,
                             "Expected ')' after binding");
        }
    } else {
        parser_->error("Expected pattern in match arm");
        return arm;
    }

    // Fat arrow (=>)
    parser_->consume(TokenType::TOK_FAT_ARROW,
                     "Expected '=>' after match pattern");

    // 本体を解析
    // 単一式または複合文（{}）
    if (parser_->check(TokenType::TOK_LBRACE)) {
        arm.body = std::unique_ptr<ASTNode>(parseCompoundStatement());
    } else {
        // 単一式の場合
        arm.body = std::unique_ptr<ASTNode>(parser_->parseExpression());
        // セミコロンは省略可能だが、あれば消費
        if (parser_->check(TokenType::TOK_SEMICOLON)) {
            parser_->advance();
        }
    }

    return arm;
}
