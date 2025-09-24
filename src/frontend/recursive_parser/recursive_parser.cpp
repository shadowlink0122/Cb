#include "recursive_parser.h"
#include "../../backend/error_handler.h"
#include "../../common/debug.h"
#include <iostream>
#include <sstream>

using namespace RecursiveParserNS;

RecursiveParser::RecursiveParser(const std::string& source, const std::string& filename) 
    : lexer_(source), current_token_(TokenType::TOK_EOF, "", 0, 0), filename_(filename), source_(source) {
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
        
        // typedef alias変数宣言の可能性をチェック: TypeAlias variableName;
        if (check(TokenType::TOK_IDENTIFIER)) {
            // これはtypedef alias変数宣言: TypeAlias variableName;
            std::string var_name = advance().value;
            
            ASTNode* node = new ASTNode(ASTNodeType::AST_VAR_DECL);
            node->name = var_name;
            node->type_name = name;  // typedef alias名を記録
            node->type_info = TYPE_UNKNOWN;  // インタープリターで解決
            node->is_const = isConst;
            
            if (match(TokenType::TOK_ASSIGN)) {
                node->init_expr = std::unique_ptr<ASTNode>(parseExpression());
            }
            
            consume(TokenType::TOK_SEMICOLON, "Expected ';'");
            return node;
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
            
            consume(TokenType::TOK_ASSIGN, "Expected '='");
            ASTNode* value_expr = parseExpression();
            consume(TokenType::TOK_SEMICOLON, "Expected ';'");
            
            // 代入ノードを作成
            ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
            assignment->left = std::unique_ptr<ASTNode>(left_expr);
            assignment->right = std::unique_ptr<ASTNode>(value_expr);
            
            return assignment;
        } else if (check(TokenType::TOK_ASSIGN)) {
            // 通常の代入: identifier = value
            consume(TokenType::TOK_ASSIGN, "Expected '='");
            
            // 配列リテラルかどうかをチェック
            if (check(TokenType::TOK_LBRACKET)) {
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
                // 通常の式
                ASTNode* expr = parseExpression();
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                
                ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                assignment->name = name;
                assignment->right = std::unique_ptr<ASTNode>(expr);
                return assignment;
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
    std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> variables;
    
    do {
        if (!check(TokenType::TOK_IDENTIFIER)) {
            error("Expected variable name");
        }
        
        std::string var_name = advance().value;
        std::unique_ptr<ASTNode> init_expr = nullptr;
        
        if (match(TokenType::TOK_ASSIGN)) {
            init_expr = std::unique_ptr<ASTNode>(parseExpression());
        }
        
        variables.emplace_back(var_name, std::move(init_expr));
        
    } while (match(TokenType::TOK_COMMA));
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';'");
    
    // 単一変数の場合は従来通りAST_VAR_DECL、複数の場合はAST_MULTIPLE_VAR_DECL
    if (variables.size() == 1) {
        ASTNode* node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        node->name = variables[0].first;
        node->type_name = var_type;
        
        // Set type_info based on type string - typedef aliasは後でインタープリターで解決
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
            // typedef aliasの可能性があるのでUNKNOWNに設定（インタープリターで解決）
            node->type_info = TYPE_UNKNOWN;
        }
        
        if (variables[0].second) {
            node->init_expr = std::move(variables[0].second);
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
            var_node->name = var.first;
            var_node->type_name = var_type;
            var_node->type_info = node->type_info;
            
            if (var.second) {
                var_node->init_expr = std::move(var.second);
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
    } else if (check(TokenType::TOK_IDENTIFIER)) {
        // typedef aliasの可能性
        base_type = advance().value; // とりあえずそのまま返す（インタープリターで解決）
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
    ASTNode* left = parseLogicalOr();
    
    if (check(TokenType::TOK_ASSIGN)) {
        advance(); // consume '='
        ASTNode* right = parseAssignment(); // Right associative
        
        ASTNode* assign = new ASTNode(ASTNodeType::AST_ASSIGN);
        
        // leftが変数か配列参照でない場合はエラー
        if (left->node_type == ASTNodeType::AST_VARIABLE) {
            assign->name = left->name;
            delete left; // leftノードはもう不要
        } else if (left->node_type == ASTNodeType::AST_ARRAY_REF) {
            assign->left = std::unique_ptr<ASTNode>(left);
        } else {
            error("Invalid assignment target");
            return nullptr;
        }
        
        assign->right = std::unique_ptr<ASTNode>(right);
        return assign;
    }
    
    return left;
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
    ASTNode* left = parseComparison();
    
    while (check(TokenType::TOK_AND)) {
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
    ASTNode* left = parseAdditive();
    
    while (check(TokenType::TOK_EQ) || check(TokenType::TOK_NE) || 
           check(TokenType::TOK_LT) || check(TokenType::TOK_LE) ||
           check(TokenType::TOK_GT) || check(TokenType::TOK_GE)) {
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
    // Prefix operators: !, -, ++, --
    if (check(TokenType::TOK_NOT) || check(TokenType::TOK_MINUS) || 
        check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
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
    
    // 配列アクセスチェーン: arr[i][j][k]...
    while (check(TokenType::TOK_LBRACKET)) {
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
            
            // 型情報を設定
            if (param_type.find("[") != std::string::npos) {
                // 配列パラメータの場合
                std::string base_type = param_type.substr(0, param_type.find("["));
                if (base_type == "int") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
                } else if (base_type == "string") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
                } else if (base_type == "bool") {
                    param->type_info = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL);
                } else {
                    param->type_info = TYPE_UNKNOWN;
                }
            } else if (param_type == "int") {
                param->type_info = TYPE_INT;
            } else if (param_type == "long") {
                param->type_info = TYPE_LONG;
            } else if (param_type == "short") {
                param->type_info = TYPE_SHORT;
            } else if (param_type == "tiny") {
                param->type_info = TYPE_TINY;
            } else if (param_type == "bool") {
                param->type_info = TYPE_BOOL;
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
    } else {
        function_node->return_types.push_back(TYPE_UNKNOWN);
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
    // typedef <type> <alias>;
    consume(TokenType::TOK_TYPEDEF, "Expected 'typedef'");
    
    // 基底型を解析
    TypeInfo base_type = TYPE_UNKNOWN;
    if (check(TokenType::TOK_INT)) {
        base_type = TYPE_INT;
        advance();
    } else if (check(TokenType::TOK_LONG)) {
        base_type = TYPE_LONG;
        advance();
    } else if (check(TokenType::TOK_SHORT)) {
        base_type = TYPE_SHORT;
        advance();
    } else if (check(TokenType::TOK_TINY)) {
        base_type = TYPE_TINY;
        advance();
    } else if (check(TokenType::TOK_BOOL)) {
        base_type = TYPE_BOOL;
        advance();
    } else if (check(TokenType::TOK_STRING_TYPE)) {
        base_type = TYPE_STRING;
        advance();
    } else if (check(TokenType::TOK_CHAR_TYPE)) {
        base_type = TYPE_CHAR;
        advance();
    } else if (check(TokenType::TOK_VOID)) {
        base_type = TYPE_VOID;
        advance();
    } else {
        error("Expected type after typedef");
        return nullptr;
    }
    
    // typedef ASTノードを作成
    ASTNode* typedef_node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
    typedef_node->type_info = base_type;
    
    // 基底型名を取得
    std::string base_type_name;
    switch (base_type) {
        case TYPE_INT: base_type_name = "int"; break;
        case TYPE_LONG: base_type_name = "long"; break;
        case TYPE_SHORT: base_type_name = "short"; break;
        case TYPE_TINY: base_type_name = "tiny"; break;
        case TYPE_BOOL: base_type_name = "bool"; break;
        case TYPE_STRING: base_type_name = "string"; break;
        case TYPE_CHAR: base_type_name = "char"; break;
        case TYPE_VOID: base_type_name = "void"; break;
        default: base_type_name = "unknown"; break;
    }
    
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
