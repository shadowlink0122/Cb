#include "recursive_parser.h"
#include "../../backend/interpreter/core/error_handler.h"
#include "../../common/debug.h"
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <algorithm>
#include <cctype>

using namespace RecursiveParserNS;

RecursiveParser::RecursiveParser(const std::string& source, const std::string& filename) 
    : lexer_(source), current_token_(TokenType::TOK_EOF, "", 0, 0), filename_(filename), source_(source), debug_mode_(false) {
    // ソースコードを行ごとに分割
    std::istringstream iss(source);
    std::string line;
    while (std::getline(iss, line)) {
        source_lines_.push_back(line);
    }
    advance();
}

ASTNode* RecursiveParser::parse() {
    return parseProgram();
}

bool RecursiveParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool RecursiveParser::check(TokenType type) {
    return current_token_.type == type;
}

Token RecursiveParser::advance() {
    Token previous = current_token_;
    current_token_ = lexer_.nextToken();
    return previous;
}

Token RecursiveParser::peek() {
    return current_token_;
}

bool RecursiveParser::isAtEnd() {
    return current_token_.type == TokenType::TOK_EOF;
}

void RecursiveParser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return;
    }
    error(message);
}

void RecursiveParser::error(const std::string& message) {
    // 詳細なエラー表示
    std::string source_line = getSourceLine(current_token_.line);
    print_error_with_location(
        message,
        filename_,
        current_token_.line,
        current_token_.column,
        source_line
    );
    
    throw DetailedErrorException(message);
}

ASTNode* RecursiveParser::parseProgram() {
    ASTNode* program = new ASTNode(ASTNodeType::AST_STMT_LIST);
    
    while (!isAtEnd()) {
        ASTNode* stmt = parseStatement();
        if (stmt != nullptr) {
            program->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }
    
    return program;
}

ASTNode* RecursiveParser::parseStatement() {
    // static修飾子のチェック
    bool isStatic = false;
    if (check(TokenType::TOK_STATIC)) {
        isStatic = true;
        advance(); // consume 'static'
    }
    
    // const修飾子のチェック
    bool isConst = false;
    if (check(TokenType::TOK_CONST)) {
        isConst = true;
        advance(); // consume 'const'
    }
    
    // DEBUG: 現在のトークンを出力
    if (debug_mode && current_token_.type != TokenType::TOK_EOF) {
        std::cerr << "[DEBUG] parseStatement: " << current_token_.value 
                  << " (type: " << (int)current_token_.type << ")" << std::endl;
    }
    
    // main関数の場合の特別処理
    if (check(TokenType::TOK_MAIN)) {
        Token main_token = current_token_;
        advance(); // consume 'main'
        
        if (check(TokenType::TOK_LPAREN)) {
            // main() 関数の定義
            return parseFunctionDeclarationAfterName("int", main_token.value);
        } else {
            error("Expected '(' after main");
            return nullptr;
        }
    }
    
    // typedef宣言の処理
    if (check(TokenType::TOK_TYPEDEF)) {
        return parseTypedefDeclaration();
    }
    
    // struct宣言の処理
    if (check(TokenType::TOK_STRUCT)) {
        return parseStructDeclaration();
    }
    
    // typedef型変数宣言の処理
    if (check(TokenType::TOK_IDENTIFIER)) {
        std::string potential_type = current_token_.value;
        
        // typedef型または構造体型の可能性をチェック
        bool is_typedef = typedef_map_.find(potential_type) != typedef_map_.end();
        bool is_struct_type = struct_definitions_.find(potential_type) != struct_definitions_.end();
        
        if (is_typedef || is_struct_type) {
            // 簡単な先読み: 現在の位置を保存
            RecursiveLexer temp_lexer = lexer_;
            Token temp_current = current_token_;
            
            advance(); // 型名をスキップ
            
            bool is_function = false;
            if (check(TokenType::TOK_IDENTIFIER)) {
                advance(); // 識別子をスキップ
                if (check(TokenType::TOK_LPAREN)) {
                    is_function = true;
                } else if (check(TokenType::TOK_LBRACKET)) {
                    // 配列宣言なので関数ではない
                    is_function = false;
                }
            }
            
            // 元の位置に戻す
            lexer_ = temp_lexer;
            current_token_ = temp_current;
            
            if (is_function) {
                // これは戻り値型が構造体またはtypedefの関数宣言
                std::string return_type = advance().value; // 型名を取得
                std::string function_name = advance().value; // 関数名を取得
                return parseFunctionDeclarationAfterName(return_type, function_name);
            } else {
                // これは構造体またはtypedef型変数宣言
                if (is_struct_type) {
                    // 構造体変数宣言
                    std::string struct_type = advance().value;
                    
                    // 配列チェック: Person[3] people; または Person people;
                    if (check(TokenType::TOK_LBRACKET)) {
                        // struct配列宣言: Person[3] people;
                        advance(); // consume '['
                        ASTNode* size_expr = parseExpression();
                        consume(TokenType::TOK_RBRACKET, "Expected ']' after array size");
                        
                        std::string var_name = advance().value; // 変数名を取得
                        
                        ASTNode* var_node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
                        var_node->name = var_name;
                        var_node->type_name = struct_type;
                        var_node->type_info = TYPE_STRUCT;
                        var_node->array_size_expr = std::unique_ptr<ASTNode>(size_expr);
                        
                        // array_type_infoを設定
                        var_node->array_type_info.base_type = TYPE_STRUCT;
                        var_node->array_type_info.dimensions.push_back({0}); // サイズは後で評価される
                        
                        // 初期化式のチェック
                        if (match(TokenType::TOK_ASSIGN)) {
                            if (check(TokenType::TOK_LBRACKET)) {
                                // struct配列リテラル: Person[3] people = [{25, "Alice"}, {30, "Bob"}];
                                var_node->init_expr = std::unique_ptr<ASTNode>(parseArrayLiteral());
                            } else {
                                var_node->init_expr = std::unique_ptr<ASTNode>(parseExpression());
                            }
                        }
                        
                        consume(TokenType::TOK_SEMICOLON, "Expected ';' after struct array declaration");
                        return var_node;
                    } else {
                        // 通常のstruct変数宣言: Person people;
                        std::string var_name = advance().value;
                        
                        ASTNode* var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                        var_node->name = var_name;
                        var_node->type_name = struct_type;
                        var_node->type_info = TYPE_STRUCT;
                        
                        // 初期化式のチェック
                        if (match(TokenType::TOK_ASSIGN)) {
                            if (check(TokenType::TOK_LBRACE)) {
                                // struct literal: Person p = {25, "Bob"};
                                var_node->init_expr = std::unique_ptr<ASTNode>(parseStructLiteral());
                            } else {
                                // その他の初期化式
                                var_node->init_expr = std::unique_ptr<ASTNode>(parseExpression());
                            }
                        }
                        
                        consume(TokenType::TOK_SEMICOLON, "Expected ';' after struct variable declaration");
                        return var_node;
                    }
                } else {
                    // typedef型変数宣言
                    return parseTypedefVariableDeclaration();
                }
            }
        }
    }

    // 関数定義の解析 (int main() など)
    if (check(TokenType::TOK_INT) || check(TokenType::TOK_LONG) || 
        check(TokenType::TOK_SHORT) || check(TokenType::TOK_TINY) || 
        check(TokenType::TOK_VOID) || check(TokenType::TOK_BOOL) ||
        check(TokenType::TOK_STRING_TYPE) || check(TokenType::TOK_CHAR_TYPE)) {
        
        // 型名を取得
        std::string type_name;
        if (check(TokenType::TOK_INT)) type_name = "int";
        else if (check(TokenType::TOK_LONG)) type_name = "long";
        else if (check(TokenType::TOK_SHORT)) type_name = "short";
        else if (check(TokenType::TOK_TINY)) type_name = "tiny";
        else if (check(TokenType::TOK_VOID)) type_name = "void";
        else if (check(TokenType::TOK_BOOL)) type_name = "bool";
        else if (check(TokenType::TOK_STRING_TYPE)) type_name = "string";
        else if (check(TokenType::TOK_CHAR_TYPE)) type_name = "char";
        
        advance(); // consume type
        
        // 配列型の場合: int[size][size2]... identifier
        if (check(TokenType::TOK_LBRACKET)) {
            // これは配列型宣言（多次元対応）
            std::vector<std::string> array_sizes;
            
            // 全ての配列次元を解析
            while (check(TokenType::TOK_LBRACKET)) {
                advance(); // consume '['
                
                std::string size = "";
                if (check(TokenType::TOK_NUMBER)) {
                    size = advance().value;
                } else if (check(TokenType::TOK_IDENTIFIER)) {
                    // 変数名を配列サイズとして使用
                    size = advance().value;
                    // 簡単な算術式もサポート（n+1のような形）
                    if (check(TokenType::TOK_PLUS)) {
                        advance(); // consume '+'
                        if (check(TokenType::TOK_NUMBER)) {
                            size += "+" + advance().value;
                        }
                    }
                } else {
                    // 空の配列サイズ（動的配列）
                    size = "";
                }
                array_sizes.push_back(size);
                
                consume(TokenType::TOK_RBRACKET, "Expected ']' in array type");
            }
            
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected identifier after array type");
                return nullptr;
            }
            
            Token name_token = advance(); // consume identifier
            std::string var_name = name_token.value;
            
            // 関数宣言かチェック
            if (check(TokenType::TOK_LPAREN)) {
                // これは配列戻り値の関数宣言
                std::string return_type = type_name;
                for (const auto& size : array_sizes) {
                    return_type += "[" + size + "]";
                }
                return parseFunctionDeclarationAfterName(return_type, var_name);
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
            TypeInfo base_type = TYPE_INT; // default
            if (type_name == "int") base_type = TYPE_INT;
            else if (type_name == "long") base_type = TYPE_LONG;
            else if (type_name == "short") base_type = TYPE_SHORT;
            else if (type_name == "tiny") base_type = TYPE_TINY;
            else if (type_name == "bool") base_type = TYPE_BOOL;
            else if (type_name == "string") base_type = TYPE_STRING;
            else if (type_name == "char") base_type = TYPE_CHAR;
            
            node->type_info = base_type;
            
            // const修飾子を設定
            node->is_const = isConst;
            
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
            node->array_type_info = ArrayTypeInfo(base_type, dimensions);
            
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
            if (check(TokenType::TOK_ASSIGN)) {
                advance(); // consume '='
                
                if (check(TokenType::TOK_LBRACKET)) {
                    // 配列リテラル初期化: int[SIZE] var = [val1, val2, ...]
                    advance(); // consume '['
                    
                    ASTNode* array_literal = new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);
                    while (!check(TokenType::TOK_RBRACKET) && !isAtEnd()) {
                        ASTNode* element = parseExpression();
                        array_literal->arguments.push_back(std::unique_ptr<ASTNode>(element));
                        
                        if (check(TokenType::TOK_COMMA)) {
                            advance(); // consume ','
                        } else if (!check(TokenType::TOK_RBRACKET)) {
                            error("Expected ',' or ']' in array literal");
                            return nullptr;
                        }
                    }
                    
                    consume(TokenType::TOK_RBRACKET, "Expected ']' after array literal");
                    
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
                                error("Array literal size (" + std::to_string(array_literal->arguments.size()) + 
                                      ") does not match declared size (" + array_sizes[0] + ")");
                                return nullptr;
                            }
                        }
                        // 変数サイズの場合は実行時にチェックされるのでスキップ
                    }
                    
                    node->init_expr = std::unique_ptr<ASTNode>(array_literal);
                } else {
                    // 配列リテラル以外の式（配列スライス等）も許可
                    ASTNode* expr = parseExpression();
                    node->init_expr = std::unique_ptr<ASTNode>(expr);
                }
            }
            
            consume(TokenType::TOK_SEMICOLON, "Expected ';' after array declaration");
            return node;
        }
        // 関数宣言か通常の変数宣言かチェック
        else if (check(TokenType::TOK_IDENTIFIER) || check(TokenType::TOK_MAIN)) {
            Token name_token = current_token_;
            advance(); // consume identifier/main
            
            if (check(TokenType::TOK_LPAREN)) {
                // これは関数定義
                return parseFunctionDeclarationAfterName(type_name, name_token.value);
            } else {
                // 変数宣言: type identifier [, identifier2, ...] [= expr];
                std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> variables;
                
                // 最初の変数を追加
                std::unique_ptr<ASTNode> init_expr = nullptr;
                if (match(TokenType::TOK_ASSIGN)) {
                    init_expr = std::unique_ptr<ASTNode>(parseExpression());
                }
                variables.emplace_back(name_token.value, std::move(init_expr));
                
                // カンマで区切られた追加の変数をパース
                while (match(TokenType::TOK_COMMA)) {
                    if (!check(TokenType::TOK_IDENTIFIER)) {
                        error("Expected variable name after ','");
                        return nullptr;
                    }
                    
                    std::string var_name = advance().value;
                    std::unique_ptr<ASTNode> var_init = nullptr;
                    
                    if (match(TokenType::TOK_ASSIGN)) {
                        var_init = std::unique_ptr<ASTNode>(parseExpression());
                    }
                    
                    variables.emplace_back(var_name, std::move(var_init));
                }
                
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                
                // 単一変数か複数変数かに応じてノードを作成
                if (variables.size() == 1) {
                    ASTNode* node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                    node->name = variables[0].first;
                    node->type_name = type_name;
                    node->is_const = isConst;
                    node->is_static = isStatic;
                    
                    // 型情報を設定
                    if (type_name == "int") {
                        node->type_info = TYPE_INT;
                    } else if (type_name == "long") {
                        node->type_info = TYPE_LONG;
                    } else if (type_name == "short") {
                        node->type_info = TYPE_SHORT;
                    } else if (type_name == "tiny") {
                        node->type_info = TYPE_TINY;
                    } else if (type_name == "bool") {
                        node->type_info = TYPE_BOOL;
                    } else if (type_name == "string") {
                        node->type_info = TYPE_STRING;
                    } else if (type_name == "char") {
                        node->type_info = TYPE_CHAR;
                    } else if (type_name == "void") {
                        node->type_info = TYPE_VOID;
                    } else {
                        node->type_info = TYPE_UNKNOWN;
                    }
                    
                    if (variables[0].second) {
                        node->init_expr = std::move(variables[0].second);
                    }
                    
                    return node;
                } else {
                    // 複数変数宣言
                    ASTNode* node = new ASTNode(ASTNodeType::AST_MULTIPLE_VAR_DECL);
                    node->type_name = type_name;
                    
                    // 型情報を設定
                    if (type_name == "int") {
                        node->type_info = TYPE_INT;
                    } else if (type_name == "long") {
                        node->type_info = TYPE_LONG;
                    } else if (type_name == "short") {
                        node->type_info = TYPE_SHORT;
                    } else if (type_name == "tiny") {
                        node->type_info = TYPE_TINY;
                    } else if (type_name == "bool") {
                        node->type_info = TYPE_BOOL;
                    } else if (type_name == "string") {
                        node->type_info = TYPE_STRING;
                    } else if (type_name == "char") {
                        node->type_info = TYPE_CHAR;
                    } else if (type_name == "void") {
                        node->type_info = TYPE_VOID;
                    } else {
                        node->type_info = TYPE_UNKNOWN;
                    }
                    
                    // 各変数を子ノードとして追加
                    for (auto& var : variables) {
                        ASTNode* var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                        var_node->name = var.first;
                        var_node->type_name = type_name;
                        var_node->type_info = node->type_info;
                        var_node->is_const = isConst;
                        var_node->is_static = isStatic;
                        
                        if (var.second) {
                            var_node->init_expr = std::move(var.second);
                        }
                        
                        node->children.push_back(std::unique_ptr<ASTNode>(var_node));
                    }
                    
                    return node;
                }
            }
        } else {
            error("Expected identifier after type");
            return nullptr;
        }
    }
    
    // return文の処理
    if (check(TokenType::TOK_RETURN)) {
        return parseReturnStatement();
    }
    
    // break文の処理
    if (check(TokenType::TOK_BREAK)) {
        return parseBreakStatement();
    }
    
    // continue文の処理
    if (check(TokenType::TOK_CONTINUE)) {
        return parseContinueStatement();
    }
    
    // if文の処理
    if (check(TokenType::TOK_IF)) {
        return parseIfStatement();
    }
    
    // for文の処理
    if (check(TokenType::TOK_FOR)) {
        return parseForStatement();
    }
    
    // while文の処理
    if (check(TokenType::TOK_WHILE)) {
        return parseWhileStatement();
    }
    
    // ブレース文 { statements }
    if (check(TokenType::TOK_LBRACE)) {
        return parseCompoundStatement();
    }
    
    // println文の処理
    if (check(TokenType::TOK_PRINTLN)) {
        return parsePrintlnStatement();
    }
    
    // print文の処理
    if (check(TokenType::TOK_PRINT)) {
        return parsePrintStatement();
    }
    
    // identifier で始まる文 (代入文、配列要素への代入、関数呼び出し、typedef alias変数宣言等)
    if (check(TokenType::TOK_IDENTIFIER)) {
        std::string name = advance().value;
        
        // typedef alias変数宣言または関数宣言の可能性をチェック
        if (check(TokenType::TOK_IDENTIFIER)) {
            std::string second_name = current_token_.value;
            advance(); // consume second identifier
            
            // 関数宣言かチェック: TypeAlias funcName(
            if (check(TokenType::TOK_LPAREN)) {
                // これは関数宣言: TypeAlias funcName(
                return parseFunctionDeclarationAfterName(name, second_name);
            } else {
                // これはtypedef alias変数宣言: TypeAlias variableName;
                ASTNode* node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                node->name = second_name;
                node->type_name = name;  // typedef alias名を記録
                node->type_info = TYPE_UNKNOWN;  // インタープリターで解決
                node->is_const = isConst;
                
                if (match(TokenType::TOK_ASSIGN)) {
                    node->init_expr = std::unique_ptr<ASTNode>(parseExpression());
                }
                
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return node;
            }
        }
        // 配列要素への代入をチェック arr[0] = value または arr[0][0] = value
        else if (check(TokenType::TOK_LBRACKET)) {
            // 変数ノードを作成
            ASTNode* var_node = new ASTNode(ASTNodeType::AST_VARIABLE);
            var_node->name = name;
            setLocation(var_node, current_token_.line, current_token_.column);
            
            // 配列アクセスチェーンを解析（parsePostfixロジックを使用）
            ASTNode* left_expr = var_node;
            while (check(TokenType::TOK_LBRACKET)) {
                advance(); // consume '['
                ASTNode* index = parseExpression();
                consume(TokenType::TOK_RBRACKET, "Expected ']'");
                
                ASTNode* array_ref = new ASTNode(ASTNodeType::AST_ARRAY_REF);
                array_ref->left = std::unique_ptr<ASTNode>(left_expr);
                array_ref->array_index = std::unique_ptr<ASTNode>(index);
                left_expr = array_ref;
            }
            
            // 配列要素のメンバアクセスをチェック: people[0].age
            if (check(TokenType::TOK_DOT)) {
                advance(); // consume '.'
                
                if (!check(TokenType::TOK_IDENTIFIER)) {
                    error("Expected member name after '.'");
                    return nullptr;
                }
                
                std::string member_name = advance().value;
                
                // メンバアクセスノードを作成
                ASTNode* member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
                member_access->left = std::unique_ptr<ASTNode>(left_expr);
                member_access->name = member_name;
                left_expr = member_access;
            }
            
            // 配列への代入および複合代入の処理
            if (check(TokenType::TOK_ASSIGN) || check(TokenType::TOK_PLUS_ASSIGN) || 
                check(TokenType::TOK_MINUS_ASSIGN) || check(TokenType::TOK_MUL_ASSIGN) ||
                check(TokenType::TOK_DIV_ASSIGN) || check(TokenType::TOK_MOD_ASSIGN) ||
                check(TokenType::TOK_AND_ASSIGN) || check(TokenType::TOK_OR_ASSIGN) ||
                check(TokenType::TOK_XOR_ASSIGN) || check(TokenType::TOK_LSHIFT_ASSIGN) ||
                check(TokenType::TOK_RSHIFT_ASSIGN)) {
                
                TokenType op_type = current_token_.type;
                advance(); // consume assignment operator
                
                ASTNode* value_expr = parseExpression();
                
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
                
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return assignment;
            } else {
                error("Expected assignment operator after array access");
                return nullptr;
            }
        } else if (check(TokenType::TOK_DOT)) {
            // メンバアクセス代入の処理: obj.member = value または obj.member[index] = value
            advance(); // consume '.'
            
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected member name after '.'");
                return nullptr;
            }
            
            std::string member_name = advance().value;
            
            // ネストしたメンバーアクセスの検出 (obj.member.submember = value)
            if (check(TokenType::TOK_DOT)) {
                error("Nested member access assignment (obj.member.submember = value) is not supported yet. Consider using pointers in future implementation.");
                return nullptr;
            }
            
            // メンバアクセス後の配列アクセスをチェック（多次元対応）
            if (check(TokenType::TOK_LBRACKET)) {
                std::vector<std::unique_ptr<ASTNode>> indices;
                
                advance(); // consume '['
                
                ASTNode* index_expr = parseExpression();
                
                if (!check(TokenType::TOK_RBRACKET)) {
                    error("Expected ']' after array index");
                    return nullptr;
                }
                advance(); // consume ']'
                
                // 代入演算子をチェック
                if (check(TokenType::TOK_ASSIGN)) {
                    advance(); // consume '='
                    
                    ASTNode* value_expr = parseExpression();
                    
                    // メンバの配列アクセスを表す特別なノードを作成
                    ASTNode* member_array_access = new ASTNode(ASTNodeType::AST_MEMBER_ARRAY_ACCESS);
                    member_array_access->name = member_name;
                    
                    ASTNode* obj_var = new ASTNode(ASTNodeType::AST_VARIABLE);
                    obj_var->name = name;
                    member_array_access->left = std::unique_ptr<ASTNode>(obj_var);
                    member_array_access->right = std::unique_ptr<ASTNode>(index_expr);
                    
                    // 代入ノードを作成
                    ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                    assignment->left = std::unique_ptr<ASTNode>(member_array_access);
                    assignment->right = std::unique_ptr<ASTNode>(value_expr);
                    
                    consume(TokenType::TOK_SEMICOLON, "Expected ';' after assignment");
                    return assignment;
                } else {
                    error("Expected '=' after member array access");
                    return nullptr;
                }
            }
            // 通常のメンバアクセス代入処理
            else if (check(TokenType::TOK_ASSIGN)) {
                advance(); // consume '='
                
                ASTNode* value_expr = parseExpression();
                
                // メンバアクセスノードを作成
                ASTNode* member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
                member_access->name = member_name;
                
                ASTNode* obj_var = new ASTNode(ASTNodeType::AST_VARIABLE);
                obj_var->name = name;
                member_access->left = std::unique_ptr<ASTNode>(obj_var);
                
                // 代入ノードを作成
                ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                assignment->left = std::unique_ptr<ASTNode>(member_access);
                assignment->right = std::unique_ptr<ASTNode>(value_expr);
                
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return assignment;
            } else {
                error("Expected '=' after member access");
                return nullptr;
            }
        } else if (check(TokenType::TOK_ASSIGN) || check(TokenType::TOK_PLUS_ASSIGN) || 
                   check(TokenType::TOK_MINUS_ASSIGN) || check(TokenType::TOK_MUL_ASSIGN) ||
                   check(TokenType::TOK_DIV_ASSIGN) || check(TokenType::TOK_MOD_ASSIGN) ||
                   check(TokenType::TOK_AND_ASSIGN) || check(TokenType::TOK_OR_ASSIGN) ||
                   check(TokenType::TOK_XOR_ASSIGN) || check(TokenType::TOK_LSHIFT_ASSIGN) ||
                   check(TokenType::TOK_RSHIFT_ASSIGN)) {
            // 通常の代入と複合代入: identifier = value or identifier += value
            TokenType op_type = current_token_.type;
            std::string op_value = current_token_.value;
            advance(); // consume assignment operator
            
            // 配列リテラルかどうかをチェック (通常の代入の場合のみ)
            if (op_type == TokenType::TOK_ASSIGN && check(TokenType::TOK_LBRACKET)) {
                // 配列リテラル代入: identifier = [val1, val2, ...]
                advance(); // consume '['
                
                ASTNode* array_literal = new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);
                while (!check(TokenType::TOK_RBRACKET) && !isAtEnd()) {
                    ASTNode* element = parseExpression();
                    array_literal->arguments.push_back(std::unique_ptr<ASTNode>(element));
                    
                    if (check(TokenType::TOK_COMMA)) {
                        advance(); // consume ','
                    } else if (!check(TokenType::TOK_RBRACKET)) {
                        error("Expected ',' or ']' in array literal");
                        return nullptr;
                    }
                }
                
                consume(TokenType::TOK_RBRACKET, "Expected ']' after array literal");
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                
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
                    ASTNode* right_expr = parseExpression();
                    
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
                    
                    consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                    return assignment;
                } else {
                    // 通常の式 (identifier = expression)
                    ASTNode* expr = parseExpression();
                    consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                    
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
            setLocation(identifier_node, current_token_.line, current_token_.column);
            
            // 関数呼び出しかどうかチェック
            if (check(TokenType::TOK_LPAREN)) {
                advance(); // consume '('
                
                ASTNode* func_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
                func_call->name = name;
                
                // 引数の解析
                if (!check(TokenType::TOK_RPAREN)) {
                    do {
                        ASTNode* arg = parseExpression();
                        func_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                    } while (match(TokenType::TOK_COMMA));
                }
                
                consume(TokenType::TOK_RPAREN, "Expected ')' after function arguments");
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                
                return func_call;
            }
            // 後置インクリメント/デクリメントのチェック
            else if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
                TokenType op_type = current_token_.type;
                advance(); // consume '++' or '--'
                
                ASTNode* postfix_node = new ASTNode(ASTNodeType::AST_POST_INCDEC);
                postfix_node->op = (op_type == TokenType::TOK_INCR) ? "++" : "--";
                postfix_node->left = std::unique_ptr<ASTNode>(identifier_node);
                
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return postfix_node;
            } else {
                // その他の単純な識別子式
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return identifier_node;
            }
        }
    }
    
    // その他の式
    ASTNode* expr = parseExpression();
    consume(TokenType::TOK_SEMICOLON, "Expected ';'");
    return expr;
}

ASTNode* RecursiveParser::parseVariableDeclaration() {
    std::string var_type = parseType();
    
    // 変数名のリストを収集
    struct VariableInfo {
        std::string name;
        std::unique_ptr<ASTNode> init_expr;
        ArrayTypeInfo array_info;
        bool is_array;
        
        VariableInfo(const std::string& n, std::unique_ptr<ASTNode> expr, const ArrayTypeInfo& arr_info, bool arr)
            : name(n), init_expr(std::move(expr)), array_info(arr_info), is_array(arr) {}
    };
    std::vector<VariableInfo> variables;
    
    do {
        if (!check(TokenType::TOK_IDENTIFIER)) {
            error("Expected variable name");
        }
        
        std::string var_name = advance().value;
        std::unique_ptr<ASTNode> init_expr = nullptr;
        ArrayTypeInfo array_info;
        bool is_array = false;
        
        // 配列の角括弧をチェック
        if (check(TokenType::TOK_LBRACKET)) {
            is_array = true;
            
            // 配列次元を解析
            while (check(TokenType::TOK_LBRACKET)) {
                advance(); // consume '['
                
                if (check(TokenType::TOK_NUMBER)) {
                    int size = std::stoi(advance().value);
                    array_info.dimensions.push_back({size});
                } else if (check(TokenType::TOK_IDENTIFIER)) {
                    // 変数名を配列サイズとして使用
                    std::string size_name = advance().value;
                    array_info.dimensions.push_back({0}); // 動的サイズは0で表現
                } else {
                    // サイズが指定されていない場合
                    array_info.dimensions.push_back({0});
                }
                
                consume(TokenType::TOK_RBRACKET, "Expected ']'");
            }
        }
        
        if (match(TokenType::TOK_ASSIGN)) {
            init_expr = std::unique_ptr<ASTNode>(parseExpression());
        }
        
        variables.emplace_back(var_name, std::move(init_expr), array_info, is_array);
        
    } while (match(TokenType::TOK_COMMA));
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';'");
    
    // 単一変数の場合は従来通りAST_VAR_DECL、複数の場合はAST_MULTIPLE_VAR_DECL
    if (variables.size() == 1) {
        ASTNode* node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        node->name = variables[0].name;
        node->type_name = var_type;
        
        // 配列情報を設定
        if (variables[0].is_array) {
            node->array_type_info = variables[0].array_info;
        }
        
        // Set type_info based on type string - typedef aliasは後でインタープリターで解決
        if (variables[0].is_array) {
            // 配列の場合、基本型を設定
            if (var_type.substr(0, 3) == "int") {
                node->type_info = TYPE_INT;
                node->array_type_info.base_type = TYPE_INT;
            } else if (var_type.substr(0, 4) == "char") {
                node->type_info = TYPE_CHAR;
                node->array_type_info.base_type = TYPE_CHAR;
            } else if (var_type.substr(0, 4) == "bool") {
                node->type_info = TYPE_BOOL;
                node->array_type_info.base_type = TYPE_BOOL;
            } else if (var_type.substr(0, 4) == "long") {
                node->type_info = TYPE_LONG;
                node->array_type_info.base_type = TYPE_LONG;
            } else if (var_type.substr(0, 5) == "short") {
                node->type_info = TYPE_SHORT;
                node->array_type_info.base_type = TYPE_SHORT;
            } else if (var_type.substr(0, 4) == "tiny") {
                node->type_info = TYPE_TINY;
                node->array_type_info.base_type = TYPE_TINY;
            } else if (var_type.substr(0, 6) == "string") {
                node->type_info = TYPE_STRING;
                node->array_type_info.base_type = TYPE_STRING;
            } else {
                // 構造体配列または typedef aliasの可能性
                if (struct_definitions_.find(var_type) != struct_definitions_.end()) {
                    node->type_info = TYPE_STRUCT;
                    node->array_type_info.base_type = TYPE_STRUCT;
                } else {
                    node->type_info = TYPE_UNKNOWN;
                    node->array_type_info.base_type = TYPE_UNKNOWN;
                }
            }
        } else {
            // 通常の変数の場合
            if (var_type.substr(0, 3) == "int") {
                node->type_info = TYPE_INT;
            } else if (var_type.substr(0, 4) == "char") {
                node->type_info = TYPE_CHAR;
            } else if (var_type.substr(0, 4) == "bool") {
                node->type_info = TYPE_BOOL;
            } else if (var_type.substr(0, 4) == "long") {
                node->type_info = TYPE_LONG;
            } else if (var_type.substr(0, 5) == "short") {
                node->type_info = TYPE_SHORT;
            } else if (var_type.substr(0, 4) == "tiny") {
                node->type_info = TYPE_TINY;
            } else if (var_type.substr(0, 6) == "string") {
                node->type_info = TYPE_STRING;
            } else {
                // 構造体または typedef aliasの可能性
                if (struct_definitions_.find(var_type) != struct_definitions_.end()) {
                    node->type_info = TYPE_STRUCT;
                } else {
                    node->type_info = TYPE_UNKNOWN;
                }
            }
        }
        
        if (variables[0].init_expr) {
            node->init_expr = std::move(variables[0].init_expr);
        }
        
        return node;
    } else {
        // 複数変数宣言の場合
        ASTNode* node = new ASTNode(ASTNodeType::AST_MULTIPLE_VAR_DECL);
        node->type_name = var_type;
        
        // Set type_info based on type string
        if (var_type.substr(0, 3) == "int") {
            node->type_info = TYPE_INT;
        } else if (var_type.substr(0, 4) == "char") {
            node->type_info = TYPE_CHAR;
        } else if (var_type.substr(0, 4) == "bool") {
            node->type_info = TYPE_BOOL;
        } else if (var_type.substr(0, 4) == "long") {
            node->type_info = TYPE_LONG;
        } else if (var_type.substr(0, 5) == "short") {
            node->type_info = TYPE_SHORT;
        } else if (var_type.substr(0, 4) == "tiny") {
            node->type_info = TYPE_TINY;
        } else if (var_type.substr(0, 6) == "string") {
            node->type_info = TYPE_STRING;
        } else {
            node->type_info = TYPE_UNKNOWN;
        }
        
        // 各変数を子ノードとして追加
        for (auto& var : variables) {
            ASTNode* var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
            var_node->name = var.name;
            var_node->type_name = var_type;
            var_node->type_info = node->type_info;
            
            // 配列情報を設定
            if (var.is_array) {
                var_node->array_type_info = var.array_info;
                // 配列の場合、base_typeも設定
                if (var_type.substr(0, 3) == "int") {
                    var_node->array_type_info.base_type = TYPE_INT;
                } else if (var_type.substr(0, 4) == "char") {
                    var_node->array_type_info.base_type = TYPE_CHAR;
                } else if (var_type.substr(0, 4) == "bool") {
                    var_node->array_type_info.base_type = TYPE_BOOL;
                } else if (var_type.substr(0, 4) == "long") {
                    var_node->array_type_info.base_type = TYPE_LONG;
                } else if (var_type.substr(0, 5) == "short") {
                    var_node->array_type_info.base_type = TYPE_SHORT;
                } else if (var_type.substr(0, 4) == "tiny") {
                    var_node->array_type_info.base_type = TYPE_TINY;
                } else if (var_type.substr(0, 6) == "string") {
                    var_node->array_type_info.base_type = TYPE_STRING;
                } else {
                    // 構造体配列または typedef aliasの可能性
                    if (struct_definitions_.find(var_type) != struct_definitions_.end()) {
                        var_node->array_type_info.base_type = TYPE_STRUCT;
                    } else {
                        var_node->array_type_info.base_type = TYPE_UNKNOWN;
                    }
                }
            }
            
            if (var.init_expr) {
                var_node->init_expr = std::move(var.init_expr);
            }
            
            node->children.push_back(std::unique_ptr<ASTNode>(var_node));
        }
        
        return node;
    }
}

std::string RecursiveParser::parseType() {
    std::string base_type;
    
    if (check(TokenType::TOK_INT)) {
        advance(); // consume 'int'
        base_type = "int";
    } else if (check(TokenType::TOK_LONG)) {
        advance(); // consume 'long'
        base_type = "long";
    } else if (check(TokenType::TOK_SHORT)) {
        advance(); // consume 'short'
        base_type = "short";
    } else if (check(TokenType::TOK_TINY)) {
        advance(); // consume 'tiny'
        base_type = "tiny";
    } else if (check(TokenType::TOK_VOID)) {
        advance(); // consume 'void'
        base_type = "void";
    } else if (check(TokenType::TOK_BOOL)) {
        advance(); // consume 'bool'
        base_type = "bool";
    } else if (check(TokenType::TOK_STRING_TYPE)) {
        advance(); // consume 'string'
        base_type = "string";
    } else if (check(TokenType::TOK_CHAR_TYPE)) {
        advance(); // consume 'char'
        base_type = "char";
    } else if (check(TokenType::TOK_STRUCT)) {
        advance(); // consume 'struct'
        if (!check(TokenType::TOK_IDENTIFIER)) {
            error("Expected struct name after 'struct'");
            return "";
        }
        std::string struct_name = current_token_.value;
        advance();
        base_type = "struct " + struct_name;
    } else if (check(TokenType::TOK_IDENTIFIER)) {
        // typedef aliasまたはstruct型名の可能性
        std::string identifier = current_token_.value;
        if (typedef_map_.find(identifier) != typedef_map_.end()) {
            // typedef aliasが見つかった場合、再帰的に型を解決
            advance(); // consume identifier
            base_type = resolveTypedefChain(identifier);
        } else if (struct_definitions_.find(identifier) != struct_definitions_.end()) {
            // struct型名として認識
            advance(); // consume identifier  
            base_type = identifier; // typedef structの場合のstruct型
        } else {
            // 未知のidentifier
            base_type = advance().value; // とりあえずそのまま返す（インタープリターで解決）
        }
    } else {
        error("Expected type specifier");
        return "";
    }
    
    // 配列型のチェック int[size]
    if (check(TokenType::TOK_LBRACKET)) {
        advance(); // consume '['
        
        if (check(TokenType::TOK_NUMBER)) {
            Token size_token = advance();
            base_type += "[" + size_token.value + "]";
        } else {
            // 動的配列 int[]
            base_type += "[]";
        }
        
        consume(TokenType::TOK_RBRACKET, "Expected ']' in array type");
    }
    
    return base_type;
}

ASTNode* RecursiveParser::parseExpression() {
    return parseAssignment();
}

ASTNode* RecursiveParser::parseAssignment() {
    ASTNode* left = parseTernary();
    
    // 通常の代入と複合代入演算子をチェック
    if (check(TokenType::TOK_ASSIGN) || check(TokenType::TOK_PLUS_ASSIGN) || 
        check(TokenType::TOK_MINUS_ASSIGN) || check(TokenType::TOK_MUL_ASSIGN) ||
        check(TokenType::TOK_DIV_ASSIGN) || check(TokenType::TOK_MOD_ASSIGN) ||
        check(TokenType::TOK_AND_ASSIGN) || check(TokenType::TOK_OR_ASSIGN) ||
        check(TokenType::TOK_XOR_ASSIGN) || check(TokenType::TOK_LSHIFT_ASSIGN) ||
        check(TokenType::TOK_RSHIFT_ASSIGN)) {
        
        TokenType op_type = current_token_.type;
        std::string op_value = current_token_.value;
        advance(); // consume assignment operator
        
        ASTNode* right = parseAssignment(); // Right associative
        
        ASTNode* assign = new ASTNode(ASTNodeType::AST_ASSIGN);
        
        // leftが変数か配列参照でない場合はエラー
        if (left->node_type == ASTNodeType::AST_VARIABLE) {
            // 複合代入の場合、a += b を a = a + b に変換
            if (op_type != TokenType::TOK_ASSIGN) {
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
                
                // a = a op b の形に変換
                ASTNode* var_ref = new ASTNode(ASTNodeType::AST_VARIABLE);
                var_ref->name = left->name;
                
                ASTNode* binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                binop->op = binary_op;
                binop->left = std::unique_ptr<ASTNode>(var_ref);
                binop->right = std::unique_ptr<ASTNode>(right);
                
                assign->name = left->name;
                assign->right = std::unique_ptr<ASTNode>(binop);
            } else {
                assign->name = left->name;
                assign->right = std::unique_ptr<ASTNode>(right);
            }
            delete left; // leftノードはもう不要
        } else if (left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 配列要素への複合代入の場合
            if (op_type != TokenType::TOK_ASSIGN) {
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
                
                // arr[i] = arr[i] op b の形に変換
                ASTNode* array_ref_copy = new ASTNode(ASTNodeType::AST_ARRAY_REF);
                // 配列参照をコピー（左辺と右辺で同じものを参照）
                ASTNode* var_copy = new ASTNode(ASTNodeType::AST_VARIABLE);
                var_copy->name = static_cast<ASTNode*>(left->left.get())->name;
                array_ref_copy->left = std::unique_ptr<ASTNode>(var_copy);
                
                // インデックス式をディープコピー
                ASTNode* index_copy = nullptr;
                if (left->array_index) {
                    // 簡単なケースのみサポート（変数やリテラル）
                    if (left->array_index->node_type == ASTNodeType::AST_VARIABLE) {
                        index_copy = new ASTNode(ASTNodeType::AST_VARIABLE);
                        index_copy->name = left->array_index->name;
                    } else if (left->array_index->node_type == ASTNodeType::AST_NUMBER) {
                        index_copy = new ASTNode(ASTNodeType::AST_NUMBER);
                        index_copy->int_value = left->array_index->int_value;
                    }
                }
                array_ref_copy->array_index = std::unique_ptr<ASTNode>(index_copy);
                
                ASTNode* binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                binop->op = binary_op;
                binop->left = std::unique_ptr<ASTNode>(array_ref_copy);
                binop->right = std::unique_ptr<ASTNode>(right);
                
                assign->left = std::unique_ptr<ASTNode>(left);
                assign->right = std::unique_ptr<ASTNode>(binop);
            } else {
                assign->left = std::unique_ptr<ASTNode>(left);
                assign->right = std::unique_ptr<ASTNode>(right);
            }
        } else if (left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // メンバアクセス代入: obj.member = value
            // 複合代入は現時点では未対応
            if (op_type != TokenType::TOK_ASSIGN) {
                error("Compound assignment not supported for struct members yet");
                return nullptr;
            }
            assign->left = std::unique_ptr<ASTNode>(left);
            assign->right = std::unique_ptr<ASTNode>(right);
        } else {
            error("Invalid assignment target");
            return nullptr;
        }
        
        return assign;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseTernary() {
    ASTNode* condition = parseLogicalOr();
    
    if (check(TokenType::TOK_QUESTION)) {
        advance(); // consume '?'
        ASTNode* true_expr = parseExpression();
        consume(TokenType::TOK_COLON, "Expected ':' in ternary expression");
        ASTNode* false_expr = parseTernary();
        
        ASTNode* ternary = new ASTNode(ASTNodeType::AST_TERNARY_OP);
        ternary->left = std::unique_ptr<ASTNode>(condition);
        ternary->right = std::unique_ptr<ASTNode>(true_expr);
        ternary->third = std::unique_ptr<ASTNode>(false_expr);
        
        return ternary;
    }
    
    return condition;
}

ASTNode* RecursiveParser::parseLogicalOr() {
    ASTNode* left = parseLogicalAnd();
    
    while (check(TokenType::TOK_OR)) {
        Token op = advance();
        ASTNode* right = parseLogicalAnd();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseLogicalAnd() {
    ASTNode* left = parseBitwiseOr();
    
    while (check(TokenType::TOK_AND)) {
        Token op = advance();
        ASTNode* right = parseBitwiseOr();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseBitwiseOr() {
    ASTNode* left = parseBitwiseXor();
    
    while (check(TokenType::TOK_BIT_OR)) {
        Token op = advance();
        ASTNode* right = parseBitwiseXor();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseBitwiseXor() {
    ASTNode* left = parseBitwiseAnd();
    
    while (check(TokenType::TOK_BIT_XOR)) {
        Token op = advance();
        ASTNode* right = parseBitwiseAnd();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseBitwiseAnd() {
    ASTNode* left = parseComparison();
    
    while (check(TokenType::TOK_BIT_AND)) {
        Token op = advance();
        ASTNode* right = parseComparison();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseComparison() {
    ASTNode* left = parseShift();
    
    while (check(TokenType::TOK_EQ) || check(TokenType::TOK_NE) || 
           check(TokenType::TOK_LT) || check(TokenType::TOK_LE) ||
           check(TokenType::TOK_GT) || check(TokenType::TOK_GE)) {
        Token op = advance();
        ASTNode* right = parseShift();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseShift() {
    ASTNode* left = parseAdditive();
    
    while (check(TokenType::TOK_LEFT_SHIFT) || check(TokenType::TOK_RIGHT_SHIFT)) {
        Token op = advance();
        ASTNode* right = parseAdditive();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseAdditive() {
    ASTNode* left = parseMultiplicative();
    
    while (check(TokenType::TOK_PLUS) || check(TokenType::TOK_MINUS)) {
        Token op = advance();
        ASTNode* right = parseMultiplicative();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseMultiplicative() {
    ASTNode* left = parseUnary();
    
    while (check(TokenType::TOK_MUL) || check(TokenType::TOK_DIV) || check(TokenType::TOK_MOD)) {
        Token op = advance();
        ASTNode* right = parseUnary();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

ASTNode* RecursiveParser::parseUnary() {
    // Prefix operators: !, -, ++, --, ~
    if (check(TokenType::TOK_NOT) || check(TokenType::TOK_MINUS) || 
        check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR) || 
        check(TokenType::TOK_BIT_NOT)) {
        Token op = advance();
        ASTNode* operand = parseUnary();
        
        ASTNode* unary = new ASTNode(ASTNodeType::AST_UNARY_OP);
        unary->op = op.value;
        unary->left = std::unique_ptr<ASTNode>(operand);
        
        return unary;
    }
    
    return parsePostfix();
}

ASTNode* RecursiveParser::parsePostfix() {
    ASTNode* primary = parsePrimary();
    
    while (true) {
        if (check(TokenType::TOK_LBRACKET)) {
            // 配列アクセス: arr[i]
            advance(); // consume '['
            ASTNode* index = parseExpression();
            consume(TokenType::TOK_RBRACKET, "Expected ']'");
            
            ASTNode* array_ref = new ASTNode(ASTNodeType::AST_ARRAY_REF);
            array_ref->left = std::unique_ptr<ASTNode>(primary); // 左側を設定
            array_ref->array_index = std::unique_ptr<ASTNode>(index);
            
            // デバッグ: 配列アクセスノード作成をログ出力
            if (debug_mode && primary && primary->node_type == ASTNodeType::AST_VARIABLE) {
                std::cerr << "[DEBUG] Creating array access: " << primary->name << "[...]" << std::endl;
                std::cerr << "[DEBUG] array_ref->left: " << array_ref->left.get() << std::endl;
                std::cerr << "[DEBUG] array_ref->array_index: " << array_ref->array_index.get() << std::endl;
            }
            
            primary = array_ref; // 次のアクセスのベースとして設定
        } else if (check(TokenType::TOK_DOT)) {
            // メンバアクセス: obj.member
            primary = parseMemberAccess(primary);
        } else {
            break; // どちらでもない場合はループを抜ける
        }
    }
    
    // Postfix operators: ++, --
    if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
        Token op = advance();
        ASTNode* postfix = new ASTNode(ASTNodeType::AST_UNARY_OP);
        postfix->op = op.value + "_post"; // postfixを区別
        postfix->left = std::unique_ptr<ASTNode>(primary);
        return postfix;
    }
    
    return primary;
}

ASTNode* RecursiveParser::parsePrimary() {
    if (check(TokenType::TOK_NUMBER)) {
        Token token = advance();
        ASTNode* node = new ASTNode(ASTNodeType::AST_NUMBER);
        try {
            node->int_value = std::stoll(token.value);  // 64ビット整数対応
        } catch (const std::exception& e) {
            error("Invalid number: " + token.value);
            return nullptr;
        }
        return node;
    }
    
    if (check(TokenType::TOK_STRING)) {
        Token token = advance();
        ASTNode* node = new ASTNode(ASTNodeType::AST_STRING_LITERAL);
        node->str_value = token.value;
        return node;
    }
    
    if (check(TokenType::TOK_CHAR)) {
        Token token = advance();
        ASTNode* node = new ASTNode(ASTNodeType::AST_NUMBER);
        // 文字リテラルをASCII値として処理
        if (!token.value.empty()) {
            node->int_value = static_cast<int>(token.value[0]);
        } else {
            node->int_value = 0;
        }
        return node;
    }
    
    if (check(TokenType::TOK_TRUE) || check(TokenType::TOK_FALSE)) {
        Token token = advance();
        ASTNode* node = new ASTNode(ASTNodeType::AST_NUMBER); // bool値も数値として扱う
        node->int_value = (token.type == TokenType::TOK_TRUE) ? 1 : 0;
        return node;
    }
    
    if (check(TokenType::TOK_IDENTIFIER)) {
        Token token = advance();
        
        // 関数呼び出しをチェック
        if (check(TokenType::TOK_LPAREN)) {
            advance(); // consume '('
            
            ASTNode* call_node = new ASTNode(ASTNodeType::AST_FUNC_CALL);
            call_node->name = token.value;
            
            // 引数リストの解析
            if (!check(TokenType::TOK_RPAREN)) {
                do {
                    ASTNode* arg = parseExpression();
                    call_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                } while (match(TokenType::TOK_COMMA));
            }
            
            consume(TokenType::TOK_RPAREN, "Expected ')' after function arguments");
            return call_node;
        }
        // 配列アクセスは parsePostfix で処理
        else {
            ASTNode* node = new ASTNode(ASTNodeType::AST_VARIABLE);
            node->name = token.value;
            setLocation(node, token.line, token.column);
            return node;
        }
    }
    
    // 括弧式の処理
    if (check(TokenType::TOK_LPAREN)) {
        advance(); // consume '('
        ASTNode* expr = parseExpression();
        consume(TokenType::TOK_RPAREN, "Expected ')'");
        return expr;
    }
    
    // 配列リテラルの処理
    if (check(TokenType::TOK_LBRACKET)) {
        advance(); // consume '['
        
        ASTNode* array_literal = new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);
        
        // 空の配列リテラル []
        if (check(TokenType::TOK_RBRACKET)) {
            advance(); // consume ']'
            return array_literal;
        }
        
        // 配列要素を解析
        while (!check(TokenType::TOK_RBRACKET) && !isAtEnd()) {
            ASTNode* element = parseExpression();
            array_literal->arguments.push_back(std::unique_ptr<ASTNode>(element));
            
            if (check(TokenType::TOK_COMMA)) {
                advance(); // consume ','
            } else if (!check(TokenType::TOK_RBRACKET)) {
                error("Expected ',' or ']' in array literal");
                return nullptr;
            }
        }
        
        consume(TokenType::TOK_RBRACKET, "Expected ']' after array literal");
        return array_literal;
    }
    
    // 構造体リテラルの処理 {member: value, ...}
    if (check(TokenType::TOK_LBRACE)) {
        return parseStructLiteral();
    }
    
    error("Unexpected token");
    return nullptr;
}

ASTNode* RecursiveParser::parseFunctionDeclarationAfterName(const std::string& return_type, const std::string& function_name) {
    // '(' を期待（すでにチェック済み）
    consume(TokenType::TOK_LPAREN, "Expected '(' after function name");
    
    // 関数本体のパース
    ASTNode* function_node = new ASTNode(ASTNodeType::AST_FUNC_DECL);
    function_node->name = function_name;
    
    // DEBUG: 関数作成をログ出力
    if (debug_mode) {
        std::cerr << "[DEBUG] Created function: " << function_name << std::endl;
    }
    
    // パラメータリストの解析
    if (!check(TokenType::TOK_RPAREN)) {
        do {
            // パラメータ型
            std::string param_type = parseType();
            
            // パラメータ名
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected parameter name");
                return nullptr;
            }
            
            Token param_name = advance();
            
            // パラメータノードを作成
            ASTNode* param = new ASTNode(ASTNodeType::AST_PARAM_DECL);
            param->name = param_name.value;
            param->type_name = param_type;
            
            // 配列パラメータかチェック
            param->is_array = (param_type.find("[") != std::string::npos);
            
            // 型情報を設定（typedef解決を含む）
            std::string resolved_param_type = resolveTypedefChain(param_type);
            param->type_name = resolved_param_type;  // 解決された型名を保存
            
            if (resolved_param_type.find("[") != std::string::npos) {
                // 配列パラメータの場合
                param->is_array = true;
                std::string base_type = resolved_param_type.substr(0, resolved_param_type.find("["));
                if (base_type == "int") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
                } else if (base_type == "string") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
                } else if (base_type == "bool") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL);
                } else if (base_type == "long") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_LONG);
                } else if (base_type == "short") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_SHORT);
                } else if (base_type == "tiny") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_TINY);
                } else if (base_type == "char") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_CHAR);
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
            } else {
                param->type_info = TYPE_UNKNOWN;
            }
            
            function_node->parameters.push_back(std::unique_ptr<ASTNode>(param));
        } while (match(TokenType::TOK_COMMA));
    }
    
    consume(TokenType::TOK_RPAREN, "Expected ')' after parameters");
    
    // return_typeをTypeInfo enum値として設定
    if (return_type.find("[") != std::string::npos) {
        // 配列戻り値型
        std::string base_type = return_type.substr(0, return_type.find("["));
        if (base_type == "int") {
            function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT));
        } else if (base_type == "string") {
            function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING));
        } else if (base_type == "bool") {
            function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL));
        } else if (base_type == "long") {
            function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_LONG));
        } else if (base_type == "short") {
            function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_SHORT));
        } else if (base_type == "tiny") {
            function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_TINY));
        } else if (base_type == "char") {
            function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_CHAR));
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
        std::string resolved_type = resolveTypedefChain(return_type);
        if (resolved_type != return_type) {
            // typedef型が解決された場合、再帰的に処理
            if (resolved_type.find("[") != std::string::npos) {
                // 配列戻り値型
                std::string base_type = resolved_type.substr(0, resolved_type.find("["));
                if (base_type == "int") {
                    function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT));
                } else if (base_type == "string") {
                    function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING));
                } else if (base_type == "bool") {
                    function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL));
                } else if (base_type == "long") {
                    function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_LONG));
                } else if (base_type == "short") {
                    function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_SHORT));
                } else if (base_type == "tiny") {
                    function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_TINY));
                } else if (base_type == "char") {
                    function_node->return_types.push_back(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_CHAR));
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
        } else {
            // typedef型でない場合、不明な型として処理
            function_node->return_types.push_back(TYPE_UNKNOWN);
        }
    }
    
    // 関数本体の開始 '{'
    consume(TokenType::TOK_LBRACE, "Expected '{' to start function body");
    
    // 文のリストノードを作成
    ASTNode* body_node = new ASTNode(ASTNodeType::AST_STMT_LIST);
    
    // 文の解析
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        ASTNode* stmt = parseStatement();
        if (stmt != nullptr) {
            body_node->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }
    
    // 関数本体の終了 '}'
    consume(TokenType::TOK_RBRACE, "Expected '}' to end function body");
    
    // bodyフィールドに設定
    function_node->body = std::unique_ptr<ASTNode>(body_node);
    
    return function_node;
}

ASTNode* RecursiveParser::parseFunctionDeclaration() {
    // この時点で既にintとidentifierとlparenが確認済み（または解析開始状態）
    // 戻り値の型を解析（まだ消費されていない場合）
    std::string return_type;
    if (check(TokenType::TOK_INT)) {
        return_type = "int";
        advance(); // consume 'int'
    } else {
        error("Expected return type");
        return nullptr;
    }
    
    // 関数名を解析  
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected function name");
        return nullptr;
    }
    
    Token name_token = advance();
    std::string function_name = name_token.value;
    
    // '(' を期待
    consume(TokenType::TOK_LPAREN, "Expected '(' after function name");
    
    // パラメータリスト（現在は空のみサポート）
    consume(TokenType::TOK_RPAREN, "Expected ')' after parameters");
    
    // 関数本体の開始 '{'
    consume(TokenType::TOK_LBRACE, "Expected '{' to start function body");
    
    // 関数本体のパース
    ASTNode* function_node = new ASTNode(ASTNodeType::AST_FUNC_DECL);
    function_node->name = function_name;
    
    // return_typeをTypeInfo enum値として設定
    if (return_type == "int") {
        function_node->return_types.push_back(TYPE_INT);
    } else {
        function_node->return_types.push_back(TYPE_UNKNOWN);
    }
    
    // 文の解析
    ASTNode* body_node = new ASTNode(ASTNodeType::AST_STMT_LIST);
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        ASTNode* stmt = parseStatement();
        if (stmt != nullptr) {
            body_node->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }
    
    // 関数本体の終了 '}'
    consume(TokenType::TOK_RBRACE, "Expected '}' to end function body");
    
    // bodyフィールドに設定
    function_node->body = std::unique_ptr<ASTNode>(body_node);
    
    return function_node;
}


ASTNode* RecursiveParser::parseTypedefDeclaration() {
    // typedef <type> <alias>; または typedef struct {...} <alias>;
    consume(TokenType::TOK_TYPEDEF, "Expected 'typedef'");
    
    // typedef struct の場合
    if (check(TokenType::TOK_STRUCT)) {
        return parseStructTypedefDeclaration();
    }
    
    // 基底型を解析（基本型またはtypedef型）
    TypeInfo base_type = TYPE_UNKNOWN;
    std::string base_type_name;
    
    if (check(TokenType::TOK_INT)) {
        base_type = TYPE_INT;
        base_type_name = "int";
        advance();
    } else if (check(TokenType::TOK_LONG)) {
        base_type = TYPE_LONG;
        base_type_name = "long";
        advance();
    } else if (check(TokenType::TOK_SHORT)) {
        base_type = TYPE_SHORT;
        base_type_name = "short";
        advance();
    } else if (check(TokenType::TOK_TINY)) {
        base_type = TYPE_TINY;
        base_type_name = "tiny";
        advance();
    } else if (check(TokenType::TOK_BOOL)) {
        base_type = TYPE_BOOL;
        base_type_name = "bool";
        advance();
    } else if (check(TokenType::TOK_STRING_TYPE)) {
        base_type = TYPE_STRING;
        base_type_name = "string";
        advance();
    } else if (check(TokenType::TOK_CHAR_TYPE)) {
        base_type = TYPE_CHAR;
        base_type_name = "char";
        advance();
    } else if (check(TokenType::TOK_VOID)) {
        base_type = TYPE_VOID;
        base_type_name = "void";
        advance();
    } else if (check(TokenType::TOK_IDENTIFIER)) {
        // 既存のtypedef型を参照する場合
        std::string typedef_name = advance().value;
        
        // typedef_map_でチェーンを解決
        std::string resolved_type = resolveTypedefChain(typedef_name);
        if (resolved_type.empty()) {
            error("Unknown typedef type: " + typedef_name);
            throw std::runtime_error("Unknown typedef type: " + typedef_name);
        }
        
        base_type_name = resolved_type;
        base_type = getTypeInfoFromString(extractBaseType(resolved_type));
    } else {
        error("Expected type after typedef");
        return nullptr;
    }
    
    // typedef ASTノードを作成
    ASTNode* typedef_node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
    typedef_node->type_info = base_type;
    
    // 配列次元の解析
    std::string array_dimensions_str = "";
    while (check(TokenType::TOK_LBRACKET)) {
        advance(); // consume '['
        
        std::string dimension_str = "[";
        
        ASTNode* size_expr = nullptr;
        if (check(TokenType::TOK_NUMBER)) {
            size_expr = new ASTNode(ASTNodeType::AST_NUMBER);
            size_expr->int_value = std::stoll(advance().value);
            dimension_str += std::to_string(size_expr->int_value);
        }
        
        dimension_str += "]";
        array_dimensions_str += dimension_str;
        
        consume(TokenType::TOK_RBRACKET, "Expected ']' after array size");
        
        typedef_node->array_dimensions.push_back(std::unique_ptr<ASTNode>(size_expr));
    }
    
    // 完全な型名を設定
    typedef_node->type_name = base_type_name + array_dimensions_str;
    
    // エイリアス名
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected typedef alias name");
        return nullptr;
    }
    
    typedef_node->name = advance().value;  // エイリアス名
    
    // typedefマップに登録
    typedef_map_[typedef_node->name] = typedef_node->type_name;
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after typedef");
    
    return typedef_node;
}

TypeInfo RecursiveParser::getTypeInfoFromString(const std::string& type_name) {
    if (type_name == "int") {
        return TYPE_INT;
    } else if (type_name == "long") {
        return TYPE_LONG;
    } else if (type_name == "short") {
        return TYPE_SHORT;
    } else if (type_name == "tiny") {
        return TYPE_TINY;
    } else if (type_name == "bool") {
        return TYPE_BOOL;
    } else if (type_name == "string") {
        return TYPE_STRING;
    } else if (type_name == "char") {
        return TYPE_CHAR;
    } else if (type_name == "void") {
        return TYPE_VOID;
    } else if (type_name.substr(0, 7) == "struct " || struct_definitions_.find(type_name) != struct_definitions_.end()) {
        return TYPE_STRUCT;
    } else {
        return TYPE_UNKNOWN;
    }
}

ASTNode* RecursiveParser::parseReturnStatement() {
    advance(); // consume 'return'
    ASTNode* return_node = new ASTNode(ASTNodeType::AST_RETURN_STMT);
    
    // return値があるかチェック
    if (!check(TokenType::TOK_SEMICOLON)) {
        return_node->left = std::unique_ptr<ASTNode>(parseExpression());
    }
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after return statement");
    return return_node;
}

ASTNode* RecursiveParser::parseBreakStatement() {
    advance(); // consume 'break'
    ASTNode* break_node = new ASTNode(ASTNodeType::AST_BREAK_STMT);
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after break statement");
    return break_node;
}

ASTNode* RecursiveParser::parseContinueStatement() {
    advance(); // consume 'continue'
    ASTNode* continue_node = new ASTNode(ASTNodeType::AST_CONTINUE_STMT);
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after continue statement");
    return continue_node;
}

ASTNode* RecursiveParser::parseIfStatement() {
    advance(); // consume 'if'
    consume(TokenType::TOK_LPAREN, "Expected '(' after if");
    
    ASTNode* if_node = new ASTNode(ASTNodeType::AST_IF_STMT);
    if_node->condition = std::unique_ptr<ASTNode>(parseExpression());
    
    consume(TokenType::TOK_RPAREN, "Expected ')' after if condition");
    
    // if本体をパース（then節はleftに格納してinterpreterと統一）
    if_node->left = std::unique_ptr<ASTNode>(parseStatement());
    
    // else節があるかチェック
    if (match(TokenType::TOK_ELSE)) {
        if_node->right = std::unique_ptr<ASTNode>(parseStatement());
    }
    
    return if_node;
}

ASTNode* RecursiveParser::parseForStatement() {
    advance(); // consume 'for'
    consume(TokenType::TOK_LPAREN, "Expected '(' after for");
    
    ASTNode* for_node = new ASTNode(ASTNodeType::AST_FOR_STMT);
    
    // 初期化部分 (int i = 0;) - 文として扱う
    for_node->init_expr = std::unique_ptr<ASTNode>(parseStatement());
    
    // 条件部分 (i < 5) - 式として扱う
    for_node->condition = std::unique_ptr<ASTNode>(parseExpression());
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after for condition");
    
    // 更新部分 - 一般的な式として処理（i++, i--, i=i+1など）
    for_node->update_expr = std::unique_ptr<ASTNode>(parseExpression());
    
    consume(TokenType::TOK_RPAREN, "Expected ')' after for update");
    
    // for本体
    for_node->body = std::unique_ptr<ASTNode>(parseStatement());
    
    return for_node;
}

ASTNode* RecursiveParser::parseWhileStatement() {
    advance(); // consume 'while'
    consume(TokenType::TOK_LPAREN, "Expected '(' after while");
    
    ASTNode* while_node = new ASTNode(ASTNodeType::AST_WHILE_STMT);
    
    // 条件部分
    while_node->condition = std::unique_ptr<ASTNode>(parseExpression());
    
    consume(TokenType::TOK_RPAREN, "Expected ')' after while condition");
    
    // while本体
    while_node->body = std::unique_ptr<ASTNode>(parseStatement());
    
    return while_node;
}

ASTNode* RecursiveParser::parseCompoundStatement() {
    advance(); // consume '{'
    
    ASTNode* compound = new ASTNode(ASTNodeType::AST_COMPOUND_STMT);
    
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        ASTNode* stmt = parseStatement();
        if (stmt) {
            compound->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }
    
    consume(TokenType::TOK_RBRACE, "Expected '}'");
    return compound;
}

ASTNode* RecursiveParser::parsePrintlnStatement() {
    advance(); // consume 'println'
    consume(TokenType::TOK_LPAREN, "Expected '(' after println");
    
    ASTNode* print_node = new ASTNode(ASTNodeType::AST_PRINTLN_STMT);
    
    // 複数の引数をパース
    if (!check(TokenType::TOK_RPAREN)) {
        do {
            ASTNode* arg = parseExpression();
            print_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
        } while (match(TokenType::TOK_COMMA));
    }
    
    consume(TokenType::TOK_RPAREN, "Expected ')' after println arguments");
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after println statement");
    return print_node;
}

ASTNode* RecursiveParser::parsePrintStatement() {
    advance(); // consume 'print'
    
    ASTNode* print_node = new ASTNode(ASTNodeType::AST_PRINT_STMT);
    
    // 引数をパース - 任意の式を受け入れる
    if (check(TokenType::TOK_LPAREN)) {
        // print(expression[, expression, ...]); 形式
        advance(); // consume '('
        
        // 複数の引数をパース
        if (!check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode* arg = parseExpression();
                print_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
            } while (match(TokenType::TOK_COMMA));
        }
        
        consume(TokenType::TOK_RPAREN, "Expected ')' after print arguments");
    } else if (!check(TokenType::TOK_SEMICOLON)) {
        // print expression; 形式（括弧なし）
        print_node->left = std::unique_ptr<ASTNode>(parseExpression());
    } else {
        error("Expected expression after print");
        return nullptr;
    }
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after print statement");
    return print_node;
}

// 位置情報設定のヘルパーメソッド
void RecursiveParser::setLocation(ASTNode* node, const Token& token) {
    if (node) {
        node->location.filename = filename_;
        node->location.line = token.line;
        node->location.column = token.column;
        node->location.source_line = getSourceLine(token.line);
    }
}

void RecursiveParser::setLocation(ASTNode* node, int line, int column) {
    if (node) {
        node->location.filename = filename_;
        node->location.line = line;
        node->location.column = column;
        node->location.source_line = getSourceLine(line);
    }
}

std::string RecursiveParser::getSourceLine(int line_number) {
    // 1-based line numberを0-basedインデックスに変換
    if (line_number < 1 || line_number > static_cast<int>(source_lines_.size())) {
        return "";
    }
    return source_lines_[line_number - 1];
}

// typedef型のチェーンを解決する
std::string RecursiveParser::resolveTypedefChain(const std::string& typedef_name) {
    std::unordered_set<std::string> visited;
    std::string current = typedef_name;
    
    while (typedef_map_.find(current) != typedef_map_.end()) {
        if (visited.find(current) != visited.end()) {
            // 循環参照検出
            return "";
        }
        visited.insert(current);
        
        std::string next = typedef_map_[current];
        // 次がtypedef型かチェック
        if (typedef_map_.find(next) != typedef_map_.end()) {
            current = next;
        } else {
            // 基本型に到達
            return next;
        }
    }
    
    // 基本型かチェック
    if (current == "int" || current == "long" || current == "short" || 
        current == "tiny" || current == "bool" || current == "string" || 
        current == "char" || current == "void") {
        return current;
    }
    
    // 未定義型の場合は空文字列を返す
    return "";
}

// 型名から基本型部分を抽出する

ASTNode* RecursiveParser::parseTypedefVariableDeclaration() {
    // typedef型名を取得
    std::string typedef_name = advance().value;
    std::string resolved_type = resolveTypedefChain(typedef_name);
    
    // 変数名を取得
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected variable name after typedef type");
        return nullptr;
    }
    
    std::string var_name = advance().value;
    
    // 変数宣言ノードを作成
    ASTNode* node = new ASTNode(ASTNodeType::AST_VAR_DECL);
    node->name = var_name;
    node->type_name = typedef_name;  // 元のtypedef名を保持
    
    // 実際の型から型情報を設定
    if (resolved_type.find("[") != std::string::npos) {
        // 配列型の場合
        std::string base_type = resolved_type.substr(0, resolved_type.find("["));
        node->type_info = getTypeInfoFromString(base_type);
        
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
        for (const auto& size : array_sizes) {
            if (!size.empty() && std::all_of(size.begin(), size.end(), ::isdigit)) {
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
        if (debug_mode_) {
            std::cerr << "DEBUG: Parser setting array_type_info for " << node->name 
                      << " with base_type=" << node->array_type_info.base_type << std::endl;
        }
    } else {
        // 基本型の場合
        node->type_info = getTypeInfoFromString(resolved_type);
    }
    
    // 初期化式のチェック
    if (check(TokenType::TOK_ASSIGN)) {
        advance(); // consume '='
        
        if (check(TokenType::TOK_LBRACKET)) {
            // 配列リテラル初期化
            advance(); // consume '['
            
            ASTNode* array_literal = new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);
            while (!check(TokenType::TOK_RBRACKET) && !isAtEnd()) {
                ASTNode* element = parseExpression();
                array_literal->arguments.push_back(std::unique_ptr<ASTNode>(element));
                
                if (check(TokenType::TOK_COMMA)) {
                    advance(); // consume ','
                } else if (!check(TokenType::TOK_RBRACKET)) {
                    error("Expected ',' or ']' in array literal");
                    return nullptr;
                }
            }
            
            consume(TokenType::TOK_RBRACKET, "Expected ']' after array literal");
            node->init_expr = std::unique_ptr<ASTNode>(array_literal);
        } else {
            // 通常の式
            ASTNode* expr = parseExpression();
            node->init_expr = std::unique_ptr<ASTNode>(expr);
        }
    }
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after variable declaration");
    return node;
}
std::string RecursiveParser::extractBaseType(const std::string& type_name) {
    // 配列部分 [n] を除去して基本型を取得
    size_t bracket_pos = type_name.find('[');
    if (bracket_pos != std::string::npos) {
        return type_name.substr(0, bracket_pos);
    }
    return type_name;
}

// struct宣言の解析: struct name { members };
ASTNode* RecursiveParser::parseStructDeclaration() {
    consume(TokenType::TOK_STRUCT, "Expected 'struct'");
    
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected struct name");
        return nullptr;
    }
    
    std::string struct_name = current_token_.value;
    advance(); // struct名をスキップ
    
    consume(TokenType::TOK_LBRACE, "Expected '{' after struct name");
    
    StructDefinition struct_def(struct_name);
    
    // メンバ変数の解析
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        // メンバの型を解析
        std::string member_type = parseType();
        
        if (member_type.empty()) {
            error("Expected member type in struct definition");
            return nullptr;
        }
        
        // メンバ名のリストを解析（int a, b, c; のような複数宣言に対応）
        do {
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected member name");
                return nullptr;
            }
            
            std::string member_name = current_token_.value;
            advance();
            
            // 配列宣言のチェック ([size1][size2]...)
            if (check(TokenType::TOK_LBRACKET)) {
                std::vector<ArrayDimension> dimensions;
                
                // 多次元配列の各次元を解析
                while (check(TokenType::TOK_LBRACKET)) {
                    advance(); // '[' をスキップ
                    
                    if (check(TokenType::TOK_NUMBER)) {
                        int array_size = std::stoi(current_token_.value);
                        advance(); // サイズをスキップ
                        dimensions.push_back(ArrayDimension(array_size, false));
                    } else if (check(TokenType::TOK_IDENTIFIER)) {
                        std::string size_expr = current_token_.value;
                        advance(); // 識別子をスキップ
                        // 定数識別子として扱い、後でランタイムで解決
                        dimensions.push_back(ArrayDimension(-1, true, size_expr));
                    } else {
                        error("Expected array size or constant identifier in struct member");
                        return nullptr;
                    }
                    
                    consume(TokenType::TOK_RBRACKET, "Expected ']' after array size");
                }
                
                // 多次元配列メンバを追加
                TypeInfo member_type_info = getTypeInfoFromString(member_type);
                StructMember array_member(member_name, member_type_info, member_type);
                array_member.array_info = ArrayTypeInfo(member_type_info, dimensions);
                struct_def.members.push_back(array_member);
                
                // Debug output removed - use --debug option if needed
            } else {
                // 通常のメンバを追加
                TypeInfo member_type_info = getTypeInfoFromString(member_type);
                struct_def.add_member(member_name, member_type_info, member_type);
                
                // Debug output removed - use --debug option if needed
            }
            
            if (check(TokenType::TOK_COMMA)) {
                advance(); // カンマをスキップ
                continue;
            } else {
                break;
            }
        } while (true);
        
        consume(TokenType::TOK_SEMICOLON, "Expected ';' after struct member");
    }
    
    consume(TokenType::TOK_RBRACE, "Expected '}' after struct members");
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after struct definition");
    
    // struct定義をパーサー内に保存（変数宣言の認識のため）
    struct_definitions_[struct_name] = struct_def;
    
    // Debug output removed - use --debug option if needed
    
    // ASTノードを作成してstruct定義情報を保存
    ASTNode* node = new ASTNode(ASTNodeType::AST_STRUCT_DECL);
    node->name = struct_name;
    setLocation(node, current_token_);
    
    // struct定義情報をASTノードに保存
    for (const auto& member : struct_def.members) {
        ASTNode* member_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        member_node->name = member.name;
        member_node->type_name = member.type_alias;
        member_node->type_info = member.type;
        member_node->array_type_info = member.array_info;  // 配列情報をコピー
        node->arguments.push_back(std::unique_ptr<ASTNode>(member_node));
    }
    
    return node;
}

// typedef struct宣言の解析: typedef struct { members } alias;
ASTNode* RecursiveParser::parseStructTypedefDeclaration() {
    consume(TokenType::TOK_STRUCT, "Expected 'struct'");
    
    consume(TokenType::TOK_LBRACE, "Expected '{' after 'typedef struct'");
    
    StructDefinition struct_def;
    
    // メンバ変数の解析
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        // メンバの型を解析
        std::string member_type = parseType();
        
        if (member_type.empty()) {
            error("Expected member type in struct definition");
            return nullptr;
        }
        
        // メンバ名のリストを解析
        do {
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected member name");
                return nullptr;
            }
            
            std::string member_name = current_token_.value;
            advance();
            
            // メンバを追加
            TypeInfo member_type_info = getTypeInfoFromString(member_type);
            struct_def.add_member(member_name, member_type_info, member_type);
            
            if (check(TokenType::TOK_COMMA)) {
                advance(); // カンマをスキップ
                continue;
            } else {
                break;
            }
        } while (true);
        
        consume(TokenType::TOK_SEMICOLON, "Expected ';' after struct member");
    }
    
    consume(TokenType::TOK_RBRACE, "Expected '}' after struct members");
    
    // typedef エイリアス名を取得
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected typedef alias name");
        return nullptr;
    }
    
    std::string alias_name = current_token_.value;
    advance();
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after typedef struct declaration");
    
    // struct定義を保存（エイリアス名で）
    struct_def.name = alias_name;
    struct_definitions_[alias_name] = struct_def;
    
    // typedef マッピングも追加
    typedef_map_[alias_name] = "struct " + alias_name;
    
    // ASTノードを作成
    ASTNode* node = new ASTNode(ASTNodeType::AST_STRUCT_TYPEDEF_DECL);
    node->name = alias_name;
    setLocation(node, current_token_);
    
    return node;
}

// メンバアクセスの解析: obj.member
ASTNode* RecursiveParser::parseMemberAccess(ASTNode* object) {
    consume(TokenType::TOK_DOT, "Expected '.'");
    
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected member name after '.'");
        return nullptr;
    }
    
    std::string member_name = current_token_.value;
    advance();
    
    // ネストしたメンバーアクセスの検出 (obj.member.submember)
    if (check(TokenType::TOK_DOT)) {
        error("Nested member access (obj.member.submember) is not supported yet. Consider using pointers in future implementation.");
        return nullptr;
    }
    
    ASTNode* member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
    member_access->left = std::unique_ptr<ASTNode>(object);
    member_access->name = member_name; // メンバ名を保存
    setLocation(member_access, current_token_);
    
    return member_access;
}

// 構造体リテラルの解析: {member: value, member2: value2}
ASTNode* RecursiveParser::parseStructLiteral() {
    consume(TokenType::TOK_LBRACE, "Expected '{'");
    
    ASTNode* struct_literal = new ASTNode(ASTNodeType::AST_STRUCT_LITERAL);
    
    // 空の構造体リテラル {}
    if (check(TokenType::TOK_RBRACE)) {
        advance(); // consume '}'
        return struct_literal;
    }
    
    // 最初の要素をチェックして、名前付きか位置ベースかを判断
    bool is_named_initialization = false;
    
    // 先読みして名前付き初期化かチェック
    if (check(TokenType::TOK_IDENTIFIER)) {
        RecursiveLexer temp_lexer = lexer_;
        Token temp_current = current_token_;
        advance();
        if (check(TokenType::TOK_COLON)) {
            is_named_initialization = true;
        }
        lexer_ = temp_lexer;
        current_token_ = temp_current;
    }
    
    if (is_named_initialization) {
        // 名前付き初期化: {name: "Bob", age: 25}
        while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
            // メンバ名の解析
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected member name in struct literal");
                return nullptr;
            }
            
            std::string member_name = current_token_.value;
            advance();
            
            consume(TokenType::TOK_COLON, "Expected ':' after member name");
            
            // メンバ値の解析
            ASTNode* member_value = parseExpression();
            
            // メンバ代入ノードを作成（name: valueの形で保存）
            ASTNode* member_init = new ASTNode(ASTNodeType::AST_ASSIGN);
            member_init->name = member_name;
            member_init->right = std::unique_ptr<ASTNode>(member_value);
            
            struct_literal->arguments.push_back(std::unique_ptr<ASTNode>(member_init));
            
            if (check(TokenType::TOK_COMMA)) {
                advance(); // consume ','
            } else if (!check(TokenType::TOK_RBRACE)) {
                error("Expected ',' or '}' in struct literal");
                return nullptr;
            }
        }
    } else {
        // 位置ベース初期化: {25, "Bob"}
        while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
            ASTNode* value = parseExpression();
            struct_literal->arguments.push_back(std::unique_ptr<ASTNode>(value));
            
            if (check(TokenType::TOK_COMMA)) {
                advance(); // consume ','
            } else if (!check(TokenType::TOK_RBRACE)) {
                error("Expected ',' or '}' in struct literal");
                return nullptr;
            }
        }
    }
    
    consume(TokenType::TOK_RBRACE, "Expected '}' after struct literal");
    return struct_literal;
}

// 配列リテラルの解析: [{}, {}, ...] または [1, 2, 3]
ASTNode* RecursiveParser::parseArrayLiteral() {
    consume(TokenType::TOK_LBRACKET, "Expected '[' at start of array literal");
    
    ASTNode* array_literal = new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);
    
    // 空の配列リテラル []
    if (check(TokenType::TOK_RBRACKET)) {
        advance(); // consume ']'
        return array_literal;
    }
    
    // 配列要素を解析
    while (!check(TokenType::TOK_RBRACKET) && !isAtEnd()) {
        ASTNode* element;
        
        if (check(TokenType::TOK_LBRACE)) {
            // struct literal要素: {25, "Alice"}
            element = parseStructLiteral();
        } else {
            // 通常の式要素
            element = parseExpression();
        }
        
        array_literal->arguments.push_back(std::unique_ptr<ASTNode>(element));
        
        if (check(TokenType::TOK_COMMA)) {
            advance(); // consume ','
        } else if (!check(TokenType::TOK_RBRACKET)) {
            error("Expected ',' or ']' in array literal");
            return nullptr;
        }
    }
    
    consume(TokenType::TOK_RBRACKET, "Expected ']' after array literal");
    return array_literal;
}
