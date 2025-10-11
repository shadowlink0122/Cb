#include "interface_parser.h"
#include "../../../common/debug.h"
#include "../recursive_parser.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * @file interface_parser.cpp
 * @brief interface/impl宣言を担当するInterfaceParserクラスの実装
 * @note recursive_parser.cppから移行（Phase 6-2）
 */

// ========================================
// コンストラクタ
// ========================================

InterfaceParser::InterfaceParser(RecursiveParser *parser) : parser_(parser) {}

// ========================================
// interface/impl宣言の解析
// ========================================

// parseInterfaceDeclarationとparseImplDeclarationの実装は以下に追加
ASTNode *InterfaceParser::parseInterfaceDeclaration() {
    parser_->consume(TokenType::TOK_INTERFACE, "Expected 'interface'");

    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected interface name");
        return nullptr;
    }

    std::string interface_name = parser_->current_token_.value;
    parser_->advance(); // interface名をスキップ

    parser_->consume(TokenType::TOK_LBRACE,
                     "Expected '{' after interface name");

    InterfaceDefinition interface_def(interface_name);

    // メソッド宣言の解析
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        // メソッドの戻り値型を解析
        std::string return_type = parser_->parseType();
        ParsedTypeInfo return_parsed = parser_->getLastParsedTypeInfo();

        if (return_type.empty()) {
            parser_->error(
                "Expected return type in interface method declaration");
            return nullptr;
        }

        // メソッド名を解析（予約キーワードも許可）
        std::string method_name;
        if (parser_->check(TokenType::TOK_IDENTIFIER)) {
            method_name = parser_->current_token_.value;
            parser_->advance();
        } else if (parser_->check(TokenType::TOK_PRINT) ||
                   parser_->check(TokenType::TOK_PRINTLN) ||
                   parser_->check(TokenType::TOK_PRINTF)) {
            // 予約キーワードだが、メソッド名として許可
            method_name = parser_->current_token_.value;
            parser_->advance();
        } else {
            parser_->error("Expected method name in interface declaration");
            return nullptr;
        } // パラメータリストの解析
        parser_->consume(TokenType::TOK_LPAREN,
                         "Expected '(' after method name");

        TypeInfo resolved_return_type =
            parser_->resolveParsedTypeInfo(return_parsed);
        if (resolved_return_type == TYPE_UNKNOWN) {
            resolved_return_type = parser_->getTypeInfoFromString(return_type);
        }

        InterfaceMember method(method_name, resolved_return_type,
                               return_parsed.is_unsigned);

        // パラメータの解析
        if (!parser_->check(TokenType::TOK_RPAREN)) {
            do {
                // パラメータの型
                std::string param_type = parser_->parseType();
                ParsedTypeInfo param_parsed = parser_->getLastParsedTypeInfo();
                if (param_type.empty()) {
                    parser_->error("Expected parameter type");
                    return nullptr;
                }

                // パラメータ名（オプション）
                std::string param_name = "";
                if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                    param_name = parser_->current_token_.value;
                    parser_->advance();
                }

                TypeInfo param_type_info =
                    parser_->resolveParsedTypeInfo(param_parsed);
                if (param_type_info == TYPE_UNKNOWN) {
                    param_type_info =
                        parser_->getTypeInfoFromString(param_type);
                }

                method.add_parameter(param_name, param_type_info,
                                     param_parsed.is_unsigned);

                if (!parser_->check(TokenType::TOK_COMMA)) {
                    break;
                }
                parser_->advance(); // consume comma
            } while (!parser_->check(TokenType::TOK_RPAREN));
        }

        parser_->consume(TokenType::TOK_RPAREN,
                         "Expected ')' after parameters");
        parser_->consume(TokenType::TOK_SEMICOLON,
                         "Expected ';' after interface method declaration");

        interface_def.methods.push_back(method);
    }

    parser_->consume(TokenType::TOK_RBRACE,
                     "Expected '}' after interface methods");
    // セミコロンは不要（オプション）
    if (parser_->check(TokenType::TOK_SEMICOLON)) {
        parser_->advance(); // consume optional semicolon
    }

    // interface定義をパーサー内に保存
    parser_->interface_definitions_[interface_name] = interface_def;

    // ASTノードを作成
    ASTNode *node = new ASTNode(ASTNodeType::AST_INTERFACE_DECL);
    node->name = interface_name;
    parser_->setLocation(node, parser_->current_token_);

    // interface定義情報をASTノードに保存
    for (const auto &method : interface_def.methods) {
        ASTNode *method_node = new ASTNode(ASTNodeType::AST_FUNC_DECL);
        method_node->name = method.name;
        method_node->type_info = method.return_type;
        method_node->is_unsigned = method.return_is_unsigned;
        method_node->return_types.push_back(method.return_type);

        // パラメータ情報を保存
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            const auto &param = method.parameters[i];
            ASTNode *param_node = new ASTNode(ASTNodeType::AST_PARAM_DECL);
            param_node->name = param.first;
            param_node->type_info = param.second;
            param_node->is_unsigned = method.get_parameter_is_unsigned(i);
            method_node->arguments.push_back(
                std::unique_ptr<ASTNode>(param_node));
        }

        node->arguments.push_back(std::unique_ptr<ASTNode>(method_node));
    }

    return node;
}
ASTNode *InterfaceParser::parseImplDeclaration() {
    parser_->consume(TokenType::TOK_IMPL, "Expected 'impl'");

    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected identifier after 'impl'");
        return nullptr;
    }

    std::string first_name = parser_->current_token_.value;
    parser_->advance();

    // パターン判定: impl Struct {} または impl Interface for Struct {}
    bool is_constructor_impl = false;
    std::string interface_name;
    std::string struct_name;

    if (parser_->check(TokenType::TOK_LBRACE)) {
        // パターン1: impl Struct {} (コンストラクタ/デストラクタ用)
        is_constructor_impl = true;
        struct_name = first_name;
        interface_name = ""; // interface名なし
    } else if (parser_->check(TokenType::TOK_FOR) ||
               (parser_->check(TokenType::TOK_IDENTIFIER) &&
                parser_->current_token_.value == "for")) {
        // パターン2: impl Interface for Struct {} (通常のメソッド実装)
        interface_name = first_name;

        // ★ 課題1の解決: 存在しないinterfaceを実装しようとする場合のエラー検出
        if (parser_->interface_definitions_.find(interface_name) ==
            parser_->interface_definitions_.end()) {
            parser_->error(
                "Interface '" + interface_name +
                "' is not defined. Please declare the interface before "
                "implementing it.");
            return nullptr;
        }

        parser_->advance(); // consume 'for'

        if (!parser_->check(TokenType::TOK_IDENTIFIER) &&
            !parser_->check(TokenType::TOK_STRING_TYPE) &&
            !parser_->check(TokenType::TOK_INT) &&
            !parser_->check(TokenType::TOK_LONG) &&
            !parser_->check(TokenType::TOK_SHORT) &&
            !parser_->check(TokenType::TOK_TINY) &&
            !parser_->check(TokenType::TOK_BOOL) &&
            !parser_->check(TokenType::TOK_CHAR_TYPE)) {
            parser_->error(
                "Expected type name (struct or primitive type) after 'for'");
            return nullptr;
        }

        if (parser_->check(TokenType::TOK_STRING_TYPE)) {
            struct_name = "string";
        } else if (parser_->check(TokenType::TOK_INT)) {
            struct_name = "int";
        } else if (parser_->check(TokenType::TOK_LONG)) {
            struct_name = "long";
        } else if (parser_->check(TokenType::TOK_SHORT)) {
            struct_name = "short";
        } else if (parser_->check(TokenType::TOK_TINY)) {
            struct_name = "tiny";
        } else if (parser_->check(TokenType::TOK_BOOL)) {
            struct_name = "bool";
        } else if (parser_->check(TokenType::TOK_CHAR_TYPE)) {
            struct_name = "char";
        } else {
            struct_name = parser_->current_token_
                              .value; // 識別子（構造体名またはtypedef名）
        }
        parser_->advance();

        // 生の配列型チェック - 配列記法が続く場合はエラー
        if (parser_->check(TokenType::TOK_LBRACKET)) {
            parser_->error("Cannot implement interface for raw array type '" +
                           struct_name +
                           "[...]'. Use typedef to define array type first.");
            return nullptr;
        }
    } else {
        parser_->error(
            "Expected '{' or 'for' after struct name in impl declaration");
        return nullptr;
    }

    parser_->consume(TokenType::TOK_LBRACE,
                     "Expected '{' after type name in impl declaration");

    ImplDefinition impl_def(interface_name, struct_name);
    std::vector<std::unique_ptr<ASTNode>>
        method_nodes; // 所有権は一時的に保持し、最終的にASTへ移動
    std::vector<std::unique_ptr<ASTNode>> static_var_nodes; // impl static変数

    // メソッド実装の解析
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        // デストラクタのチェック (~self)
        if (is_constructor_impl && parser_->check(TokenType::TOK_BIT_NOT)) {
            parser_->advance(); // consume '~'

            if (!parser_->check(TokenType::TOK_SELF)) {
                parser_->error(
                    "Expected 'self' after '~' in destructor declaration");
                return nullptr;
            }
            parser_->advance(); // consume 'self'

            parser_->consume(TokenType::TOK_LPAREN,
                             "Expected '(' after '~self'");
            parser_->consume(TokenType::TOK_RPAREN,
                             "Expected ')' after '~self('");

            // デストラクタ本体の解析
            ASTNode *destructor_body = parser_->parseCompoundStatement();
            if (!destructor_body) {
                parser_->error("Expected destructor body");
                return nullptr;
            }

            // デストラクタノードを作成
            ASTNode *destructor = new ASTNode(ASTNodeType::AST_DESTRUCTOR_DECL);
            destructor->is_destructor = true;
            destructor->constructor_struct_name = struct_name;
            destructor->body.reset(destructor_body);
            parser_->setLocation(destructor, parser_->current_token_);

            impl_def.set_destructor(destructor);
            method_nodes.push_back(std::unique_ptr<ASTNode>(destructor));

            debug_msg(DebugMsgId::PARSE_VAR_DECL, struct_name.c_str(),
                      "destructor");
            continue;
        }

        // コンストラクタのチェック (self)
        if (is_constructor_impl && parser_->check(TokenType::TOK_SELF)) {
            parser_->advance(); // consume 'self'

            parser_->consume(TokenType::TOK_LPAREN,
                             "Expected '(' after 'self'");

            // パラメータリストの解析
            std::vector<std::unique_ptr<ASTNode>> parameters;
            if (!parser_->check(TokenType::TOK_RPAREN)) {
                do {
                    // パラメータ型
                    std::string param_type = parser_->parseType();
                    ParsedTypeInfo param_parsed =
                        parser_->getLastParsedTypeInfo();

                    // パラメータ名
                    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                        parser_->error(
                            "Expected parameter name in constructor");
                        return nullptr;
                    }
                    Token param_name = parser_->advance();

                    // パラメータノードを作成
                    ASTNode *param = new ASTNode(ASTNodeType::AST_PARAM_DECL);
                    param->name = param_name.value;
                    param->type_name = param_type;
                    param->type_info = param_parsed.base_type_info;
                    param->is_pointer = param_parsed.is_pointer;
                    param->pointer_depth = param_parsed.pointer_depth;
                    param->is_reference = param_parsed.is_reference;
                    param->is_unsigned = param_parsed.is_unsigned;
                    param->is_const = param_parsed.is_const;
                    param->is_pointer_const_qualifier =
                        param_parsed.is_pointer_const;
                    param->is_pointee_const_qualifier =
                        param_parsed.is_const && param_parsed.is_pointer;
                    parser_->setLocation(param, param_name);

                    parameters.push_back(std::unique_ptr<ASTNode>(param));
                } while (parser_->match(TokenType::TOK_COMMA));
            }

            parser_->consume(TokenType::TOK_RPAREN,
                             "Expected ')' after constructor parameters");

            // コンストラクタ本体の解析
            ASTNode *constructor_body = parser_->parseCompoundStatement();
            if (!constructor_body) {
                parser_->error("Expected constructor body");
                return nullptr;
            }

            // コンストラクタノードを作成
            ASTNode *constructor =
                new ASTNode(ASTNodeType::AST_CONSTRUCTOR_DECL);
            constructor->is_constructor = true;
            constructor->constructor_struct_name = struct_name;
            constructor->parameters = std::move(parameters);
            constructor->body.reset(constructor_body);
            parser_->setLocation(constructor, parser_->current_token_);

            impl_def.add_constructor(constructor);
            method_nodes.push_back(std::unique_ptr<ASTNode>(constructor));

            debug_msg(DebugMsgId::PARSE_VAR_DECL, struct_name.c_str(),
                      "constructor");
            continue;
        }

        // static修飾子のチェック（impl staticの可能性）
        if (parser_->check(TokenType::TOK_STATIC)) {
            parser_->advance(); // consume 'static'

            // 次にconst修飾子が続く可能性もある
            bool is_const_static = false;
            if (parser_->check(TokenType::TOK_CONST)) {
                is_const_static = true;
                parser_->advance(); // consume 'const'
            }

            // 型名を取得
            std::string var_type = parser_->parseType();
            if (var_type.empty()) {
                parser_->error("Expected type after 'static' in impl block");
                return nullptr;
            }

            // 変数名を取得
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected variable name after type in impl "
                               "static declaration");
                return nullptr;
            }
            std::string var_name = parser_->current_token_.value;
            parser_->advance();

            // 初期化式の解析（オプション）
            std::unique_ptr<ASTNode> init_expr;
            if (parser_->check(TokenType::TOK_ASSIGN)) {
                parser_->advance(); // consume '='
                ASTNode *expr_raw = parser_->parseExpression();
                if (!expr_raw) {
                    parser_->error("Expected expression after '=' in impl "
                                   "static variable initialization");
                    return nullptr;
                }
                init_expr.reset(expr_raw);
            }

            parser_->consume(
                TokenType::TOK_SEMICOLON,
                "Expected ';' after impl static variable declaration");

            // impl static変数ノードを作成
            ASTNode *static_var = new ASTNode(ASTNodeType::AST_VAR_DECL);
            static_var->name = var_name;
            static_var->type_name = var_type;
            static_var->type_info = parser_->getTypeInfoFromString(var_type);
            static_var->is_static = true;
            static_var->is_impl_static = true;
            static_var->is_const = is_const_static;
            if (init_expr) {
                static_var->init_expr = std::move(init_expr);
            }
            parser_->setLocation(static_var, parser_->current_token_);

            static_var_nodes.push_back(std::unique_ptr<ASTNode>(static_var));
            debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(),
                      "impl_static_variable");

            continue; // 次の宣言へ
        }

        // impl Struct {} の場合、コンストラクタ/デストラクタ以外は禁止
        if (is_constructor_impl) {
            parser_->error(
                "impl Struct {} can only contain constructors (self), "
                "destructor (~self), and static variables. "
                "For regular methods, use 'impl Interface for Struct'");
            return nullptr;
        }

        // private修飾子のチェック
        bool is_private_method = false;
        if (parser_->check(TokenType::TOK_PRIVATE)) {
            is_private_method = true;
            parser_->advance(); // consume 'private'
        }

        // メソッド実装をパース（関数宣言として）
        // impl内では戻り値の型から始まるメソッド定義
        std::string return_type = parser_->parseType();
        if (return_type.empty()) {
            parser_->error("Expected return type in method implementation");
            return nullptr;
        }

        // メソッド名を解析（予約キーワードも許可）
        std::string method_name;
        if (parser_->check(TokenType::TOK_IDENTIFIER)) {
            method_name = parser_->current_token_.value;
            parser_->advance();
        } else if (parser_->check(TokenType::TOK_PRINT) ||
                   parser_->check(TokenType::TOK_PRINTLN) ||
                   parser_->check(TokenType::TOK_PRINTF)) {
            // 予約キーワードだが、メソッド名として許可
            method_name = parser_->current_token_.value;
            parser_->advance();
        } else {
            parser_->error("Expected method name in method implementation");
            return nullptr;
        }

        // 関数宣言として解析
        ASTNode *method_impl_raw = parser_->parseFunctionDeclarationAfterName(
            return_type, method_name);
        if (method_impl_raw) {
            std::unique_ptr<ASTNode> method_impl(method_impl_raw);
            // DEBUG: parser_->debug_print removed
            // privateフラグを設定
            method_impl->is_private_method = is_private_method;
            // privateメソッドの場合はinterface署名チェックをスキップ
            if (!is_private_method) {
                // ★ 課題2の解決: メソッド署名の不一致の検出
                auto interface_it =
                    parser_->interface_definitions_.find(interface_name);
                if (interface_it != parser_->interface_definitions_.end()) {
                    bool method_found = false;
                    for (const auto &interface_method :
                         interface_it->second.methods) {
                        if (interface_method.name == method_name) {
                            method_found = true;
                            auto format_type = [](TypeInfo type,
                                                  bool is_unsigned_flag) {
                                std::string base = type_info_to_string(type);
                                if (is_unsigned_flag) {
                                    return std::string("unsigned ") + base;
                                }
                                return base;
                            };

                            TypeInfo expected_return_type_info =
                                interface_method.return_type;
                            bool expected_return_unsigned =
                                interface_method.return_is_unsigned;

                            TypeInfo actual_return_type_info = TYPE_UNKNOWN;
                            if (!method_impl->return_types.empty()) {
                                actual_return_type_info =
                                    method_impl->return_types[0];
                            } else {
                                actual_return_type_info =
                                    parser_->getTypeInfoFromString(return_type);
                            }
                            bool actual_return_unsigned =
                                method_impl->is_unsigned;

                            if (expected_return_type_info !=
                                    actual_return_type_info ||
                                expected_return_unsigned !=
                                    actual_return_unsigned) {
                                parser_->error(
                                    "Method signature mismatch: Expected "
                                    "return type '" +
                                    format_type(expected_return_type_info,
                                                expected_return_unsigned) +
                                    "' but got '" +
                                    format_type(actual_return_type_info,
                                                actual_return_unsigned) +
                                    "' for method '" + method_name + "'");
                                return nullptr;
                            }
                            // 引数の数をチェック
                            if (interface_method.parameters.size() !=
                                method_impl->parameters.size()) {
                                parser_->error(
                                    "Method signature mismatch: Expected " +
                                    std::to_string(
                                        interface_method.parameters.size()) +
                                    " parameter(s) but got " +
                                    std::to_string(
                                        method_impl->parameters.size()) +
                                    " for method '" + method_name + "'");
                                return nullptr;
                            }
                            // 引数の型をチエック
                            for (size_t i = 0;
                                 i < interface_method.parameters.size(); ++i) {
                                TypeInfo expected_param_type =
                                    interface_method.parameters[i].second;
                                bool expected_param_unsigned =
                                    interface_method.get_parameter_is_unsigned(
                                        i);
                                TypeInfo actual_param_type =
                                    method_impl->parameters[i]->type_info;
                                bool actual_param_unsigned =
                                    method_impl->parameters[i]->is_unsigned;

                                if (expected_param_type != actual_param_type ||
                                    expected_param_unsigned !=
                                        actual_param_unsigned) {
                                    parser_->error(
                                        "Method signature mismatch: "
                                        "Parameter " +
                                        std::to_string(i + 1) +
                                        " expected type '" +
                                        format_type(expected_param_type,
                                                    expected_param_unsigned) +
                                        "' but got '" +
                                        format_type(actual_param_type,
                                                    actual_param_unsigned) +
                                        "' for method '" + method_name + "'");
                                    return nullptr;
                                }
                            }
                            break;
                        }
                    }
                    if (!method_found) {
                        // 警告:
                        // interfaceに定義されていないメソッドが実装されている
                        std::cerr << "[WARNING] Method '" << method_name
                                  << "' is implemented but not declared in "
                                     "interface '"
                                  << interface_name << "'" << std::endl;
                    }
                }
            }

            // メソッド情報を保存（ImplDefinitionにはポインタのみ保持）
            impl_def.add_method(method_impl.get());
            method_nodes.push_back(std::move(method_impl));
            debug_msg(DebugMsgId::PARSE_VAR_DECL, method_name.c_str(),
                      "impl_method");
        }
    }

    parser_->consume(TokenType::TOK_RBRACE, "Expected '}' after impl methods");
    // セミコロンは不要（オプション）
    if (parser_->check(TokenType::TOK_SEMICOLON)) {
        parser_->advance(); // consume optional semicolon
    }

    // ★ interfaceの全メソッドが実装されているかチェック
    // （impl Struct {} の場合はスキップ）
    if (!is_constructor_impl) {
        auto interface_it =
            parser_->interface_definitions_.find(interface_name);
        if (interface_it != parser_->interface_definitions_.end()) {
            for (const auto &interface_method : interface_it->second.methods) {
                bool implemented = false;
                for (const auto *impl_method : impl_def.methods) {
                    if (impl_method->name == interface_method.name) {
                        implemented = true;
                        break;
                    }
                }
                if (!implemented) {
                    parser_->error("Incomplete implementation: Method '" +
                                   interface_method.name +
                                   "' declared in interface '" +
                                   interface_name + "' is not implemented");
                    return nullptr;
                }
            }
        }

        // ★ 課題3の解決: 重複impl定義の検出
        for (const auto &existing_impl : parser_->impl_definitions_) {
            if (existing_impl.interface_name == interface_name &&
                existing_impl.struct_name == struct_name) {
                parser_->error("Duplicate implementation: Interface '" +
                               interface_name +
                               "' is already implemented for struct '" +
                               struct_name + "'");
                return nullptr;
            }
        }
    }

    // impl定義を保存（ポインタ参照のみ保持）
    parser_->impl_definitions_.push_back(impl_def);

    // ASTノードを作成
    ASTNode *node = new ASTNode(ASTNodeType::AST_IMPL_DECL);
    node->name = interface_name + "_for_" + struct_name;
    node->type_name = struct_name;         // struct名を保存
    node->interface_name = interface_name; // interface名を保存
    node->struct_name = struct_name;       // struct名を明示的に保存
    parser_->setLocation(node, parser_->current_token_);

    // impl static変数の所有権をASTノードに移動
    for (auto &static_var : static_var_nodes) {
        node->impl_static_variables.push_back(std::move(static_var));
    }

    // implメソッドの所有権をASTノードに移動
    for (auto &method_node : method_nodes) {
        node->arguments.push_back(std::move(method_node));
    }

    return node;
}
