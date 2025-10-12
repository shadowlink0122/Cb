// Primary Expression Parser - プライマリ式解析を担当
//
// このファイルは、式解析の最も基本的な要素を処理します：
// - リテラル（数値、文字列、文字、真偽値、nullptr）
// - 識別子（変数、enum値アクセス）
// - 関数呼び出し
// - 括弧式
// - 配列リテラル
// - 構造体リテラル

#include "primary_expression_parser.h"
#include "../recursive_parser.h"

PrimaryExpressionParser::PrimaryExpressionParser(RecursiveParser *parser)
    : parser_(parser) {}

ASTNode *PrimaryExpressionParser::parsePrimary() {
    if (parser_->check(TokenType::TOK_NUMBER)) {
        Token token = parser_->advance();
        ASTNode *node = new ASTNode(ASTNodeType::AST_NUMBER);

        std::string literal = token.value;
        node->literal_text = literal;

        // サフィックス解析（f/F, d/D, q/Q）
        char suffix = '\0';
        if (!literal.empty()) {
            char last_char = literal.back();
            if (last_char == 'f' || last_char == 'F' || last_char == 'd' ||
                last_char == 'D' || last_char == 'q' || last_char == 'Q') {
                suffix = last_char;
                literal.pop_back();
            }
        }

        // 指数表記の小文字e/E対応のため、末尾サフィックスを除外した後で判定
        auto contains_decimal = [](const std::string &value) {
            return value.find('.') != std::string::npos;
        };
        auto contains_exponent = [](const std::string &value) {
            return value.find('e') != std::string::npos ||
                   value.find('E') != std::string::npos;
        };

        bool is_float_literal = contains_decimal(literal) ||
                                contains_exponent(literal) || suffix != '\0';

        try {
            if (is_float_literal) {
                node->is_float_literal = true;

                if (suffix == 'f' || suffix == 'F') {
                    node->literal_type = TYPE_FLOAT;
                    node->type_info = TYPE_FLOAT;
                    node->double_value = std::stod(literal);
                    node->quad_value =
                        static_cast<long double>(node->double_value);
                } else if (suffix == 'q' || suffix == 'Q') {
                    node->literal_type = TYPE_QUAD;
                    node->type_info = TYPE_QUAD;
                    node->quad_value = std::stold(literal);
                    node->double_value = static_cast<double>(node->quad_value);
                } else {
                    // デフォルトはdouble（サフィックスなし、またはd/D）
                    node->literal_type = TYPE_DOUBLE;
                    node->type_info = TYPE_DOUBLE;
                    node->double_value = std::stod(literal);
                    node->quad_value =
                        static_cast<long double>(node->double_value);
                }

                // 整数値としても保持（必要に応じて使用）
                node->int_value = static_cast<int64_t>(node->double_value);
            } else {
                node->literal_type = TYPE_INT;
                node->type_info = TYPE_INT;
                node->int_value = std::stoll(literal); // 64ビット整数対応
                node->double_value = static_cast<double>(node->int_value);
                node->quad_value = static_cast<long double>(node->int_value);
            }
        } catch (const std::exception &e) {
            parser_->error("Invalid number: " + token.value);
            delete node;
            return nullptr;
        }

        return node;
    }

    if (parser_->check(TokenType::TOK_STRING)) {
        Token token = parser_->advance();
        ASTNode *node = new ASTNode(ASTNodeType::AST_STRING_LITERAL);
        node->str_value = token.value;
        return node;
    }

    if (parser_->check(TokenType::TOK_CHAR)) {
        Token token = parser_->advance();
        ASTNode *node = new ASTNode(ASTNodeType::AST_NUMBER);
        // 文字リテラルをASCII値として処理
        if (!token.value.empty()) {
            node->int_value = static_cast<int>(token.value[0]);
        } else {
            node->int_value = 0;
        }
        return node;
    }

    if (parser_->check(TokenType::TOK_TRUE) ||
        parser_->check(TokenType::TOK_FALSE)) {
        Token token = parser_->advance();
        ASTNode *node =
            new ASTNode(ASTNodeType::AST_NUMBER); // bool値も数値として扱う
        node->int_value = (token.type == TokenType::TOK_TRUE) ? 1 : 0;
        return node;
    }

    if (parser_->check(TokenType::TOK_NULLPTR) ||
        parser_->check(TokenType::TOK_NULL)) {
        Token token = parser_->advance();
        ASTNode *node = new ASTNode(ASTNodeType::AST_NULLPTR);
        parser_->setLocation(node, token.line, token.column);
        return node;
    }

    if (parser_->check(TokenType::TOK_SELF)) {
        Token token = parser_->advance();
        ASTNode *node = new ASTNode(ASTNodeType::AST_IDENTIFIER);
        node->name = "self";
        parser_->setLocation(node, token.line, token.column);
        return node;
    }

    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        Token token = parser_->advance();

        // 無名変数 (_) のチェック
        if (token.value == "_") {
            ASTNode *node = new ASTNode(ASTNodeType::AST_DISCARD_VARIABLE);
            node->name = "_";
            node->is_discard = true;
            parser_->setLocation(node, token.line, token.column);
            return node;
        }

        // enum値アクセス（EnumName::member）または修飾名（namespace::function）をチェック
        if (parser_->check(TokenType::TOK_SCOPE_RESOLUTION)) {
            // 修飾名のパス全体を構築: outer::inner::multiply
            std::string qualified_name = token.value;

            while (parser_->check(TokenType::TOK_SCOPE_RESOLUTION)) {
                parser_->advance(); // consume '::'

                if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                    parser_->error("Expected identifier after '::'");
                    return nullptr;
                }

                qualified_name += "::" + parser_->current_token_.value;
                parser_->advance(); // consume identifier
            }

            // 修飾名関数呼び出し: namespace::function() or
            // outer::inner::multiply()
            if (parser_->check(TokenType::TOK_LPAREN)) {
                parser_->advance(); // consume '('

                ASTNode *call_node = new ASTNode(ASTNodeType::AST_FUNC_CALL);
                call_node->name = qualified_name; // 完全修飾名

                // 引数リストの解析
                if (!parser_->check(TokenType::TOK_RPAREN)) {
                    do {
                        ASTNode *arg = parser_->parseExpression();
                        call_node->arguments.push_back(
                            std::unique_ptr<ASTNode>(arg));
                    } while (parser_->match(TokenType::TOK_COMMA));
                }

                parser_->consume(TokenType::TOK_RPAREN,
                                 "Expected ')' after function arguments");

                parser_->setLocation(call_node, token.line, token.column);
                return call_node;
            }

            // enum値アクセス: EnumName::member (単一の::のみ)
            // qualified_nameが "EnumName::member" の形式の場合
            size_t last_colon = qualified_name.rfind("::");
            if (last_colon != std::string::npos) {
                std::string enum_name = qualified_name.substr(0, last_colon);
                std::string enum_member = qualified_name.substr(last_colon + 2);

                ASTNode *enum_access =
                    new ASTNode(ASTNodeType::AST_ENUM_ACCESS);
                enum_access->enum_name = enum_name;
                enum_access->enum_member = enum_member;
                parser_->setLocation(enum_access, token.line, token.column);

                return enum_access;
            }
        }

        // 関数呼び出しをチェック
        if (parser_->check(TokenType::TOK_LPAREN)) {
            parser_->advance(); // consume '('

            ASTNode *call_node = new ASTNode(ASTNodeType::AST_FUNC_CALL);
            call_node->name = token.value;

            // 引数リストの解析
            if (!parser_->check(TokenType::TOK_RPAREN)) {
                do {
                    ASTNode *arg = parser_->parseExpression();
                    call_node->arguments.push_back(
                        std::unique_ptr<ASTNode>(arg));
                } while (parser_->match(TokenType::TOK_COMMA));
            }

            if (parser_->debug_mode_) {
                std::fprintf(
                    stderr, "[CALL_DEBUG] Parsed call %s with %zu args\n",
                    call_node->name.c_str(), call_node->arguments.size());
            }

            parser_->consume(TokenType::TOK_RPAREN,
                             "Expected ')' after function arguments");

            // チェーン呼び出しのサポート: func()() 形式
            // 最初の呼び出しが関数ポインタを返す場合、続けて呼び出し可能
            while (parser_->check(TokenType::TOK_LPAREN)) {
                parser_->advance(); // consume '('

                // チェーン呼び出しノードを作成
                ASTNode *chained_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
                chained_call->left = std::unique_ptr<ASTNode>(
                    call_node); // 前の呼び出し結果を左側に

                // 引数リストの解析
                if (!parser_->check(TokenType::TOK_RPAREN)) {
                    do {
                        ASTNode *arg = parser_->parseExpression();
                        chained_call->arguments.push_back(
                            std::unique_ptr<ASTNode>(arg));
                    } while (parser_->match(TokenType::TOK_COMMA));
                }

                parser_->consume(
                    TokenType::TOK_RPAREN,
                    "Expected ')' after chained function arguments");

                call_node = chained_call; // 次のイテレーションのために更新
            }

            return call_node;
        }
        // 配列アクセスは parsePostfix で処理
        else {
            ASTNode *node = new ASTNode(ASTNodeType::AST_VARIABLE);
            node->name = token.value;
            parser_->setLocation(node, token.line, token.column);
            return node;
        }
    }

    // 括弧式の処理
    if (parser_->check(TokenType::TOK_LPAREN)) {
        parser_->advance(); // consume '('
        ASTNode *expr = parser_->parseExpression();
        parser_->consume(TokenType::TOK_RPAREN, "Expected ')'");
        return expr;
    }

    // 配列リテラルの処理
    if (parser_->check(TokenType::TOK_LBRACKET)) {
        return parseArrayLiteral();
    }

    // 構造体リテラルの処理 {member: value, ...}
    if (parser_->check(TokenType::TOK_LBRACE)) {
        return parseStructLiteral();
    }

    // 無名関数（ラムダ式）の処理: 型 func(params) { body }
    // 例: int func(int x) { return x * 2; }
    if (parser_->check(TokenType::TOK_INT) ||
        parser_->check(TokenType::TOK_VOID) ||
        parser_->check(TokenType::TOK_LONG) ||
        parser_->check(TokenType::TOK_SHORT) ||
        parser_->check(TokenType::TOK_TINY) ||
        parser_->check(TokenType::TOK_FLOAT) ||
        parser_->check(TokenType::TOK_DOUBLE) ||
        parser_->check(TokenType::TOK_BOOL) ||
        parser_->check(TokenType::TOK_STRING_TYPE) ||
        parser_->check(TokenType::TOK_CHAR_TYPE)) {

        // 先読みして無名関数かチェック
        RecursiveLexer temp_lexer = parser_->lexer_;
        Token temp_current = parser_->current_token_;

        parser_->advance(); // 型をスキップ

        // `func` キーワードがあれば無名関数
        if (parser_->check(TokenType::TOK_FUNC)) {
            // 状態を戻してparseLambdaを呼ぶ
            parser_->lexer_ = temp_lexer;
            parser_->current_token_ = temp_current;
            return parseLambda();
        }

        // 無名関数でない場合は状態を戻す
        parser_->lexer_ = temp_lexer;
        parser_->current_token_ = temp_current;
    }

    parser_->error("Unexpected token");
    return nullptr;
}

ASTNode *PrimaryExpressionParser::parseStructLiteral() {
    parser_->consume(TokenType::TOK_LBRACE, "Expected '{'");

    ASTNode *struct_literal = new ASTNode(ASTNodeType::AST_STRUCT_LITERAL);

    // 空の構造体リテラル {}
    if (parser_->check(TokenType::TOK_RBRACE)) {
        parser_->advance(); // consume '}'
        return struct_literal;
    }

    // 最初の要素をチェックして、名前付きか位置ベースかを判断
    bool is_named_initialization = false;

    // 先読みして名前付き初期化かチェック
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        RecursiveLexer temp_lexer = parser_->lexer_;
        Token temp_current = parser_->current_token_;
        parser_->advance();
        if (parser_->check(TokenType::TOK_COLON)) {
            is_named_initialization = true;
        }
        parser_->lexer_ = temp_lexer;
        parser_->current_token_ = temp_current;
    }

    if (is_named_initialization) {
        // 名前付き初期化: {name: "Bob", age: 25}
        while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
            // メンバ名の解析
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected member name in struct literal");
                return nullptr;
            }

            std::string member_name = parser_->current_token_.value;
            parser_->advance();

            parser_->consume(TokenType::TOK_COLON,
                             "Expected ':' after member name");

            // メンバ値の解析
            ASTNode *member_value = parser_->parseExpression();

            // メンバ代入ノードを作成（name: valueの形で保存）
            ASTNode *member_init = new ASTNode(ASTNodeType::AST_ASSIGN);
            member_init->name = member_name;
            member_init->right = std::unique_ptr<ASTNode>(member_value);

            struct_literal->arguments.push_back(
                std::unique_ptr<ASTNode>(member_init));

            if (parser_->check(TokenType::TOK_COMMA)) {
                parser_->advance(); // consume ','
            } else if (!parser_->check(TokenType::TOK_RBRACE)) {
                parser_->error("Expected ',' or '}' in struct literal");
                return nullptr;
            }
        }
    } else {
        // 位置ベース初期化: {25, "Bob"}
        while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
            ASTNode *value = parser_->parseExpression();
            struct_literal->arguments.push_back(
                std::unique_ptr<ASTNode>(value));

            if (parser_->check(TokenType::TOK_COMMA)) {
                parser_->advance(); // consume ','
            } else if (!parser_->check(TokenType::TOK_RBRACE)) {
                parser_->error("Expected ',' or '}' in struct literal");
                return nullptr;
            }
        }
    }

    parser_->consume(TokenType::TOK_RBRACE,
                     "Expected '}' after struct literal");
    return struct_literal;
}

ASTNode *PrimaryExpressionParser::parseArrayLiteral() {
    parser_->consume(TokenType::TOK_LBRACKET,
                     "Expected '[' at start of array literal");

    ASTNode *array_literal = new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);

    // 空の配列リテラル []
    if (parser_->check(TokenType::TOK_RBRACKET)) {
        parser_->advance(); // consume ']'
        return array_literal;
    }

    // 配列要素を解析
    while (!parser_->check(TokenType::TOK_RBRACKET) && !parser_->isAtEnd()) {
        ASTNode *element;

        if (parser_->check(TokenType::TOK_LBRACE)) {
            // struct literal要素: {25, "Alice"}
            element = parseStructLiteral();
        } else {
            // 通常の式要素
            element = parser_->parseExpression();
        }

        array_literal->arguments.push_back(std::unique_ptr<ASTNode>(element));

        if (parser_->check(TokenType::TOK_COMMA)) {
            parser_->advance(); // consume ','
        } else if (!parser_->check(TokenType::TOK_RBRACKET)) {
            parser_->error("Expected ',' or ']' in array literal");
            return nullptr;
        }
    }

    parser_->consume(TokenType::TOK_RBRACKET,
                     "Expected ']' after array literal");
    return array_literal;
}

ASTNode *PrimaryExpressionParser::parseLambda() {
    // 戻り値の型を解析
    std::string return_type = parser_->parseType();

    // 'func' キーワードを消費
    if (!parser_->check(TokenType::TOK_FUNC)) {
        parser_->error("Expected 'func' keyword in lambda expression");
        return nullptr;
    }
    parser_->advance();

    // '(' を消費
    parser_->consume(TokenType::TOK_LPAREN,
                     "Expected '(' after 'func' in lambda expression");

    // 無名関数ノードを作成
    ASTNode *lambda = new ASTNode(ASTNodeType::AST_LAMBDA_EXPR);
    lambda->is_lambda = true;
    lambda->lambda_return_type_name = return_type;

    // 型名をTypeInfoに変換（parser_->getTypeInfoFromStringを使用）
    lambda->lambda_return_type = parser_->getTypeInfoFromString(return_type);
    lambda->type_info = lambda->lambda_return_type; // ASTNodeのtype_infoも設定

    // 内部識別子を生成
    extern std::string generate_lambda_name();
    lambda->internal_name = generate_lambda_name();
    lambda->name = lambda->internal_name;

    // パラメータリストを解析
    if (!parser_->check(TokenType::TOK_RPAREN)) {
        do {
            // パラメータの型を解析
            std::string param_type = parser_->parseType();

            // パラメータ名を解析
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected parameter name in lambda expression");
                delete lambda;
                return nullptr;
            }

            std::string param_name = parser_->current_token_.value;
            parser_->advance();

            // パラメータノードを作成
            ASTNode *param = new ASTNode(ASTNodeType::AST_PARAM_DECL);
            param->name = param_name;
            param->type_name = param_type;
            param->type_info = parser_->getTypeInfoFromString(param_type);

            lambda->lambda_params.push_back(std::unique_ptr<ASTNode>(param));

        } while (parser_->match(TokenType::TOK_COMMA));
    }

    // ')' を消費
    parser_->consume(TokenType::TOK_RPAREN,
                     "Expected ')' after lambda parameters");

    // '{' を消費
    parser_->consume(TokenType::TOK_LBRACE, "Expected '{' before lambda body");

    // 文のリストノードを作成（関数本体と同じパターン）
    ASTNode *body_node = new ASTNode(ASTNodeType::AST_STMT_LIST);

    // 文の解析
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        ASTNode *stmt = parser_->parseStatement();
        if (stmt != nullptr) {
            body_node->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }

    // '}' を消費
    parser_->consume(TokenType::TOK_RBRACE, "Expected '}' after lambda body");

    // 本体をlambda_bodyに設定
    lambda->lambda_body = std::unique_ptr<ASTNode>(body_node);

    // parametersフィールドにlambda_paramsの参照を設定（インタプリタで使用）
    // lambda_paramsとparametersで同じデータを参照する
    lambda->parameters.reserve(lambda->lambda_params.size());
    for (size_t i = 0; i < lambda->lambda_params.size(); ++i) {
        // unique_ptrなので所有権を移動
        lambda->parameters.push_back(std::move(lambda->lambda_params[i]));
    }
    lambda->lambda_params.clear(); // 移動したのでクリア

    // ラムダの直接実行をサポート: int func(int x){return x;}(10) 形式
    // チェーン呼び出しもサポート: func()()() 形式
    ASTNode *result = lambda;
    while (parser_->check(TokenType::TOK_LPAREN)) {
        parser_->advance(); // consume '('

        // ラムダ即座実行ノードを作成
        ASTNode *call_node = new ASTNode(ASTNodeType::AST_FUNC_CALL);
        call_node->left =
            std::unique_ptr<ASTNode>(result); // ラムダまたは前の呼び出し結果
        call_node->is_lambda_call = true; // ラムダ呼び出しフラグ

        // 引数リストの解析
        if (!parser_->check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode *arg = parser_->parseExpression();
                call_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
            } while (parser_->match(TokenType::TOK_COMMA));
        }

        parser_->consume(TokenType::TOK_RPAREN,
                         "Expected ')' after lambda call arguments");

        result = call_node; // 次のチェーンのために更新
    }

    return result;
}
