#include "statement_parser.h"
#include "../recursive_parser.h"
#include <stdexcept>
#include <memory>

/**
 * @file statement_parser.cpp
 * @brief 文解析を担当するStatementParserクラスの実装
 * @note recursive_parser.cppから移行
 * @note Phase 5-4-2で完全実装を追加
 */

// ========================================
// コンストラクタ
// ========================================

StatementParser::StatementParser(RecursiveParser* parser) 
    : parser_(parser) {}

// ========================================
// 文解析メインエントリ
// ========================================

/**
 * @brief 文を解析（メインエントリポイント）
 * @return 解析されたAST文ノード
 * 
 * トークンの種類に応じて適切な解析メソッドを呼び出します
 */
ASTNode* StatementParser::parseStatement() {
    // static修飾子のチェック
    bool isStatic = false;
    if (parser_->check(TokenType::TOK_STATIC)) {
        debug_msg(DebugMsgId::PARSE_STATIC_MODIFIER, parser_->current_token_.line, parser_->current_token_.column);
        isStatic = true;
        parser_->advance(); // consume 'static'
    }
    
    // const修飾子のチェック
    bool isConst = false;
    if (parser_->check(TokenType::TOK_CONST)) {
        debug_msg(DebugMsgId::PARSE_CONST_MODIFIER, parser_->current_token_.line, parser_->current_token_.column);
        isConst = true;
        parser_->advance(); // consume 'const'
    }
    
    // DEBUG: 現在のトークンを出力
    std::string token_type_str = std::to_string(static_cast<int>(parser_->current_token_.type));
    debug_msg(DebugMsgId::PARSE_CURRENT_TOKEN, parser_->current_token_.value.c_str(), token_type_str.c_str());
    
    // main関数の場合の特別処理
    if (parser_->check(TokenType::TOK_MAIN)) {
        Token main_token = parser_->current_token_;
        parser_->advance(); // consume 'main'
        
        if (parser_->check(TokenType::TOK_LPAREN)) {
            // main() 関数の定義
            return parser_->parseFunctionDeclarationAfterName("int", main_token.value);
        } else {
            parser_->error("Expected '(' after main");
            return nullptr;
        }
    }
    
    // typedef宣言の処理
    if (parser_->check(TokenType::TOK_TYPEDEF)) {
        debug_msg(DebugMsgId::PARSE_TYPEDEF_START, parser_->current_token_.line);
        return parser_->parseTypedefDeclaration();
    }
    
    // struct宣言の処理
    if (parser_->check(TokenType::TOK_STRUCT)) {
        debug_msg(DebugMsgId::PARSE_STRUCT_DECL_START, parser_->current_token_.line);
        return parser_->parseStructDeclaration();
    }
    
    // enum宣言の処理
    if (parser_->check(TokenType::TOK_ENUM)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START, parser_->current_token_.line);
        return parser_->parseEnumDeclaration();
    }
    
    // interface宣言の処理
    if (parser_->check(TokenType::TOK_INTERFACE)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START, parser_->current_token_.line); // 適切なデバッグIDに変更する必要がある
        return parser_->parseInterfaceDeclaration();
    }
    
    // impl宣言の処理
    if (parser_->check(TokenType::TOK_IMPL)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START, parser_->current_token_.line); // 適切なデバッグIDに変更する必要がある
        return parser_->parseImplDeclaration();
    }
    
    // typedef型変数宣言の処理
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        std::string potential_type = parser_->current_token_.value;
        
        // 構造体配列戻り値関数の早期検出: Type[size] function_name()
        bool is_array_return_function = false;
        if ((parser_->typedef_map_.find(potential_type) != parser_->typedef_map_.end() || 
             parser_->struct_definitions_.find(potential_type) != parser_->struct_definitions_.end())) {
            
            // Type[...] function_name() のパターンをチェック
            RecursiveLexer temp_lexer = parser_->lexer_;
            Token temp_current = parser_->current_token_;
            
            parser_->advance(); // Type
            if (parser_->check(TokenType::TOK_LBRACKET)) {
                parser_->advance(); // [
                // 配列サイズをスキップ
                int bracket_count = 1;
                while (bracket_count > 0 && !parser_->isAtEnd()) {
                    if (parser_->check(TokenType::TOK_LBRACKET)) bracket_count++;
                    else if (parser_->check(TokenType::TOK_RBRACKET)) bracket_count--;
                    parser_->advance();
                }
                // ]の後に識別子、その後に(があれば配列戻り値関数
                if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                    parser_->advance(); // function_name
                    if (parser_->check(TokenType::TOK_LPAREN)) {
                        is_array_return_function = true;
                    }
                }
            }
            
            // 元の位置に戻す
            parser_->lexer_ = temp_lexer;
            parser_->current_token_ = temp_current;
        }
        
        if (is_array_return_function) {
            // 配列戻り値関数として処理
            std::string return_type = parser_->advance().value; // 型名
            
            // 配列部分を戻り値型に追加: Type[size]
            return_type += parser_->advance().value; // '['
            while (!parser_->check(TokenType::TOK_RBRACKET) && !parser_->isAtEnd()) {
                return_type += parser_->advance().value; // 配列サイズ
            }
            if (parser_->check(TokenType::TOK_RBRACKET)) {
                return_type += parser_->advance().value; // ']'
            }
            
            std::string function_name = parser_->advance().value; // 関数名
            debug_msg(DebugMsgId::PARSE_FUNCTION_DECL_FOUND, function_name.c_str(), return_type.c_str());
            return parser_->parseFunctionDeclarationAfterName(return_type, function_name);
        }
        
        // typedef型または構造体型の可能性をチェック
        bool is_typedef = parser_->typedef_map_.find(potential_type) != parser_->typedef_map_.end();
        bool is_struct_type = parser_->struct_definitions_.find(potential_type) != parser_->struct_definitions_.end();
        bool is_interface_type = parser_->interface_definitions_.find(potential_type) != parser_->interface_definitions_.end();
        
        debug_msg(DebugMsgId::PARSE_TYPE_CHECK, potential_type.c_str(), 
                  is_typedef ? "true" : "false", is_struct_type ? "true" : "false");

        if (is_typedef || is_struct_type || is_interface_type) {
            debug_msg(DebugMsgId::PARSE_TYPEDEF_OR_STRUCT_TYPE_FOUND, potential_type.c_str());
            // 簡単な先読み: 現在の位置を保存
            RecursiveLexer temp_lexer = parser_->lexer_;
            Token temp_current = parser_->current_token_;
            
            parser_->advance(); // 型名をスキップ
            
            bool is_function = false;
            if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                debug_msg(DebugMsgId::PARSE_IDENTIFIER_AFTER_TYPE, parser_->current_token_.value.c_str());
                parser_->advance(); // 識別子をスキップ
                if (parser_->check(TokenType::TOK_LPAREN)) {
                    debug_msg(DebugMsgId::PARSE_FUNCTION_DETECTED, "");
                    is_function = true;
                } else if (parser_->check(TokenType::TOK_LBRACKET)) {
                    // 配列型かもしれないが、配列戻り値の関数の可能性もあるので更にチェック
                    debug_msg(DebugMsgId::PARSE_ARRAY_DETECTED, "");
                    
                    // 配列の括弧をスキップ: [2]
                    parser_->advance(); // consume '['
                    while (!parser_->check(TokenType::TOK_RBRACKET) && !parser_->isAtEnd()) {
                        parser_->advance(); // 配列サイズの式をスキップ
                    }
                    if (parser_->check(TokenType::TOK_RBRACKET)) {
                        parser_->advance(); // consume ']'
                    }
                    
                    // 次が識別子かつその後に'('があれば関数（配列戻り値）
                    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                        parser_->advance(); // 関数名をスキップ
                        if (parser_->check(TokenType::TOK_LPAREN)) {
                            debug_msg(DebugMsgId::PARSE_FUNCTION_DETECTED, "Array return function");
                            is_function = true;
                        } else {
                            is_function = false;
                        }
                    } else {
                        is_function = false;
                    }
                }
            }
            
            // 元の位置に戻す
            parser_->lexer_ = temp_lexer;
            parser_->current_token_ = temp_current;
            
            if (is_function) {
                // これは戻り値型が構造体またはtypedefの関数宣言（配列戻り値の可能性もある）
                std::string return_type = parser_->advance().value; // 型名を取得
                
                // 配列戻り値の場合: Person[2] function_name() の処理
                if (parser_->check(TokenType::TOK_LBRACKET)) {
                    // 配列部分を戻り値型に追加
                    return_type += parser_->advance().value; // '['
                    while (!parser_->check(TokenType::TOK_RBRACKET) && !parser_->isAtEnd()) {
                        return_type += parser_->advance().value; // 配列サイズ
                    }
                    if (parser_->check(TokenType::TOK_RBRACKET)) {
                        return_type += parser_->advance().value; // ']'
                    }
                }
                
                std::string function_name = parser_->advance().value; // 関数名を取得
                debug_msg(DebugMsgId::PARSE_FUNCTION_DECL_FOUND, function_name.c_str(), return_type.c_str());
                return parser_->parseFunctionDeclarationAfterName(return_type, function_name);
            } else {
                // これは構造体またはtypedef型変数宣言
                if (is_struct_type) {
                    debug_msg(DebugMsgId::PARSE_STRUCT_VAR_DECL_FOUND, potential_type.c_str());
                    // 構造体変数宣言
                    std::string struct_type = parser_->advance().value;
                    
                    // ポインタまたは参照チェック: Point* pp; または Point& rp;
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
                            parser_->error("Expected variable name after pointer/reference type");
                            return nullptr;
                        }
                        
                        std::string var_name = parser_->advance().value;
                        
                        ASTNode* var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
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
                            var_node->init_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
                        }
                        
                        parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after pointer/reference variable declaration");
                        return var_node;
                    }
                    
                    // 配列チェック: Person[3] people; または Person people;
                    if (parser_->check(TokenType::TOK_LBRACKET)) {
                        // struct配列宣言: Person[3] people;
                        debug_msg(DebugMsgId::PARSE_STRUCT_ARRAY_DECL, struct_type.c_str());
                        parser_->advance(); // consume '['
                        ASTNode* size_expr = parser_->parseExpression();
                        parser_->consume(TokenType::TOK_RBRACKET, "Expected ']' after array size");
                        
                        std::string var_name = parser_->advance().value; // 変数名を取得
                        debug_msg(DebugMsgId::PARSE_STRUCT_ARRAY_VAR_NAME, var_name.c_str());
                        
                        ASTNode* var_node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
                        var_node->name = var_name;
                        var_node->type_name = struct_type;
                        var_node->type_info = TYPE_STRUCT;
                        var_node->is_const = isConst;
                        var_node->array_size_expr = std::unique_ptr<ASTNode>(size_expr);
                        
                        // array_type_infoを設定
                        var_node->array_type_info.base_type = TYPE_STRUCT;
                        var_node->array_type_info.dimensions.push_back({0}); // サイズは後で評価される
                        
                        // 初期化式のチェック
                        if (parser_->match(TokenType::TOK_ASSIGN)) {
                            if (parser_->check(TokenType::TOK_LBRACKET)) {
                                // struct配列リテラル: Person[3] people = [{25, "Alice"}, {30, "Bob"}];
                                var_node->init_expr = std::unique_ptr<ASTNode>(parser_->parseArrayLiteral());
                            } else {
                                var_node->init_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
                            }
                        }
                        
                        parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after struct array declaration");
                        return var_node;
                    } else {
                        // 通常のstruct変数宣言: Person people;
                        std::string var_name = parser_->advance().value;
                        
                        debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(), struct_type.c_str());
                        
                        ASTNode* var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                        var_node->name = var_name;
                        var_node->type_name = struct_type;
                        var_node->type_info = TYPE_STRUCT;
                        var_node->is_const = isConst;
                        
                        // 初期化式のチェック
                        if (parser_->match(TokenType::TOK_ASSIGN)) {
                            if (parser_->check(TokenType::TOK_LBRACE)) {
                                // struct literal: Person p = {25, "Bob"};
                                var_node->init_expr = std::unique_ptr<ASTNode>(parser_->parseStructLiteral());
                            } else {
                                // その他の初期化式
                                var_node->init_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
                            }
                        }
                        
                        parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after struct variable declaration");
                        return var_node;
                    }
                } else if (is_interface_type) {
                    debug_msg(DebugMsgId::PARSE_STRUCT_VAR_DECL_FOUND, potential_type.c_str()); // interface専用のデバッグIDが必要
                    // interface変数宣言
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
                    
                    debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(), interface_type.c_str());
                    
                    ASTNode* var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                    var_node->name = var_name;
                    var_node->type_name = interface_type;
                    var_node->type_info = TYPE_INTERFACE;
                    
                    // ポインタと参照の情報を設定
                    if (pointer_depth > 0) {
                        var_node->is_pointer = true;
                        var_node->pointer_depth = pointer_depth;
                        var_node->pointer_base_type_name = interface_type;
                        var_node->pointer_base_type = TYPE_INTERFACE;
                        
                        // ポインタの場合、type_nameに*を追加
                        for (int i = 0; i < pointer_depth; i++) {
                            var_node->type_name += "*";
                        }
                    }
                    
                    if (is_reference) {
                        var_node->is_reference = true;
                        var_node->type_name += "&";
                    }
                    
                    // 初期化式のチェック（interfaceの場合は構造体インスタンスを受け取る可能性）
                    if (parser_->match(TokenType::TOK_ASSIGN)) {
                        var_node->init_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
                    }
                    
                    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after interface variable declaration");
                    
                    return var_node;
                } else {
                    // typedef型変数宣言
                    return parser_->parseTypedefVariableDeclaration();
                }
            }
        }
    }

    bool saw_unsigned_specifier = false;
    if (parser_->check(TokenType::TOK_UNSIGNED)) {
        saw_unsigned_specifier = true;
        parser_->advance();
    }

    // 関数定義の解析 (int main() など)
    if (parser_->check(TokenType::TOK_INT) || parser_->check(TokenType::TOK_LONG) || 
        parser_->check(TokenType::TOK_SHORT) || parser_->check(TokenType::TOK_TINY) || 
        parser_->check(TokenType::TOK_VOID) || parser_->check(TokenType::TOK_BOOL) ||
        parser_->check(TokenType::TOK_STRING_TYPE) || parser_->check(TokenType::TOK_CHAR_TYPE) ||
        parser_->check(TokenType::TOK_FLOAT) || parser_->check(TokenType::TOK_DOUBLE) ||
        parser_->check(TokenType::TOK_BIG) || parser_->check(TokenType::TOK_QUAD)) {
        
        // 型名を取得
        std::string base_type_name;
        if (parser_->check(TokenType::TOK_INT)) base_type_name = "int";
        else if (parser_->check(TokenType::TOK_LONG)) base_type_name = "long";
        else if (parser_->check(TokenType::TOK_SHORT)) base_type_name = "short";
        else if (parser_->check(TokenType::TOK_TINY)) base_type_name = "tiny";
        else if (parser_->check(TokenType::TOK_VOID)) base_type_name = "void";
        else if (parser_->check(TokenType::TOK_BOOL)) base_type_name = "bool";
        else if (parser_->check(TokenType::TOK_STRING_TYPE)) base_type_name = "string";
        else if (parser_->check(TokenType::TOK_CHAR_TYPE)) base_type_name = "char";
        else if (parser_->check(TokenType::TOK_FLOAT)) base_type_name = "float";
        else if (parser_->check(TokenType::TOK_DOUBLE)) base_type_name = "double";
        else if (parser_->check(TokenType::TOK_BIG)) base_type_name = "big";
        else if (parser_->check(TokenType::TOK_QUAD)) base_type_name = "quad";

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
        
        if (saw_unsigned_specifier) {
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
                    parser_->error("'unsigned' modifier can only be applied to numeric types");
                    return nullptr;
            }
            type_name = "unsigned " + base_type_name;
        }

        // 参照型の場合、型情報は基底型から取得
        std::string type_for_info = type_name;
        if (is_reference && type_for_info.back() == '&') {
            type_for_info.pop_back();  // '&'を削除
        }
        
        TypeInfo declared_type_info = parser_->getTypeInfoFromString(type_for_info);
        // 配列型の場合: int[size][size2]... identifier
        if (parser_->check(TokenType::TOK_LBRACKET)) {
            // これは配列型宣言（多次元対応）
            std::vector<std::string> array_sizes;
            
            // 全ての配列次元を解析
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
                for (const auto& size : array_sizes) {
                    return_type += "[" + size + "]";
                }
                return parser_->parseFunctionDeclarationAfterName(return_type, var_name);
            }
            
            ASTNode* node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
            node->name = var_name;
            
            // 型名構築（例: "int[2][3]"）
            std::string full_type_name = type_name;
            for (const auto& size : array_sizes) {
                full_type_name += "[" + size + "]";
            }
            node->type_name = full_type_name;
            
            // Set appropriate type_info for arrays
            node->type_info = base_type_info;
            
            // const修飾子を設定
            node->is_const = isConst;
            node->is_static = isStatic;
            node->is_unsigned = saw_unsigned_specifier;
            node->is_reference = is_reference;
            
            // ArrayTypeInfoを構築
            std::vector<ArrayDimension> dimensions;
            for (const auto& size : array_sizes) {
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
            for (const auto& size : array_sizes) {
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
                        ASTNode* size_expr = new ASTNode(ASTNodeType::AST_NUMBER);
                        size_expr->int_value = std::stoll(size);
                        node->array_dimensions.push_back(std::unique_ptr<ASTNode>(size_expr));
                    } else {
                        // 変数または式なので、適切にパース
                        if (size.find('+') != std::string::npos) {
                            // 簡単な加算式 (n+1) をパース
                            size_t plus_pos = size.find('+');
                            std::string var_name = size.substr(0, plus_pos);
                            std::string number_str = size.substr(plus_pos + 1);
                            
                            ASTNode* add_expr = new ASTNode(ASTNodeType::AST_BINARY_OP);
                            add_expr->op = "+";
                            
                            ASTNode* var_node = new ASTNode(ASTNodeType::AST_VARIABLE);
                            var_node->name = var_name;
                            add_expr->left = std::unique_ptr<ASTNode>(var_node);
                            
                            ASTNode* num_node = new ASTNode(ASTNodeType::AST_NUMBER);
                            num_node->int_value = std::stoll(number_str);
                            add_expr->right = std::unique_ptr<ASTNode>(num_node);
                            
                            node->array_dimensions.push_back(std::unique_ptr<ASTNode>(add_expr));
                        } else {
                            // 単純な変数
                            ASTNode* size_expr = new ASTNode(ASTNodeType::AST_VARIABLE);
                            size_expr->name = size;
                            node->array_dimensions.push_back(std::unique_ptr<ASTNode>(size_expr));
                        }
                    }
                } else {
                    // 動的サイズ（現在はサポートされていない）
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
                    // 配列リテラル初期化: int[SIZE] var = [val1, val2, ...]
                    parser_->advance(); // consume '['
                    
                    ASTNode* array_literal = new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);
                    while (!parser_->check(TokenType::TOK_RBRACKET) && !parser_->isAtEnd()) {
                        ASTNode* element = parser_->parseExpression();
                        array_literal->arguments.push_back(std::unique_ptr<ASTNode>(element));
                        
                        if (parser_->check(TokenType::TOK_COMMA)) {
                            parser_->advance(); // consume ','
                        } else if (!parser_->check(TokenType::TOK_RBRACKET)) {
                            parser_->error("Expected ',' or ']' in array literal");
                            return nullptr;
                        }
                    }
                    
                    parser_->consume(TokenType::TOK_RBRACKET, "Expected ']' after array literal");
                    
                    // サイズと要素数の検証（1次元配列の場合のみ、数値リテラルサイズの場合のみ）
                    if (array_sizes.size() == 1 && !array_sizes[0].empty()) {
                        // 数値リテラルかチェック
                        bool is_number = true;
                        for (char c : array_sizes[0]) {
                            if (!std::isdigit(c)) {
                                is_number = false;
                                break;
                            }
                        }
                        if (is_number) {
                            int declared_size = std::stoi(array_sizes[0]);
                            if (declared_size != static_cast<int>(array_literal->arguments.size())) {
                                parser_->error("Array literal size (" + std::to_string(array_literal->arguments.size()) + 
                                      ") does not match declared size (" + array_sizes[0] + ")");
                                return nullptr;
                            }
                        }
                        // 変数サイズの場合は実行時にチェックされるのでスキップ
                    }
                    
                    node->init_expr = std::unique_ptr<ASTNode>(array_literal);
                } else {
                    // 配列リテラル以外の式（配列スライス等）も許可
                    ASTNode* expr = parser_->parseExpression();
                    node->init_expr = std::unique_ptr<ASTNode>(expr);
                }
            }
            
            parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after array declaration");
            return node;
        }
        // 関数宣言か通常の変数宣言かチェック
        else if (parser_->check(TokenType::TOK_IDENTIFIER) || parser_->check(TokenType::TOK_MAIN)) {
            Token name_token = parser_->current_token_;
            parser_->advance(); // consume identifier/main
            
            if (parser_->check(TokenType::TOK_LPAREN)) {
                // これは関数定義
                return parser_->parseFunctionDeclarationAfterName(type_name, name_token.value);
            } else {
                // 変数宣言: type identifier [, identifier2, ...] [= expr];
                std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> variables;
                
                // 最初の変数を追加
                std::unique_ptr<ASTNode> init_expr = nullptr;
                if (parser_->match(TokenType::TOK_ASSIGN)) {
                    init_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
                }
                variables.emplace_back(name_token.value, std::move(init_expr));
                
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
                    ASTNode* node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                    node->name = variables[0].first;
                    node->type_name = type_name;
                    node->is_const = isConst;
                    node->is_static = isStatic;
                    node->is_unsigned = saw_unsigned_specifier;
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
                    ASTNode* node = new ASTNode(ASTNodeType::AST_MULTIPLE_VAR_DECL);
                    node->type_name = type_name;
                    node->is_unsigned = saw_unsigned_specifier;
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
                    for (auto& var : variables) {
                        ASTNode* var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                        var_node->name = var.first;
                        var_node->type_name = type_name;
                        var_node->is_const = isConst;
                        var_node->is_reference = is_reference;
                        var_node->is_static = isStatic;
                        var_node->is_unsigned = saw_unsigned_specifier;
                        
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
        } else {
            parser_->error("Expected identifier after type");
            return nullptr;
        }
    } else if (saw_unsigned_specifier) {
        parser_->error("Expected type specifier after 'unsigned'");
        return nullptr;
    }
    
    // return文の処理
    if (parser_->check(TokenType::TOK_RETURN)) {
        return parser_->parseReturnStatement();
    }
    
    // assert文の処理
    if (parser_->check(TokenType::TOK_ASSERT)) {
        return parser_->parseAssertStatement();
    }
    
    // break文の処理
    if (parser_->check(TokenType::TOK_BREAK)) {
        return parser_->parseBreakStatement();
    }
    
    // continue文の処理
    if (parser_->check(TokenType::TOK_CONTINUE)) {
        return parser_->parseContinueStatement();
    }
    
    // if文の処理
    if (parser_->check(TokenType::TOK_IF)) {
        return parser_->parseIfStatement();
    }
    
    // for文の処理
    if (parser_->check(TokenType::TOK_FOR)) {
        return parser_->parseForStatement();
    }
    
    // while文の処理
    if (parser_->check(TokenType::TOK_WHILE)) {
        return parser_->parseWhileStatement();
    }
    
    // ブレース文 { statements }
    if (parser_->check(TokenType::TOK_LBRACE)) {
        return parser_->parseCompoundStatement();
    }
    
    // println文の処理
    if (parser_->check(TokenType::TOK_PRINTLN)) {
        return parser_->parsePrintlnStatement();
    }
    
    // print文の処理
    if (parser_->check(TokenType::TOK_PRINT)) {
        return parser_->parsePrintStatement();
    }
    
    // identifier で始まる文 (代入文、配列要素への代入、関数呼び出し、typedef alias変数宣言等)
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        std::string name = parser_->advance().value;
        
        // typedef alias変数宣言または関数宣言の可能性をチェック
        if (parser_->check(TokenType::TOK_IDENTIFIER)) {
            std::string second_name = parser_->current_token_.value;
            parser_->advance(); // consume second identifier
            
            // 関数宣言かチェック: TypeAlias funcName(
            if (parser_->check(TokenType::TOK_LPAREN)) {
                // これは関数宣言: TypeAlias funcName(
                return parser_->parseFunctionDeclarationAfterName(name, second_name);
            } else {
                // これはtypedef alias変数宣言: TypeAlias variableName;
                ASTNode* node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                node->name = second_name;
                node->type_name = name;  // typedef alias名を記録
                node->type_info = TYPE_UNKNOWN;  // インタープリターで解決
                node->is_const = isConst;
                
                if (parser_->match(TokenType::TOK_ASSIGN)) {
                    node->init_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
                }
                
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return node;
            }
        }
        // 配列要素への代入をチェック arr[0] = value または arr[0][0] = value
        else if (parser_->check(TokenType::TOK_LBRACKET)) {
            // 変数ノードを作成
            ASTNode* var_node = new ASTNode(ASTNodeType::AST_VARIABLE);
            var_node->name = name;
            parser_->setLocation(var_node, parser_->current_token_.line, parser_->current_token_.column);
            
            // 配列アクセスチェーンを解析（parsePostfixロジックを使用）
            ASTNode* left_expr = var_node;
            while (parser_->check(TokenType::TOK_LBRACKET)) {
                parser_->advance(); // consume '['
                ASTNode* index = parser_->parseExpression();
                parser_->consume(TokenType::TOK_RBRACKET, "Expected ']'");
                
                ASTNode* array_ref = new ASTNode(ASTNodeType::AST_ARRAY_REF);
                array_ref->left = std::unique_ptr<ASTNode>(left_expr);
                array_ref->array_index = std::unique_ptr<ASTNode>(index);
                left_expr = array_ref;
            }
            
            // 配列要素のメンバアクセスをチェック: people[0].age
            if (parser_->check(TokenType::TOK_DOT)) {
                parser_->advance(); // consume '.'
                
                std::string member_name;
                if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                    member_name = parser_->advance().value;
                } else if (parser_->check(TokenType::TOK_PRINT) || parser_->check(TokenType::TOK_PRINTLN) || parser_->check(TokenType::TOK_PRINTF)) {
                    member_name = parser_->advance().value;
                } else {
                    parser_->error("Expected member name after '.'");
                    return nullptr;
                }
                
                // メンバアクセスノードを作成
                ASTNode* member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
                member_access->left = std::unique_ptr<ASTNode>(left_expr);
                member_access->name = member_name;
                left_expr = member_access;
            }
            
            // 配列への代入および複合代入の処理
            if (parser_->check(TokenType::TOK_ASSIGN) || parser_->check(TokenType::TOK_PLUS_ASSIGN) || 
                parser_->check(TokenType::TOK_MINUS_ASSIGN) || parser_->check(TokenType::TOK_MUL_ASSIGN) ||
                parser_->check(TokenType::TOK_DIV_ASSIGN) || parser_->check(TokenType::TOK_MOD_ASSIGN) ||
                parser_->check(TokenType::TOK_AND_ASSIGN) || parser_->check(TokenType::TOK_OR_ASSIGN) ||
                parser_->check(TokenType::TOK_XOR_ASSIGN) || parser_->check(TokenType::TOK_LSHIFT_ASSIGN) ||
                parser_->check(TokenType::TOK_RSHIFT_ASSIGN)) {
                
                TokenType op_type = parser_->current_token_.type;
                parser_->advance(); // consume assignment operator
                
                ASTNode* value_expr = parser_->parseExpression();
                
                ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                
                if (op_type != TokenType::TOK_ASSIGN) {
                    // 複合代入: arr[i] += b を arr[i] = arr[i] + b に変換
                    std::string binary_op;
                    switch (op_type) {
                        case TokenType::TOK_PLUS_ASSIGN: binary_op = "+"; break;
                        case TokenType::TOK_MINUS_ASSIGN: binary_op = "-"; break;
                        case TokenType::TOK_MUL_ASSIGN: binary_op = "*"; break;
                        case TokenType::TOK_DIV_ASSIGN: binary_op = "/"; break;
                        case TokenType::TOK_MOD_ASSIGN: binary_op = "%"; break;
                        case TokenType::TOK_AND_ASSIGN: binary_op = "&"; break;
                        case TokenType::TOK_OR_ASSIGN: binary_op = "|"; break;
                        case TokenType::TOK_XOR_ASSIGN: binary_op = "^"; break;
                        case TokenType::TOK_LSHIFT_ASSIGN: binary_op = "<<"; break;
                        case TokenType::TOK_RSHIFT_ASSIGN: binary_op = ">>"; break;
                        default: break;
                    }
                    
                    // arr[i] = arr[i] op value の形に変換
                    ASTNode* array_ref_copy = new ASTNode(ASTNodeType::AST_ARRAY_REF);
                    
                    // 配列変数をコピー
                    ASTNode* var_copy = new ASTNode(ASTNodeType::AST_VARIABLE);
                    var_copy->name = static_cast<ASTNode*>(left_expr->left.get())->name;
                    array_ref_copy->left = std::unique_ptr<ASTNode>(var_copy);
                    
                    // インデックスをコピー (簡単なケースのみサポート)
                    ASTNode* index_copy = nullptr;
                    if (left_expr->array_index->node_type == ASTNodeType::AST_NUMBER) {
                        index_copy = new ASTNode(ASTNodeType::AST_NUMBER);
                        index_copy->int_value = left_expr->array_index->int_value;
                    } else if (left_expr->array_index->node_type == ASTNodeType::AST_VARIABLE) {
                        index_copy = new ASTNode(ASTNodeType::AST_VARIABLE);
                        index_copy->name = left_expr->array_index->name;
                    }
                    array_ref_copy->array_index = std::unique_ptr<ASTNode>(index_copy);
                    
                    ASTNode* binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                    binop->op = binary_op;
                    binop->left = std::unique_ptr<ASTNode>(array_ref_copy);
                    binop->right = std::unique_ptr<ASTNode>(value_expr);
                    
                    assignment->left = std::unique_ptr<ASTNode>(left_expr);
                    assignment->right = std::unique_ptr<ASTNode>(binop);
                } else {
                    // 通常の代入
                    assignment->left = std::unique_ptr<ASTNode>(left_expr);
                    assignment->right = std::unique_ptr<ASTNode>(value_expr);
                }
                
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return assignment;
            } else if (parser_->check(TokenType::TOK_INCR) || parser_->check(TokenType::TOK_DECR)) {
                // 配列要素のポストインクリメント/デクリメント: arr[i]++; arr[i]--;
                Token op = parser_->advance();
                ASTNode* postfix = new ASTNode(ASTNodeType::AST_POST_INCDEC);
                postfix->op = op.value;
                postfix->left = std::unique_ptr<ASTNode>(left_expr);
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return postfix;
            } else {
                parser_->error("Expected assignment operator after array access");
                return nullptr;
            }
        } else if (parser_->check(TokenType::TOK_DOT)) {
            // メンバアクセス代入の処理: obj.member = value または obj.member[index] = value
            parser_->advance(); // consume '.'
            
            std::string member_name;
            if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                member_name = parser_->advance().value;
            } else if (parser_->check(TokenType::TOK_PRINT) || parser_->check(TokenType::TOK_PRINTLN) || parser_->check(TokenType::TOK_PRINTF)) {
                member_name = parser_->advance().value;
            } else {
                parser_->error("Expected member name after '.'");
                return nullptr;
            }
            
            // 最初のメンバアクセスノードを作成
            ASTNode* member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
            member_access->name = member_name;
            
            ASTNode* obj_var = new ASTNode(ASTNodeType::AST_VARIABLE);
            obj_var->name = name;
            member_access->left = std::unique_ptr<ASTNode>(obj_var);
            
            // ネストしたメンバーアクセスの処理 (obj.member.submember.x = value)
            while (parser_->check(TokenType::TOK_DOT)) {
                parser_->advance(); // consume '.'
                
                std::string nested_member;
                if (parser_->check(TokenType::TOK_IDENTIFIER)) {
                    nested_member = parser_->advance().value;
                } else if (parser_->check(TokenType::TOK_PRINT) || parser_->check(TokenType::TOK_PRINTLN) || parser_->check(TokenType::TOK_PRINTF)) {
                    nested_member = parser_->advance().value;
                } else {
                    parser_->error("Expected member name after '.'");
                    delete member_access;
                    return nullptr;
                }
                
                // 新しいメンバアクセスノードを作成し、前のノードを左側に配置
                ASTNode* nested_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
                nested_access->name = nested_member;
                nested_access->left = std::unique_ptr<ASTNode>(member_access);
                member_access = nested_access;
            }
            
            // メンバアクセス後の配列アクセスをチェック（多次元対応）
            if (parser_->check(TokenType::TOK_LBRACKET)) {
                std::vector<std::unique_ptr<ASTNode>> indices;
                
                // 多次元配列のインデックスを順次処理
                while (parser_->check(TokenType::TOK_LBRACKET)) {
                    parser_->advance(); // consume '['
                    
                    ASTNode* index_expr = parser_->parseExpression();
                    indices.push_back(std::unique_ptr<ASTNode>(index_expr));
                    
                    if (!parser_->check(TokenType::TOK_RBRACKET)) {
                        parser_->error("Expected ']' after array index");
                        delete member_access;
                        return nullptr;
                    }
                    parser_->advance(); // consume ']'
                }
                
                // 配列アクセス後にさらにメンバアクセスがあるかチェック: obj.array[idx].member
                // またはさらに深いネスト: obj.array[idx].member.submember
                while (parser_->check(TokenType::TOK_DOT)) {
                    parser_->advance(); // consume '.'
                    
                    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                        parser_->error("Expected member name after '.' in nested struct array access");
                        delete member_access;
                        return nullptr;
                    }
                    
                    std::string nested_member_name = parser_->current_token_.value;
                    parser_->advance();
                    
                    // このメンバーアクセスをASTに追加
                    ASTNode* array_access_node = new ASTNode(ASTNodeType::AST_MEMBER_ARRAY_ACCESS);
                    array_access_node->left = std::unique_ptr<ASTNode>(member_access);
                    array_access_node->name = nested_member_name;
                    
                    // インデックスを設定（最初の配列アクセスのみ）
                    if (!indices.empty()) {
                        if (indices.size() == 1) {
                            array_access_node->right = std::move(indices[0]);
                        } else {
                            for (auto& idx : indices) {
                                array_access_node->arguments.push_back(std::move(idx));
                            }
                        }
                        indices.clear(); // インデックスは一度だけ使用
                    }
                    
                    member_access = array_access_node;
                }
                
                // 最終的に代入チェック
                if (parser_->check(TokenType::TOK_ASSIGN)) {
                    parser_->advance(); // consume '='
                    
                    ASTNode* value_expr = parser_->parseExpression();
                    
                    // indicesがまだ残っている場合（配列アクセス後にさらにメンバアクセスがない場合: obj.array[idx] = value）
                    if (!indices.empty()) {
                        // AST_ARRAY_REFノードを作成
                        ASTNode* array_ref = new ASTNode(ASTNodeType::AST_ARRAY_REF);
                        array_ref->left = std::unique_ptr<ASTNode>(member_access);
                        if (indices.size() == 1) {
                            array_ref->array_index = std::move(indices[0]);
                        } else {
                            // 多次元配列の場合（現在は最初のインデックスのみ使用）
                            array_ref->array_index = std::move(indices[0]);
                            // TODO: 多次元配列の完全サポート
                        }
                        
                        // 代入ノードを作成
                        ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                        assignment->left = std::unique_ptr<ASTNode>(array_ref);
                        assignment->right = std::unique_ptr<ASTNode>(value_expr);
                        
                        parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after assignment");
                        return assignment;
                    } else {
                        // 通常のメンバアクセス代入
                        ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                        assignment->left = std::unique_ptr<ASTNode>(member_access);
                        assignment->right = std::unique_ptr<ASTNode>(value_expr);
                        
                        parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after assignment");
                        return assignment;
                    }
                } else {
                    parser_->error("Expected '=' after member array access");
                    delete member_access;
                    return nullptr;
                }
            }
            // 通常のメンバアクセス代入処理（複合代入演算子対応）
            else if (parser_->check(TokenType::TOK_ASSIGN) || parser_->check(TokenType::TOK_PLUS_ASSIGN) || 
                     parser_->check(TokenType::TOK_MINUS_ASSIGN) || parser_->check(TokenType::TOK_MUL_ASSIGN) ||
                     parser_->check(TokenType::TOK_DIV_ASSIGN) || parser_->check(TokenType::TOK_MOD_ASSIGN) ||
                     parser_->check(TokenType::TOK_AND_ASSIGN) || parser_->check(TokenType::TOK_OR_ASSIGN) ||
                     parser_->check(TokenType::TOK_XOR_ASSIGN) || parser_->check(TokenType::TOK_LSHIFT_ASSIGN) ||
                     parser_->check(TokenType::TOK_RSHIFT_ASSIGN)) {
                
                TokenType op_type = parser_->current_token_.type;
                parser_->advance(); // consume assignment operator
                
                ASTNode* value_expr = parser_->parseExpression();
                
                ASTNode* assignment;
                if (op_type != TokenType::TOK_ASSIGN) {
                    // 複合代入: obj.member += value を obj.member = obj.member + value に変換
                    std::string binary_op;
                    switch (op_type) {
                        case TokenType::TOK_PLUS_ASSIGN: binary_op = "+"; break;
                        case TokenType::TOK_MINUS_ASSIGN: binary_op = "-"; break;
                        case TokenType::TOK_MUL_ASSIGN: binary_op = "*"; break;
                        case TokenType::TOK_DIV_ASSIGN: binary_op = "/"; break;
                        case TokenType::TOK_MOD_ASSIGN: binary_op = "%"; break;
                        case TokenType::TOK_AND_ASSIGN: binary_op = "&"; break;
                        case TokenType::TOK_OR_ASSIGN: binary_op = "|"; break;
                        case TokenType::TOK_XOR_ASSIGN: binary_op = "^"; break;
                        case TokenType::TOK_LSHIFT_ASSIGN: binary_op = "<<"; break;
                        case TokenType::TOK_RSHIFT_ASSIGN: binary_op = ">>"; break;
                        default: binary_op = ""; break;  // 警告抑制: 他のトークンタイプは考慮不要
                    }
                    
                    // 左辺のコピーを作成（ネストメンバアクセスの深いコピーが必要）
                    ASTNode* left_copy = parser_->cloneAstNode(member_access);
                    
                    ASTNode* binary_op_node = new ASTNode(ASTNodeType::AST_BINARY_OP);
                    binary_op_node->op = binary_op;
                    binary_op_node->left = std::unique_ptr<ASTNode>(left_copy);
                    binary_op_node->right = std::unique_ptr<ASTNode>(value_expr);
                    
                    assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                    assignment->left = std::unique_ptr<ASTNode>(member_access);
                    assignment->right = std::unique_ptr<ASTNode>(binary_op_node);
                } else {
                    // 通常の代入
                    assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                    assignment->left = std::unique_ptr<ASTNode>(member_access);
                    assignment->right = std::unique_ptr<ASTNode>(value_expr);
                }
                
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return assignment;
            } else if (parser_->check(TokenType::TOK_LPAREN)) {
                // メソッド呼び出し: obj.method()
                parser_->advance(); // consume '('
                
                // メソッド呼び出しノードを作成
                ASTNode* method_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
                method_call->name = member_access->name;
                
                // レシーバーを設定（ネストメンバアクセスの場合は member_access の left を使用）
                if (member_access->left) {
                    method_call->left = std::move(member_access->left);
                } else {
                    ASTNode* obj_var = new ASTNode(ASTNodeType::AST_VARIABLE);
                    obj_var->name = name;
                    method_call->left = std::unique_ptr<ASTNode>(obj_var);
                }
                delete member_access;
                
                // 引数リストをパース
                while (!parser_->check(TokenType::TOK_RPAREN) && !parser_->isAtEnd()) {
                    auto arg = parser_->parseExpression();
                    if (arg) {
                        method_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                    }
                    
                    if (!parser_->check(TokenType::TOK_RPAREN)) {
                        parser_->consume(TokenType::TOK_COMMA, "Expected ',' between arguments");
                    }
                }
                
                parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after method arguments");
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after method call");
                
                return method_call;
            } else if (parser_->check(TokenType::TOK_INCR) || parser_->check(TokenType::TOK_DECR)) {
                // メンバーへのポストインクリメント/デクリメント: obj.member++ or obj.member--
                TokenType op_type = parser_->current_token_.type;
                parser_->advance(); // consume '++' or '--'
                
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after increment/decrement");
                
                // AST_POST_INCDECノードを作成
                ASTNode* incdec = new ASTNode(ASTNodeType::AST_POST_INCDEC);
                incdec->op = (op_type == TokenType::TOK_INCR) ? "++" : "--";
                
                // 既に作成されたネストメンバアクセスを子として設定
                incdec->left = std::unique_ptr<ASTNode>(member_access);
                
                return incdec;
            } else {
                parser_->error("Expected '=', '(', '++', or '--' after member access");
                delete member_access;
                return nullptr;
            }
        } else if (parser_->check(TokenType::TOK_ASSIGN) || parser_->check(TokenType::TOK_PLUS_ASSIGN) || 
                   parser_->check(TokenType::TOK_MINUS_ASSIGN) || parser_->check(TokenType::TOK_MUL_ASSIGN) ||
                   parser_->check(TokenType::TOK_DIV_ASSIGN) || parser_->check(TokenType::TOK_MOD_ASSIGN) ||
                   parser_->check(TokenType::TOK_AND_ASSIGN) || parser_->check(TokenType::TOK_OR_ASSIGN) ||
                   parser_->check(TokenType::TOK_XOR_ASSIGN) || parser_->check(TokenType::TOK_LSHIFT_ASSIGN) ||
                   parser_->check(TokenType::TOK_RSHIFT_ASSIGN)) {
            // 通常の代入と複合代入: identifier = value or identifier += value
            TokenType op_type = parser_->current_token_.type;
            std::string op_value = parser_->current_token_.value;
            parser_->advance(); // consume assignment operator
            
            // 配列リテラルかどうかをチェック (通常の代入の場合のみ)
            if (op_type == TokenType::TOK_ASSIGN && parser_->check(TokenType::TOK_LBRACKET)) {
                // 配列リテラル代入: identifier = [val1, val2, ...]
                parser_->advance(); // consume '['
                
                ASTNode* array_literal = new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);
                while (!parser_->check(TokenType::TOK_RBRACKET) && !parser_->isAtEnd()) {
                    ASTNode* element = parser_->parseExpression();
                    array_literal->arguments.push_back(std::unique_ptr<ASTNode>(element));
                    
                    if (parser_->check(TokenType::TOK_COMMA)) {
                        parser_->advance(); // consume ','
                    } else if (!parser_->check(TokenType::TOK_RBRACKET)) {
                        parser_->error("Expected ',' or ']' in array literal");
                        return nullptr;
                    }
                }
                
                parser_->consume(TokenType::TOK_RBRACKET, "Expected ']' after array literal");
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                
                ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                assignment->name = name;
                assignment->right = std::unique_ptr<ASTNode>(array_literal);
                return assignment;
            } else {
                // 複合代入または通常の式
                if (op_type != TokenType::TOK_ASSIGN) {
                    // 複合代入: a += b を a = a + b に変換
                    std::string binary_op;
                    switch (op_type) {
                        case TokenType::TOK_PLUS_ASSIGN: binary_op = "+"; break;
                        case TokenType::TOK_MINUS_ASSIGN: binary_op = "-"; break;
                        case TokenType::TOK_MUL_ASSIGN: binary_op = "*"; break;
                        case TokenType::TOK_DIV_ASSIGN: binary_op = "/"; break;
                        case TokenType::TOK_MOD_ASSIGN: binary_op = "%"; break;
                        case TokenType::TOK_AND_ASSIGN: binary_op = "&"; break;
                        case TokenType::TOK_OR_ASSIGN: binary_op = "|"; break;
                        case TokenType::TOK_XOR_ASSIGN: binary_op = "^"; break;
                        case TokenType::TOK_LSHIFT_ASSIGN: binary_op = "<<"; break;
                        case TokenType::TOK_RSHIFT_ASSIGN: binary_op = ">>"; break;
                        default: break;
                    }
                    
                    // 右辺の式を解析
                    ASTNode* right_expr = parser_->parseExpression();
                    
                    // a = a op right_expr の形に変換
                    ASTNode* var_ref = new ASTNode(ASTNodeType::AST_VARIABLE);
                    var_ref->name = name;
                    
                    ASTNode* binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                    binop->op = binary_op;
                    binop->left = std::unique_ptr<ASTNode>(var_ref);
                    binop->right = std::unique_ptr<ASTNode>(right_expr);
                    
                    ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                    assignment->name = name;
                    assignment->right = std::unique_ptr<ASTNode>(binop);
                    
                    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                    return assignment;
                } else {
                    // 通常の式 (identifier = expression)
                    ASTNode* expr = parser_->parseExpression();
                    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                    
                    ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                    assignment->name = name;
                    assignment->right = std::unique_ptr<ASTNode>(expr);
                    return assignment;
                }
            }
        } else {
            // 関数呼び出しなど他の式（identifierを戻す必要があるが、
            // 既に消費してしまったので、もう一度処理する）
            // 識別子だけのノードを作成してから式の続きを解析
            
            ASTNode* identifier_node = new ASTNode(ASTNodeType::AST_VARIABLE);
            identifier_node->name = name;
            parser_->setLocation(identifier_node, parser_->current_token_.line, parser_->current_token_.column);
            
            // 関数呼び出しかどうかチェック
            if (parser_->check(TokenType::TOK_LPAREN)) {
                parser_->advance(); // consume '('
                
                ASTNode* func_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
                func_call->name = name;
                
                // 引数の解析
                if (!parser_->check(TokenType::TOK_RPAREN)) {
                    do {
                        ASTNode* arg = parser_->parseExpression();
                        func_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                    } while (parser_->match(TokenType::TOK_COMMA));
                }
                
                parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after function arguments");
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                
                return func_call;
            }
            // 後置インクリメント/デクリメントのチェック
            else if (parser_->check(TokenType::TOK_INCR) || parser_->check(TokenType::TOK_DECR)) {
                TokenType op_type = parser_->current_token_.type;
                parser_->advance(); // consume '++' or '--'
                
                ASTNode* postfix_node = new ASTNode(ASTNodeType::AST_POST_INCDEC);
                postfix_node->op = (op_type == TokenType::TOK_INCR) ? "++" : "--";
                postfix_node->left = std::unique_ptr<ASTNode>(identifier_node);
                
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return postfix_node;
            }
            // メンバアクセスやアロー演算子のチェック
            else if (parser_->check(TokenType::TOK_DOT) || parser_->check(TokenType::TOK_ARROW) || parser_->check(TokenType::TOK_LBRACKET)) {
                // 識別子から始まる複雑な式（obj.member, ptr->member, arr[i]等）
                // レキサーを巻き戻して、完全な式として解析し直す
                // 現在の位置を保存
                RecursiveLexer saved_lexer = parser_->lexer_;
                Token saved_token = parser_->current_token_;
                
                // 識別子を含む完全な式を解析するため、識別子の位置に戻る必要がある
                // しかし、既に識別子を消費してしまっているので、
                // 代わりに識別子ノードからpostfix処理を継続する
                
                // identifier_nodeから始めて、postfix操作（.、->、[]）を処理
                ASTNode* expr_node = identifier_node;
                identifier_node = nullptr; // ownershipを移動
                
                // postfix操作を処理
                while (true) {
                    if (parser_->check(TokenType::TOK_LBRACKET)) {
                        // 配列アクセス
                        parser_->advance(); // consume '['
                        ASTNode* index = parser_->parseExpression();
                        parser_->consume(TokenType::TOK_RBRACKET, "Expected ']'");
                        
                        ASTNode* array_ref = new ASTNode(ASTNodeType::AST_ARRAY_REF);
                        array_ref->left = std::unique_ptr<ASTNode>(expr_node);
                        array_ref->array_index = std::unique_ptr<ASTNode>(index);
                        expr_node = array_ref;
                    } else if (parser_->check(TokenType::TOK_DOT)) {
                        // メンバアクセス
                        expr_node = parser_->parseMemberAccess(expr_node);
                    } else if (parser_->check(TokenType::TOK_ARROW)) {
                        // アロー演算子
                        expr_node = parser_->parseArrowAccess(expr_node);
                    } else {
                        break;
                    }
                }
                
                // 後置インクリメント/デクリメントのチェック
                if (parser_->check(TokenType::TOK_INCR) || parser_->check(TokenType::TOK_DECR)) {
                    TokenType op_type = parser_->current_token_.type;
                    parser_->advance(); // consume '++' or '--'
                    
                    ASTNode* postfix_node = new ASTNode(ASTNodeType::AST_POST_INCDEC);
                    postfix_node->op = (op_type == TokenType::TOK_INCR) ? "++" : "--";
                    postfix_node->left = std::unique_ptr<ASTNode>(expr_node);
                    
                    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                    return postfix_node;
                }
                // 代入があるかチェック
                else if (parser_->check(TokenType::TOK_ASSIGN) || parser_->check(TokenType::TOK_PLUS_ASSIGN) || 
                    parser_->check(TokenType::TOK_MINUS_ASSIGN) || parser_->check(TokenType::TOK_MUL_ASSIGN) ||
                    parser_->check(TokenType::TOK_DIV_ASSIGN) || parser_->check(TokenType::TOK_MOD_ASSIGN)) {
                    // TokenType op_type = parser_->current_token_.type;  // 将来の拡張用に保持
                    parser_->advance(); // consume assignment operator
                    
                    ASTNode* right = parser_->parseExpression();
                    
                    ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                    assignment->left = std::unique_ptr<ASTNode>(expr_node);
                    assignment->right = std::unique_ptr<ASTNode>(right);
                    
                    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                    return assignment;
                }
                
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return expr_node;
            } else {
                // その他の単純な識別子式
                parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return identifier_node;
            }
        }
    }
    
    // その他の式
    ASTNode* expr = parser_->parseExpression();
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';'");
    return expr;
}

ASTNode* StatementParser::parseCompoundStatement() {
    parser_->advance(); // consume '{'
    
    ASTNode* compound = new ASTNode(ASTNodeType::AST_COMPOUND_STMT);
    
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        ASTNode* stmt = parser_->parseStatement();
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
ASTNode* StatementParser::parseIfStatement() {
    parser_->advance(); // consume 'if'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after if");
    
    ASTNode* if_node = new ASTNode(ASTNodeType::AST_IF_STMT);
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
ASTNode* StatementParser::parseForStatement() {
    parser_->advance(); // consume 'for'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after for");
    
    ASTNode* for_node = new ASTNode(ASTNodeType::AST_FOR_STMT);
    
    // 初期化部分 (int i = 0;) - 文として扱う
    for_node->init_expr = std::unique_ptr<ASTNode>(parser_->parseStatement());
    
    // 条件部分 (i < 5) - 式として扱う
    for_node->condition = std::unique_ptr<ASTNode>(parser_->parseExpression());
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after for condition");
    
    // 更新部分 - 一般的な式として処理（i++, i--, i=i+1など）
    for_node->update_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
    
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
ASTNode* StatementParser::parseWhileStatement() {
    parser_->advance(); // consume 'while'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after while");
    
    ASTNode* while_node = new ASTNode(ASTNodeType::AST_WHILE_STMT);
    
    // 条件部分
    while_node->condition = std::unique_ptr<ASTNode>(parser_->parseExpression());
    
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after while condition");
    
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
ASTNode* StatementParser::parseReturnStatement() {
    parser_->advance(); // consume 'return'
    ASTNode* return_node = new ASTNode(ASTNodeType::AST_RETURN_STMT);
    
    // return値があるかチェック
    if (!parser_->check(TokenType::TOK_SEMICOLON)) {
        return_node->left = std::unique_ptr<ASTNode>(parser_->parseExpression());
    }
    
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after return statement");
    return return_node;
}

/**
 * @brief break文を解析
 * @return 解析されたASTbreak文ノード
 * 
 * 構文: break;
 * ループまたはswitchから脱出（現在はループのみサポート）
 */
ASTNode* StatementParser::parseBreakStatement() {
    parser_->advance(); // consume 'break'
    ASTNode* break_node = new ASTNode(ASTNodeType::AST_BREAK_STMT);
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after break statement");
    return break_node;
}

/**
 * @brief continue文を解析
 * @return 解析されたASTcontinue文ノード
 * 
 * 構文: continue;
 * ループの次の反復へスキップ
 */
ASTNode* StatementParser::parseContinueStatement() {
    parser_->advance(); // consume 'continue'
    ASTNode* continue_node = new ASTNode(ASTNodeType::AST_CONTINUE_STMT);
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after continue statement");
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
ASTNode* StatementParser::parseAssertStatement() {
    Token assert_token = parser_->advance(); // consume 'assert'
    
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after assert");
    
    // 条件式をパース
    ASTNode* condition = parser_->parseExpression();
    
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after assert condition");
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after assert statement");
    
    ASTNode* assert_node = new ASTNode(ASTNodeType::AST_ASSERT_STMT);
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
ASTNode* StatementParser::parsePrintlnStatement() {
    parser_->advance(); // consume 'println'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after println");
    
    ASTNode* print_node = new ASTNode(ASTNodeType::AST_PRINTLN_STMT);
    
    // 複数の引数をパース
    if (!parser_->check(TokenType::TOK_RPAREN)) {
        do {
            ASTNode* arg = parser_->parseExpression();
            print_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
        } while (parser_->match(TokenType::TOK_COMMA));
    }
    
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after println arguments");
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after println statement");
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
ASTNode* StatementParser::parsePrintStatement() {
    parser_->advance(); // consume 'print'
    
    ASTNode* print_node = new ASTNode(ASTNodeType::AST_PRINT_STMT);
    
    // 引数をパース - 任意の式を受け入れる
    if (parser_->check(TokenType::TOK_LPAREN)) {
        // print(expression[, expression, ...]); 形式
        parser_->advance(); // consume '('
        
        // 複数の引数をパース
        if (!parser_->check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode* arg = parser_->parseExpression();
                print_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
            } while (parser_->match(TokenType::TOK_COMMA));
        }
        
        parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after print arguments");
    } else if (!parser_->check(TokenType::TOK_SEMICOLON)) {
        // print expression; 形式（括弧なし）
        print_node->left = std::unique_ptr<ASTNode>(parser_->parseExpression());
    } else {
        parser_->error("Expected expression after print");
        return nullptr;
    }
    
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after print statement");
    return print_node;
}
