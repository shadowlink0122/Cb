#include "recursive_parser.h"
#include "parsers/expression_parser.h"
#include "parsers/statement_parser.h"
#include "parsers/declaration_parser.h"
#include "parsers/type_parser.h"
#include "parsers/struct_parser.h"
#include "../../backend/interpreter/core/error_handler.h"
#include "../../common/debug.h"
#include "../../common/debug_messages.h"
#include "../../common/debug.h"

#include <iostream>
#include <sstream>
#include <unordered_set>
#include <algorithm>
#include <cctype>

// 外部関数宣言
extern const char *type_info_to_string(TypeInfo type);

using namespace RecursiveParserNS;

RecursiveParser::RecursiveParser(const std::string& source, const std::string& filename) 
    : lexer_(source), current_token_(TokenType::TOK_EOF, "", 0, 0), filename_(filename), source_(source), debug_mode_(false) {
    // ソースコードを行ごとに分割
    std::istringstream iss(source);
    std::string line;
    while (std::getline(iss, line)) {
        source_lines_.push_back(line);
    }
    
    // 分離されたパーサーのインスタンスを初期化
    // Phase 2: 全パーサーの有効化（委譲パターン）
    expression_parser_ = std::make_unique<ExpressionParser>(this);
    statement_parser_ = std::make_unique<StatementParser>(this);
    declaration_parser_ = std::make_unique<DeclarationParser>(this);
    type_parser_ = std::make_unique<TypeParser>(this);
    struct_parser_ = std::make_unique<StructParser>(this);
    
    advance();
}

// デストラクタ - unique_ptrの不完全型対応のため、実装ファイルで定義
RecursiveParser::~RecursiveParser() = default;

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
    debug_msg(DebugMsgId::PARSE_PROGRAM_START, filename_.c_str());
    ASTNode* program = new ASTNode(ASTNodeType::AST_STMT_LIST);
    
    while (!isAtEnd()) {
        debug_msg(DebugMsgId::PARSE_STATEMENT_START, current_token_.line, current_token_.column);
        ASTNode* stmt = parseStatement();
        if (stmt != nullptr) {
            debug_msg(DebugMsgId::PARSE_STATEMENT_SUCCESS, 
                     std::to_string((int)stmt->node_type).c_str(), stmt->name.c_str());
            program->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }
    
    debug_msg(DebugMsgId::PARSE_PROGRAM_COMPLETE, program->statements.size());
    return program;
}

ASTNode* RecursiveParser::parseStatement() {
    // static修飾子のチェック
    bool isStatic = false;
    if (check(TokenType::TOK_STATIC)) {
        debug_msg(DebugMsgId::PARSE_STATIC_MODIFIER, current_token_.line, current_token_.column);
        isStatic = true;
        advance(); // consume 'static'
    }
    
    // const修飾子のチェック
    bool isConst = false;
    if (check(TokenType::TOK_CONST)) {
        debug_msg(DebugMsgId::PARSE_CONST_MODIFIER, current_token_.line, current_token_.column);
        isConst = true;
        advance(); // consume 'const'
    }
    
    // DEBUG: 現在のトークンを出力
    std::string token_type_str = std::to_string(static_cast<int>(current_token_.type));
    debug_msg(DebugMsgId::PARSE_CURRENT_TOKEN, current_token_.value.c_str(), token_type_str.c_str());
    
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
        debug_msg(DebugMsgId::PARSE_TYPEDEF_START, current_token_.line);
        return parseTypedefDeclaration();
    }
    
    // struct宣言の処理
    if (check(TokenType::TOK_STRUCT)) {
        debug_msg(DebugMsgId::PARSE_STRUCT_DECL_START, current_token_.line);
        return parseStructDeclaration();
    }
    
    // enum宣言の処理
    if (check(TokenType::TOK_ENUM)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START, current_token_.line);
        return parseEnumDeclaration();
    }
    
    // interface宣言の処理
    if (check(TokenType::TOK_INTERFACE)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START, current_token_.line); // 適切なデバッグIDに変更する必要がある
        return parseInterfaceDeclaration();
    }
    
    // impl宣言の処理
    if (check(TokenType::TOK_IMPL)) {
        debug_msg(DebugMsgId::PARSE_ENUM_DECL_START, current_token_.line); // 適切なデバッグIDに変更する必要がある
        return parseImplDeclaration();
    }
    
    // typedef型変数宣言の処理
    if (check(TokenType::TOK_IDENTIFIER)) {
        std::string potential_type = current_token_.value;
        
        // 構造体配列戻り値関数の早期検出: Type[size] function_name()
        bool is_array_return_function = false;
        if ((typedef_map_.find(potential_type) != typedef_map_.end() || 
             struct_definitions_.find(potential_type) != struct_definitions_.end())) {
            
            // Type[...] function_name() のパターンをチェック
            RecursiveLexer temp_lexer = lexer_;
            Token temp_current = current_token_;
            
            advance(); // Type
            if (check(TokenType::TOK_LBRACKET)) {
                advance(); // [
                // 配列サイズをスキップ
                int bracket_count = 1;
                while (bracket_count > 0 && !isAtEnd()) {
                    if (check(TokenType::TOK_LBRACKET)) bracket_count++;
                    else if (check(TokenType::TOK_RBRACKET)) bracket_count--;
                    advance();
                }
                // ]の後に識別子、その後に(があれば配列戻り値関数
                if (check(TokenType::TOK_IDENTIFIER)) {
                    advance(); // function_name
                    if (check(TokenType::TOK_LPAREN)) {
                        is_array_return_function = true;
                    }
                }
            }
            
            // 元の位置に戻す
            lexer_ = temp_lexer;
            current_token_ = temp_current;
        }
        
        if (is_array_return_function) {
            // 配列戻り値関数として処理
            std::string return_type = advance().value; // 型名
            
            // 配列部分を戻り値型に追加: Type[size]
            return_type += advance().value; // '['
            while (!check(TokenType::TOK_RBRACKET) && !isAtEnd()) {
                return_type += advance().value; // 配列サイズ
            }
            if (check(TokenType::TOK_RBRACKET)) {
                return_type += advance().value; // ']'
            }
            
            std::string function_name = advance().value; // 関数名
            debug_msg(DebugMsgId::PARSE_FUNCTION_DECL_FOUND, function_name.c_str(), return_type.c_str());
            return parseFunctionDeclarationAfterName(return_type, function_name);
        }
        
        // typedef型または構造体型の可能性をチェック
        bool is_typedef = typedef_map_.find(potential_type) != typedef_map_.end();
        bool is_struct_type = struct_definitions_.find(potential_type) != struct_definitions_.end();
        bool is_interface_type = interface_definitions_.find(potential_type) != interface_definitions_.end();
        
        debug_msg(DebugMsgId::PARSE_TYPE_CHECK, potential_type.c_str(), 
                  is_typedef ? "true" : "false", is_struct_type ? "true" : "false");

        if (is_typedef || is_struct_type || is_interface_type) {
            debug_msg(DebugMsgId::PARSE_TYPEDEF_OR_STRUCT_TYPE_FOUND, potential_type.c_str());
            // 簡単な先読み: 現在の位置を保存
            RecursiveLexer temp_lexer = lexer_;
            Token temp_current = current_token_;
            
            advance(); // 型名をスキップ
            
            bool is_function = false;
            if (check(TokenType::TOK_IDENTIFIER)) {
                debug_msg(DebugMsgId::PARSE_IDENTIFIER_AFTER_TYPE, current_token_.value.c_str());
                advance(); // 識別子をスキップ
                if (check(TokenType::TOK_LPAREN)) {
                    debug_msg(DebugMsgId::PARSE_FUNCTION_DETECTED, "");
                    is_function = true;
                } else if (check(TokenType::TOK_LBRACKET)) {
                    // 配列型かもしれないが、配列戻り値の関数の可能性もあるので更にチェック
                    debug_msg(DebugMsgId::PARSE_ARRAY_DETECTED, "");
                    
                    // 配列の括弧をスキップ: [2]
                    advance(); // consume '['
                    while (!check(TokenType::TOK_RBRACKET) && !isAtEnd()) {
                        advance(); // 配列サイズの式をスキップ
                    }
                    if (check(TokenType::TOK_RBRACKET)) {
                        advance(); // consume ']'
                    }
                    
                    // 次が識別子かつその後に'('があれば関数（配列戻り値）
                    if (check(TokenType::TOK_IDENTIFIER)) {
                        advance(); // 関数名をスキップ
                        if (check(TokenType::TOK_LPAREN)) {
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
            lexer_ = temp_lexer;
            current_token_ = temp_current;
            
            if (is_function) {
                // これは戻り値型が構造体またはtypedefの関数宣言（配列戻り値の可能性もある）
                std::string return_type = advance().value; // 型名を取得
                
                // 配列戻り値の場合: Person[2] function_name() の処理
                if (check(TokenType::TOK_LBRACKET)) {
                    // 配列部分を戻り値型に追加
                    return_type += advance().value; // '['
                    while (!check(TokenType::TOK_RBRACKET) && !isAtEnd()) {
                        return_type += advance().value; // 配列サイズ
                    }
                    if (check(TokenType::TOK_RBRACKET)) {
                        return_type += advance().value; // ']'
                    }
                }
                
                std::string function_name = advance().value; // 関数名を取得
                debug_msg(DebugMsgId::PARSE_FUNCTION_DECL_FOUND, function_name.c_str(), return_type.c_str());
                return parseFunctionDeclarationAfterName(return_type, function_name);
            } else {
                // これは構造体またはtypedef型変数宣言
                if (is_struct_type) {
                    debug_msg(DebugMsgId::PARSE_STRUCT_VAR_DECL_FOUND, potential_type.c_str());
                    // 構造体変数宣言
                    std::string struct_type = advance().value;
                    
                    // ポインタまたは参照チェック: Point* pp; または Point& rp;
                    bool is_pointer = false;
                    bool is_reference = false;
                    int pointer_depth = 0;
                    
                    while (check(TokenType::TOK_MUL)) {
                        is_pointer = true;
                        pointer_depth++;
                        advance();
                    }
                    
                    if (check(TokenType::TOK_BIT_AND)) {
                        is_reference = true;
                        advance();
                    }
                    
                    // ポインタまたは参照の場合
                    if (is_pointer || is_reference) {
                        if (!check(TokenType::TOK_IDENTIFIER)) {
                            error("Expected variable name after pointer/reference type");
                            return nullptr;
                        }
                        
                        std::string var_name = advance().value;
                        
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
                        if (match(TokenType::TOK_ASSIGN)) {
                            var_node->init_expr = std::unique_ptr<ASTNode>(parseExpression());
                        }
                        
                        consume(TokenType::TOK_SEMICOLON, "Expected ';' after pointer/reference variable declaration");
                        return var_node;
                    }
                    
                    // 配列チェック: Person[3] people; または Person people;
                    if (check(TokenType::TOK_LBRACKET)) {
                        // struct配列宣言: Person[3] people;
                        debug_msg(DebugMsgId::PARSE_STRUCT_ARRAY_DECL, struct_type.c_str());
                        advance(); // consume '['
                        ASTNode* size_expr = parseExpression();
                        consume(TokenType::TOK_RBRACKET, "Expected ']' after array size");
                        
                        std::string var_name = advance().value; // 変数名を取得
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
                        
                        debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(), struct_type.c_str());
                        
                        ASTNode* var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
                        var_node->name = var_name;
                        var_node->type_name = struct_type;
                        var_node->type_info = TYPE_STRUCT;
                        var_node->is_const = isConst;
                        
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
                } else if (is_interface_type) {
                    debug_msg(DebugMsgId::PARSE_STRUCT_VAR_DECL_FOUND, potential_type.c_str()); // interface専用のデバッグIDが必要
                    // interface変数宣言
                    std::string interface_type = advance().value;
                    
                    // ポインタ深度をチェック
                    int pointer_depth = 0;
                    while (check(TokenType::TOK_MUL)) {
                        pointer_depth++;
                        advance();
                    }
                    
                    // 参照チェック
                    bool is_reference = false;
                    if (check(TokenType::TOK_BIT_AND)) {
                        is_reference = true;
                        advance();
                    }
                    
                    if (!check(TokenType::TOK_IDENTIFIER)) {
                        error("Expected interface variable name");
                        return nullptr;
                    }
                    
                    std::string var_name = advance().value;
                    
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
                    if (match(TokenType::TOK_ASSIGN)) {
                        var_node->init_expr = std::unique_ptr<ASTNode>(parseExpression());
                    }
                    
                    consume(TokenType::TOK_SEMICOLON, "Expected ';' after interface variable declaration");
                    
                    return var_node;
                } else {
                    // typedef型変数宣言
                    return parseTypedefVariableDeclaration();
                }
            }
        }
    }

    bool saw_unsigned_specifier = false;
    if (check(TokenType::TOK_UNSIGNED)) {
        saw_unsigned_specifier = true;
        advance();
    }

    // 関数定義の解析 (int main() など)
    if (check(TokenType::TOK_INT) || check(TokenType::TOK_LONG) || 
        check(TokenType::TOK_SHORT) || check(TokenType::TOK_TINY) || 
        check(TokenType::TOK_VOID) || check(TokenType::TOK_BOOL) ||
        check(TokenType::TOK_STRING_TYPE) || check(TokenType::TOK_CHAR_TYPE) ||
        check(TokenType::TOK_FLOAT) || check(TokenType::TOK_DOUBLE) ||
        check(TokenType::TOK_BIG) || check(TokenType::TOK_QUAD)) {
        
        // 型名を取得
        std::string base_type_name;
        if (check(TokenType::TOK_INT)) base_type_name = "int";
        else if (check(TokenType::TOK_LONG)) base_type_name = "long";
        else if (check(TokenType::TOK_SHORT)) base_type_name = "short";
        else if (check(TokenType::TOK_TINY)) base_type_name = "tiny";
        else if (check(TokenType::TOK_VOID)) base_type_name = "void";
        else if (check(TokenType::TOK_BOOL)) base_type_name = "bool";
        else if (check(TokenType::TOK_STRING_TYPE)) base_type_name = "string";
        else if (check(TokenType::TOK_CHAR_TYPE)) base_type_name = "char";
        else if (check(TokenType::TOK_FLOAT)) base_type_name = "float";
        else if (check(TokenType::TOK_DOUBLE)) base_type_name = "double";
        else if (check(TokenType::TOK_BIG)) base_type_name = "big";
        else if (check(TokenType::TOK_QUAD)) base_type_name = "quad";

        advance(); // consume type

        // ポインタ修飾子のチェック
        int pointer_depth = 0;
        while (check(TokenType::TOK_MUL)) {
            pointer_depth++;
            advance();
        }

        // 参照修飾子のチェック
        bool is_reference = false;
        if (check(TokenType::TOK_BIT_AND)) {
            is_reference = true;
            advance();
        }

        TypeInfo base_type_info = getTypeInfoFromString(base_type_name);
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
                    error("'unsigned' modifier can only be applied to numeric types");
                    return nullptr;
            }
            type_name = "unsigned " + base_type_name;
        }

        // 参照型の場合、型情報は基底型から取得
        std::string type_for_info = type_name;
        if (is_reference && type_for_info.back() == '&') {
            type_for_info.pop_back();  // '&'を削除
        }
        
        TypeInfo declared_type_info = getTypeInfoFromString(type_for_info);
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
            error("Expected identifier after type");
            return nullptr;
        }
    } else if (saw_unsigned_specifier) {
        error("Expected type specifier after 'unsigned'");
        return nullptr;
    }
    
    // return文の処理
    if (check(TokenType::TOK_RETURN)) {
        return parseReturnStatement();
    }
    
    // assert文の処理
    if (check(TokenType::TOK_ASSERT)) {
        return parseAssertStatement();
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
                
                std::string member_name;
                if (check(TokenType::TOK_IDENTIFIER)) {
                    member_name = advance().value;
                } else if (check(TokenType::TOK_PRINT) || check(TokenType::TOK_PRINTLN) || check(TokenType::TOK_PRINTF)) {
                    member_name = advance().value;
                } else {
                    error("Expected member name after '.'");
                    return nullptr;
                }
                
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
            } else if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
                // 配列要素のポストインクリメント/デクリメント: arr[i]++; arr[i]--;
                Token op = advance();
                ASTNode* postfix = new ASTNode(ASTNodeType::AST_POST_INCDEC);
                postfix->op = op.value;
                postfix->left = std::unique_ptr<ASTNode>(left_expr);
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return postfix;
            } else {
                error("Expected assignment operator after array access");
                return nullptr;
            }
        } else if (check(TokenType::TOK_DOT)) {
            // メンバアクセス代入の処理: obj.member = value または obj.member[index] = value
            advance(); // consume '.'
            
            std::string member_name;
            if (check(TokenType::TOK_IDENTIFIER)) {
                member_name = advance().value;
            } else if (check(TokenType::TOK_PRINT) || check(TokenType::TOK_PRINTLN) || check(TokenType::TOK_PRINTF)) {
                member_name = advance().value;
            } else {
                error("Expected member name after '.'");
                return nullptr;
            }
            
            // 最初のメンバアクセスノードを作成
            ASTNode* member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
            member_access->name = member_name;
            
            ASTNode* obj_var = new ASTNode(ASTNodeType::AST_VARIABLE);
            obj_var->name = name;
            member_access->left = std::unique_ptr<ASTNode>(obj_var);
            
            // ネストしたメンバーアクセスの処理 (obj.member.submember.x = value)
            while (check(TokenType::TOK_DOT)) {
                advance(); // consume '.'
                
                std::string nested_member;
                if (check(TokenType::TOK_IDENTIFIER)) {
                    nested_member = advance().value;
                } else if (check(TokenType::TOK_PRINT) || check(TokenType::TOK_PRINTLN) || check(TokenType::TOK_PRINTF)) {
                    nested_member = advance().value;
                } else {
                    error("Expected member name after '.'");
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
            if (check(TokenType::TOK_LBRACKET)) {
                std::vector<std::unique_ptr<ASTNode>> indices;
                
                // 多次元配列のインデックスを順次処理
                while (check(TokenType::TOK_LBRACKET)) {
                    advance(); // consume '['
                    
                    ASTNode* index_expr = parseExpression();
                    indices.push_back(std::unique_ptr<ASTNode>(index_expr));
                    
                    if (!check(TokenType::TOK_RBRACKET)) {
                        error("Expected ']' after array index");
                        delete member_access;
                        return nullptr;
                    }
                    advance(); // consume ']'
                }
                
                // 配列アクセス後にさらにメンバアクセスがあるかチェック: obj.array[idx].member
                // またはさらに深いネスト: obj.array[idx].member.submember
                while (check(TokenType::TOK_DOT)) {
                    advance(); // consume '.'
                    
                    if (!check(TokenType::TOK_IDENTIFIER)) {
                        error("Expected member name after '.' in nested struct array access");
                        delete member_access;
                        return nullptr;
                    }
                    
                    std::string nested_member_name = current_token_.value;
                    advance();
                    
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
                if (check(TokenType::TOK_ASSIGN)) {
                    advance(); // consume '='
                    
                    ASTNode* value_expr = parseExpression();
                    
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
                        
                        consume(TokenType::TOK_SEMICOLON, "Expected ';' after assignment");
                        return assignment;
                    } else {
                        // 通常のメンバアクセス代入
                        ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                        assignment->left = std::unique_ptr<ASTNode>(member_access);
                        assignment->right = std::unique_ptr<ASTNode>(value_expr);
                        
                        consume(TokenType::TOK_SEMICOLON, "Expected ';' after assignment");
                        return assignment;
                    }
                } else {
                    error("Expected '=' after member array access");
                    delete member_access;
                    return nullptr;
                }
            }
            // 通常のメンバアクセス代入処理（複合代入演算子対応）
            else if (check(TokenType::TOK_ASSIGN) || check(TokenType::TOK_PLUS_ASSIGN) || 
                     check(TokenType::TOK_MINUS_ASSIGN) || check(TokenType::TOK_MUL_ASSIGN) ||
                     check(TokenType::TOK_DIV_ASSIGN) || check(TokenType::TOK_MOD_ASSIGN) ||
                     check(TokenType::TOK_AND_ASSIGN) || check(TokenType::TOK_OR_ASSIGN) ||
                     check(TokenType::TOK_XOR_ASSIGN) || check(TokenType::TOK_LSHIFT_ASSIGN) ||
                     check(TokenType::TOK_RSHIFT_ASSIGN)) {
                
                TokenType op_type = current_token_.type;
                advance(); // consume assignment operator
                
                ASTNode* value_expr = parseExpression();
                
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
                    }
                    
                    // 左辺のコピーを作成（ネストメンバアクセスの深いコピーが必要）
                    ASTNode* left_copy = cloneAstNode(member_access);
                    
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
                
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return assignment;
            } else if (check(TokenType::TOK_LPAREN)) {
                // メソッド呼び出し: obj.method()
                advance(); // consume '('
                
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
                while (!check(TokenType::TOK_RPAREN) && !isAtEnd()) {
                    auto arg = parseExpression();
                    if (arg) {
                        method_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                    }
                    
                    if (!check(TokenType::TOK_RPAREN)) {
                        consume(TokenType::TOK_COMMA, "Expected ',' between arguments");
                    }
                }
                
                consume(TokenType::TOK_RPAREN, "Expected ')' after method arguments");
                consume(TokenType::TOK_SEMICOLON, "Expected ';' after method call");
                
                return method_call;
            } else if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
                // メンバーへのポストインクリメント/デクリメント: obj.member++ or obj.member--
                TokenType op_type = current_token_.type;
                advance(); // consume '++' or '--'
                
                consume(TokenType::TOK_SEMICOLON, "Expected ';' after increment/decrement");
                
                // AST_POST_INCDECノードを作成
                ASTNode* incdec = new ASTNode(ASTNodeType::AST_POST_INCDEC);
                incdec->op = (op_type == TokenType::TOK_INCR) ? "++" : "--";
                
                // 既に作成されたネストメンバアクセスを子として設定
                incdec->left = std::unique_ptr<ASTNode>(member_access);
                
                return incdec;
            } else {
                error("Expected '=', '(', '++', or '--' after member access");
                delete member_access;
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
            }
            // メンバアクセスやアロー演算子のチェック
            else if (check(TokenType::TOK_DOT) || check(TokenType::TOK_ARROW) || check(TokenType::TOK_LBRACKET)) {
                // 識別子から始まる複雑な式（obj.member, ptr->member, arr[i]等）
                // レキサーを巻き戻して、完全な式として解析し直す
                // 現在の位置を保存
                RecursiveLexer saved_lexer = lexer_;
                Token saved_token = current_token_;
                
                // 識別子を含む完全な式を解析するため、識別子の位置に戻る必要がある
                // しかし、既に識別子を消費してしまっているので、
                // 代わりに識別子ノードからpostfix処理を継続する
                
                // identifier_nodeから始めて、postfix操作（.、->、[]）を処理
                ASTNode* expr_node = identifier_node;
                identifier_node = nullptr; // ownershipを移動
                
                // postfix操作を処理
                while (true) {
                    if (check(TokenType::TOK_LBRACKET)) {
                        // 配列アクセス
                        advance(); // consume '['
                        ASTNode* index = parseExpression();
                        consume(TokenType::TOK_RBRACKET, "Expected ']'");
                        
                        ASTNode* array_ref = new ASTNode(ASTNodeType::AST_ARRAY_REF);
                        array_ref->left = std::unique_ptr<ASTNode>(expr_node);
                        array_ref->array_index = std::unique_ptr<ASTNode>(index);
                        expr_node = array_ref;
                    } else if (check(TokenType::TOK_DOT)) {
                        // メンバアクセス
                        expr_node = parseMemberAccess(expr_node);
                    } else if (check(TokenType::TOK_ARROW)) {
                        // アロー演算子
                        expr_node = parseArrowAccess(expr_node);
                    } else {
                        break;
                    }
                }
                
                // 後置インクリメント/デクリメントのチェック
                if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
                    TokenType op_type = current_token_.type;
                    advance(); // consume '++' or '--'
                    
                    ASTNode* postfix_node = new ASTNode(ASTNodeType::AST_POST_INCDEC);
                    postfix_node->op = (op_type == TokenType::TOK_INCR) ? "++" : "--";
                    postfix_node->left = std::unique_ptr<ASTNode>(expr_node);
                    
                    consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                    return postfix_node;
                }
                // 代入があるかチェック
                else if (check(TokenType::TOK_ASSIGN) || check(TokenType::TOK_PLUS_ASSIGN) || 
                    check(TokenType::TOK_MINUS_ASSIGN) || check(TokenType::TOK_MUL_ASSIGN) ||
                    check(TokenType::TOK_DIV_ASSIGN) || check(TokenType::TOK_MOD_ASSIGN)) {
                    TokenType op_type = current_token_.type;
                    advance(); // consume assignment operator
                    
                    ASTNode* right = parseExpression();
                    
                    ASTNode* assignment = new ASTNode(ASTNodeType::AST_ASSIGN);
                    assignment->left = std::unique_ptr<ASTNode>(expr_node);
                    assignment->right = std::unique_ptr<ASTNode>(right);
                    
                    consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                    return assignment;
                }
                
                consume(TokenType::TOK_SEMICOLON, "Expected ';'");
                return expr_node;
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
    ParsedTypeInfo base_parsed_type = getLastParsedTypeInfo();
    var_type = base_parsed_type.full_type;
    
    // 変数名のリストを収集
                struct VariableInfo {
                        std::string name;
                        std::unique_ptr<ASTNode> init_expr;
                        ArrayTypeInfo array_info;
                        bool is_array;
                        ParsedTypeInfo parsed_type;
                        bool is_private;

                        VariableInfo(const std::string& n,
                                                 std::unique_ptr<ASTNode> expr,
                                                 const ArrayTypeInfo& arr_info,
                                                 bool arr,
                                                 const ParsedTypeInfo& parsed,
                                                 bool is_priv)
                                : name(n), init_expr(std::move(expr)), array_info(arr_info),
                                    is_array(arr), parsed_type(parsed), is_private(is_priv) {}
    };
    std::vector<VariableInfo> variables;

    auto dimension_to_string = [](const ArrayDimension& dim) {
        if (!dim.size_expr.empty()) {
            return std::string("[") + dim.size_expr + "]";
        }
        if (!dim.is_dynamic && dim.size >= 0) {
            return std::string("[") + std::to_string(dim.size) + "]";
        }
        return std::string("[]");
    };

    do {
        if (!check(TokenType::TOK_IDENTIFIER)) {
            error("Expected variable name");
        }
        
        std::string var_name = advance().value;
        std::unique_ptr<ASTNode> init_expr = nullptr;
    ParsedTypeInfo var_parsed = base_parsed_type;
    ArrayTypeInfo array_info = var_parsed.array_info;
    bool is_array = var_parsed.is_array;
        
        // 配列の角括弧をチェック
        if (check(TokenType::TOK_LBRACKET)) {
            is_array = true;
            if (array_info.base_type == TYPE_UNKNOWN) {
                array_info.base_type = var_parsed.is_pointer ? TYPE_POINTER : var_parsed.base_type_info;
            }

            while (check(TokenType::TOK_LBRACKET)) {
                advance();

                if (check(TokenType::TOK_NUMBER)) {
                    int size = std::stoi(advance().value);
                    array_info.dimensions.emplace_back(size, false, "");
                } else if (check(TokenType::TOK_IDENTIFIER)) {
                    std::string size_name = advance().value;
                    array_info.dimensions.emplace_back(-1, true, size_name);
                } else {
                    array_info.dimensions.emplace_back(-1, true, "");
                }

                consume(TokenType::TOK_RBRACKET, "Expected ']'");
            }
        }

        if (is_array && array_info.base_type == TYPE_UNKNOWN) {
            array_info.base_type = var_parsed.is_pointer ? TYPE_POINTER : var_parsed.base_type_info;
        }

        var_parsed.is_array = is_array;
        var_parsed.array_info = array_info;

        std::string combined_full_type = base_parsed_type.full_type;
        if (is_array) {
            size_t base_dims = base_parsed_type.array_info.dimensions.size();
            size_t total_dims = array_info.dimensions.size();
            if (total_dims > base_dims) {
                for (size_t idx = base_dims; idx < total_dims; ++idx) {
                    combined_full_type += dimension_to_string(array_info.dimensions[idx]);
                }
            }
        }
        var_parsed.full_type = combined_full_type;
        
        if (match(TokenType::TOK_ASSIGN)) {
            init_expr = std::unique_ptr<ASTNode>(parseExpression());
        }
        
    variables.emplace_back(var_name, std::move(init_expr), array_info, is_array, var_parsed, false);
        
    } while (match(TokenType::TOK_COMMA));
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';'");
    
    // 単一変数の場合は従来通りAST_VAR_DECL、複数の場合はAST_MULTIPLE_VAR_DECL
    if (variables.size() == 1) {
    VariableInfo& var_info = variables[0];
    const ParsedTypeInfo& parsed = var_info.parsed_type;

        ASTNode* node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        node->name = var_info.name;
        node->type_name = parsed.full_type;

    TypeInfo declared_type = resolveParsedTypeInfo(parsed);
        node->type_info = declared_type;

        node->is_pointer = parsed.is_pointer;
        node->pointer_depth = parsed.pointer_depth;
        node->pointer_base_type_name = parsed.base_type;
        node->pointer_base_type = parsed.base_type_info;
        node->is_reference = parsed.is_reference;
        node->is_unsigned = parsed.is_unsigned;

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
        ASTNode* node = new ASTNode(ASTNodeType::AST_MULTIPLE_VAR_DECL);
        node->type_name = base_parsed_type.full_type;
    node->type_info = resolveParsedTypeInfo(base_parsed_type);
        node->is_pointer = base_parsed_type.is_pointer;
        node->pointer_depth = base_parsed_type.pointer_depth;
        node->pointer_base_type_name = base_parsed_type.base_type;
        node->pointer_base_type = base_parsed_type.base_type_info;
        node->is_reference = base_parsed_type.is_reference;
        node->is_unsigned = base_parsed_type.is_unsigned;
        if (base_parsed_type.is_array) {
            node->array_type_info = base_parsed_type.array_info;
            node->is_array = true;
        }
        
        // 各変数を子ノードとして追加
        for (auto& var : variables) {
            ParsedTypeInfo& parsed = var.parsed_type;
            ASTNode* var_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
            var_node->name = var.name;
            var_node->type_name = parsed.full_type;
            var_node->type_info = resolveParsedTypeInfo(parsed);
            var_node->is_pointer = parsed.is_pointer;
            var_node->pointer_depth = parsed.pointer_depth;
            var_node->pointer_base_type_name = parsed.base_type;
            var_node->pointer_base_type = parsed.base_type_info;
            var_node->is_reference = parsed.is_reference;
            var_node->is_unsigned = parsed.is_unsigned;

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

std::string RecursiveParser::parseType() {
    ParsedTypeInfo parsed;
    parsed.array_info = ArrayTypeInfo();

    std::string base_type;
    std::string original_type;
    bool saw_unsigned = false;
    bool saw_const = false;

    // Check for const qualifier
    if (check(TokenType::TOK_CONST)) {
        saw_const = true;
        advance();
    }

    if (check(TokenType::TOK_UNSIGNED)) {
        saw_unsigned = true;
        advance();
    }

    auto set_base_type = [&](const std::string &type_name) {
        base_type = type_name;
        if (original_type.empty()) {
            original_type = type_name;
        }
    };

    if (check(TokenType::TOK_INT)) {
        advance();
        set_base_type("int");
    } else if (check(TokenType::TOK_LONG)) {
        advance();
        set_base_type("long");
    } else if (check(TokenType::TOK_SHORT)) {
        advance();
        set_base_type("short");
    } else if (check(TokenType::TOK_TINY)) {
        advance();
        set_base_type("tiny");
    } else if (check(TokenType::TOK_VOID)) {
        advance();
        set_base_type("void");
    } else if (check(TokenType::TOK_BOOL)) {
        advance();
        set_base_type("bool");
    } else if (check(TokenType::TOK_FLOAT)) {
        advance();
        set_base_type("float");
    } else if (check(TokenType::TOK_DOUBLE)) {
        advance();
        set_base_type("double");
    } else if (check(TokenType::TOK_BIG)) {
        advance();
        set_base_type("big");
    } else if (check(TokenType::TOK_QUAD)) {
        advance();
        set_base_type("quad");
    } else if (check(TokenType::TOK_STRING_TYPE)) {
        advance();
        set_base_type("string");
    } else if (check(TokenType::TOK_CHAR_TYPE)) {
        advance();
        set_base_type("char");
    } else if (check(TokenType::TOK_STRUCT)) {
        advance();
        if (!check(TokenType::TOK_IDENTIFIER)) {
            error("Expected struct name after 'struct'");
            return "";
        }
        std::string struct_name = current_token_.value;
        advance();
        original_type = "struct " + struct_name;
        base_type = original_type;
    } else if (check(TokenType::TOK_IDENTIFIER)) {
        std::string identifier = current_token_.value;
        if (typedef_map_.find(identifier) != typedef_map_.end()) {
            advance();
            original_type = identifier;
            std::string resolved = resolveTypedefChain(identifier);
            if (resolved.empty()) {
                error("Unknown type: " + identifier);
                throw std::runtime_error("Unknown type: " + identifier);
            }
            set_base_type(resolved);
        } else if (struct_definitions_.find(identifier) != struct_definitions_.end()) {
            advance();
            original_type = identifier;
            set_base_type(identifier);
        } else if (enum_definitions_.find(identifier) != enum_definitions_.end()) {
            advance();
            original_type = identifier;
            set_base_type(identifier);
        } else if (interface_definitions_.find(identifier) != interface_definitions_.end()) {
            advance();
            original_type = identifier;
            set_base_type(identifier);
        } else if (union_definitions_.find(identifier) != union_definitions_.end()) {
            advance();
            original_type = identifier;
            set_base_type(identifier);
        } else {
            // 未定義型 - 前方参照の可能性として許容
            // (ポインタまたは配列の場合に限る - 後でポインタ/配列チェックで判定)
            advance();
            original_type = identifier;
            set_base_type(identifier);
            // NOTE: 値メンバーとして使用された場合のエラーは後で検出される
        }
    } else {
        error("Expected type specifier");
        return "";
    }

    if (original_type.empty()) {
        original_type = base_type;
    }

    parsed.base_type = base_type;
    parsed.original_type = original_type;
    parsed.base_type_info = getTypeInfoFromString(base_type);

    if (saw_unsigned) {
        switch (parsed.base_type_info) {
        case TYPE_TINY:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
            parsed.is_unsigned = true;
            break;
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
        case TYPE_QUAD:
            // float/double/quadにはunsignedを適用できない
            std::cerr << "[WARNING] 'unsigned' modifier cannot be applied to floating-point types (float, double, quad); 'unsigned' qualifier ignored at line " 
                      << current_token_.line << std::endl;
            break;
        case TYPE_BIG:
            parsed.is_unsigned = true;
            break;
        default:
            error("'unsigned' modifier can only be applied to numeric types");
            return "";
        }
    }

    int pointer_depth = 0;
    while (check(TokenType::TOK_MUL)) {
        pointer_depth++;
        advance();
    }

    if (pointer_depth > 0) {
        parsed.is_pointer = true;
        parsed.pointer_depth = pointer_depth;
    }

    if (check(TokenType::TOK_BIT_AND)) {
        parsed.is_reference = true;
        advance();
    }

    std::vector<ArrayDimension> dimensions;
    std::vector<std::string> dimension_texts;

    while (check(TokenType::TOK_LBRACKET)) {
        advance();

        if (check(TokenType::TOK_NUMBER)) {
            Token size_token = advance();
            dimensions.emplace_back(std::stoi(size_token.value), false, "");
            dimension_texts.push_back("[" + size_token.value + "]");
        } else if (check(TokenType::TOK_IDENTIFIER)) {
            Token const_token = advance();
            dimensions.emplace_back(-1, true, const_token.value);
            dimension_texts.push_back("[" + const_token.value + "]");
        } else {
            dimensions.emplace_back(-1, true, "");
            dimension_texts.push_back("[]");
        }

        consume(TokenType::TOK_RBRACKET, "Expected ']' in array type");
    }

    if (!dimensions.empty()) {
        parsed.is_array = true;
        parsed.array_info.base_type = parsed.is_pointer ? TYPE_POINTER : parsed.base_type_info;
        parsed.array_info.dimensions = dimensions;
    }

    std::string full_type = base_type;
    if (pointer_depth > 0) {
        full_type += std::string(pointer_depth, '*');
    }
    for (const auto &dim_text : dimension_texts) {
        full_type += dim_text;
    }

    if (parsed.is_reference) {
        full_type += "&";
    }

    if (parsed.is_unsigned) {
        full_type = "unsigned " + full_type;
        if (original_type == base_type) {
            parsed.original_type = full_type;
        }
    }

    if (saw_const) {
        parsed.is_const = true;
        full_type = "const " + full_type;
    }

    parsed.full_type = full_type;

    last_parsed_type_info_ = parsed;
    return parsed.full_type;
}

TypeInfo RecursiveParser::resolveParsedTypeInfo(const ParsedTypeInfo& parsed) const {
    TypeInfo resolved = parsed.base_type_info;

    std::string base_lookup = parsed.base_type;
    if (base_lookup.rfind("struct ", 0) == 0) {
        base_lookup = base_lookup.substr(7);
    }

    if (resolved == TYPE_UNKNOWN && !base_lookup.empty()) {
        if (struct_definitions_.find(base_lookup) != struct_definitions_.end()) {
            resolved = TYPE_STRUCT;
        } else if (enum_definitions_.find(base_lookup) != enum_definitions_.end()) {
            resolved = TYPE_ENUM;
        } else if (interface_definitions_.find(base_lookup) != interface_definitions_.end()) {
            resolved = TYPE_INTERFACE;
        } else if (union_definitions_.find(base_lookup) != union_definitions_.end()) {
            resolved = TYPE_UNION;
        }
    }

    if (parsed.is_array) {
        ArrayTypeInfo array_info = parsed.array_info;
        if (array_info.base_type == TYPE_UNKNOWN) {
            array_info.base_type = resolved;
        }
        if (array_info.base_type != TYPE_UNKNOWN) {
            return array_info.to_legacy_type_id();
        }
    }

    if (parsed.is_pointer) {
        return TYPE_POINTER;
    }

    if (resolved != TYPE_UNKNOWN) {
        return resolved;
    }

    return TYPE_UNKNOWN;
}

ASTNode* RecursiveParser::cloneAstNode(const ASTNode* node) {
    if (!node) {
        return nullptr;
    }

    ASTNode* clone = new ASTNode(node->node_type);
    clone->type_info = node->type_info;
    clone->location = node->location;
    clone->is_const = node->is_const;
    clone->is_static = node->is_static;
    clone->is_array = node->is_array;
    clone->is_array_return = node->is_array_return;
    clone->is_private_method = node->is_private_method;
    clone->is_private_member = node->is_private_member;
    clone->is_pointer = node->is_pointer;
    clone->pointer_depth = node->pointer_depth;
    clone->pointer_base_type_name = node->pointer_base_type_name;
    clone->pointer_base_type = node->pointer_base_type;
    clone->is_reference = node->is_reference;
    clone->is_unsigned = node->is_unsigned;
    clone->int_value = node->int_value;
    clone->str_value = node->str_value;
    clone->name = node->name;
    clone->type_name = node->type_name;
    clone->return_type_name = node->return_type_name;
    clone->op = node->op;
    clone->module_name = node->module_name;
    clone->import_items = node->import_items;
    clone->is_exported = node->is_exported;
    clone->qualified_name = node->qualified_name;
    clone->is_qualified_call = node->is_qualified_call;
    clone->enum_name = node->enum_name;
    clone->enum_member = node->enum_member;
    clone->enum_definition = node->enum_definition;
    clone->union_name = node->union_name;
    clone->union_definition = node->union_definition;
    clone->member_chain = node->member_chain;
    clone->array_size = node->array_size;
    clone->array_type_info = node->array_type_info;
    clone->return_types = node->return_types;
    clone->exception_var = node->exception_var;
    clone->exception_type = node->exception_type;

    if (node->left) {
        clone->left.reset(cloneAstNode(node->left.get()));
    }
    if (node->right) {
        clone->right.reset(cloneAstNode(node->right.get()));
    }
    if (node->third) {
        clone->third.reset(cloneAstNode(node->third.get()));
    }
    if (node->condition) {
        clone->condition.reset(cloneAstNode(node->condition.get()));
    }
    if (node->init_expr) {
        clone->init_expr.reset(cloneAstNode(node->init_expr.get()));
    }
    if (node->update_expr) {
        clone->update_expr.reset(cloneAstNode(node->update_expr.get()));
    }
    if (node->body) {
        clone->body.reset(cloneAstNode(node->body.get()));
    }
    if (node->array_index) {
        clone->array_index.reset(cloneAstNode(node->array_index.get()));
    }
    if (node->array_size_expr) {
        clone->array_size_expr.reset(cloneAstNode(node->array_size_expr.get()));
    }
    if (node->try_body) {
        clone->try_body.reset(cloneAstNode(node->try_body.get()));
    }
    if (node->catch_body) {
        clone->catch_body.reset(cloneAstNode(node->catch_body.get()));
    }
    if (node->finally_body) {
        clone->finally_body.reset(cloneAstNode(node->finally_body.get()));
    }
    if (node->throw_expr) {
        clone->throw_expr.reset(cloneAstNode(node->throw_expr.get()));
    }

    clone->children.reserve(node->children.size());
    for (const auto& child : node->children) {
        clone->children.push_back(std::unique_ptr<ASTNode>(cloneAstNode(child.get())));
    }

    clone->parameters.reserve(node->parameters.size());
    for (const auto& param : node->parameters) {
        clone->parameters.push_back(std::unique_ptr<ASTNode>(cloneAstNode(param.get())));
    }

    clone->arguments.reserve(node->arguments.size());
    for (const auto& arg : node->arguments) {
        clone->arguments.push_back(std::unique_ptr<ASTNode>(cloneAstNode(arg.get())));
    }

    clone->statements.reserve(node->statements.size());
    for (const auto& stmt : node->statements) {
        clone->statements.push_back(std::unique_ptr<ASTNode>(cloneAstNode(stmt.get())));
    }

    clone->array_dimensions.reserve(node->array_dimensions.size());
    for (const auto& dim : node->array_dimensions) {
        clone->array_dimensions.push_back(std::unique_ptr<ASTNode>(cloneAstNode(dim.get())));
    }

    clone->array_indices.reserve(node->array_indices.size());
    for (const auto& idx : node->array_indices) {
        clone->array_indices.push_back(std::unique_ptr<ASTNode>(cloneAstNode(idx.get())));
    }

    return clone;
}

ASTNode* RecursiveParser::parseExpression() {
    return parseAssignment();
}

ASTNode* RecursiveParser::parseAssignment() {
    ASTNode* left = parseTernary();
    
    auto getBinaryOpForCompound = [](TokenType op_type) -> std::string {
        switch (op_type) {
            case TokenType::TOK_PLUS_ASSIGN: return "+";
            case TokenType::TOK_MINUS_ASSIGN: return "-";
            case TokenType::TOK_MUL_ASSIGN: return "*";
            case TokenType::TOK_DIV_ASSIGN: return "/";
            case TokenType::TOK_MOD_ASSIGN: return "%";
            case TokenType::TOK_AND_ASSIGN: return "&";
            case TokenType::TOK_OR_ASSIGN: return "|";
            case TokenType::TOK_XOR_ASSIGN: return "^";
            case TokenType::TOK_LSHIFT_ASSIGN: return "<<";
            case TokenType::TOK_RSHIFT_ASSIGN: return ">>";
            default: return "";
        }
    };

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
                std::string binary_op = getBinaryOpForCompound(op_type);
                
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
                std::string binary_op = getBinaryOpForCompound(op_type);
                
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
            if (op_type != TokenType::TOK_ASSIGN) {
                std::string binary_op = getBinaryOpForCompound(op_type);
                ASTNode* left_copy = cloneAstNode(left);
                assign->left = std::unique_ptr<ASTNode>(left);

                ASTNode* binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                binop->op = binary_op;
                binop->left = std::unique_ptr<ASTNode>(left_copy);
                binop->right = std::unique_ptr<ASTNode>(right);

                assign->right = std::unique_ptr<ASTNode>(binop);
            } else {
                assign->left = std::unique_ptr<ASTNode>(left);
                assign->right = std::unique_ptr<ASTNode>(right);
            }
        } else if (left->node_type == ASTNodeType::AST_ARROW_ACCESS) {
            // アロー演算子代入: ptr->member = value
            if (op_type != TokenType::TOK_ASSIGN) {
                std::string binary_op = getBinaryOpForCompound(op_type);
                ASTNode* left_copy = cloneAstNode(left);
                assign->left = std::unique_ptr<ASTNode>(left);

                ASTNode* binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                binop->op = binary_op;
                binop->left = std::unique_ptr<ASTNode>(left_copy);
                binop->right = std::unique_ptr<ASTNode>(right);

                assign->right = std::unique_ptr<ASTNode>(binop);
            } else {
                assign->left = std::unique_ptr<ASTNode>(left);
                assign->right = std::unique_ptr<ASTNode>(right);
            }
        } else if (left->node_type == ASTNodeType::AST_UNARY_OP && left->op == "DEREFERENCE") {
            // 間接参照への代入: *ptr = value
            // 複合代入はサポートしない（*ptr += value は未実装）
            if (op_type != TokenType::TOK_ASSIGN) {
                error("Compound assignment to dereferenced pointer is not yet supported");
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
    // Prefix operators: !, -, ~, &, *
    if (check(TokenType::TOK_NOT) || check(TokenType::TOK_MINUS) || 
        check(TokenType::TOK_BIT_NOT) || check(TokenType::TOK_BIT_AND) ||
        check(TokenType::TOK_MUL)) {
        Token op = advance();
        
        // & 演算子の場合、関数アドレス取得かチェック
        ASTNode* operand = parseUnary();
        
        ASTNode* unary = new ASTNode(ASTNodeType::AST_UNARY_OP);
        // & はアドレス演算子、* は間接参照演算子として扱う
        if (op.type == TokenType::TOK_BIT_AND) {
            unary->op = "ADDRESS_OF";  // アドレス演算子
            
            // operandから識別子名を取得して is_function_address フラグを設定
            // インタプリタ側で関数か変数かを判断する
            if (operand) {
                if (operand->node_type == ASTNodeType::AST_VARIABLE || 
                    operand->node_type == ASTNodeType::AST_IDENTIFIER) {
                    unary->is_function_address = true;
                    unary->function_address_name = operand->name;
                } else if (operand->node_type == ASTNodeType::AST_ARRAY_REF && 
                          operand->left && 
                          operand->left->node_type == ASTNodeType::AST_VARIABLE) {
                    // &arr[0] の場合、arr の名前を保存
                    unary->is_function_address = true;
                    unary->function_address_name = operand->left->name;
                }
            }
        } else if (op.type == TokenType::TOK_MUL) {
            unary->op = "DEREFERENCE";  // 間接参照演算子
        } else {
            unary->op = op.value;
        }
        unary->left = std::unique_ptr<ASTNode>(operand);
        
        return unary;
    }
    
    // ++ と -- は別処理: AST_PRE_INCDEC を生成
    if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
        Token op = advance();
        ASTNode* operand = parsePostfix();  // parsePostfix()を直接呼ぶことでメンバーアクセスを取得
        
        // AST_PRE_INCDEC ノードを生成
        ASTNode* incdec = new ASTNode(ASTNodeType::AST_PRE_INCDEC);
        incdec->op = op.value;
        incdec->left = std::unique_ptr<ASTNode>(operand);
        
        return incdec;
    }
    
    return parsePostfix();
}

ASTNode* RecursiveParser::parsePostfix() {
    ASTNode* primary = parsePrimary();
    
    if (!primary) {
        std::cerr << "[PARSER ERROR] parsePrimary returned null" << std::endl;
        return nullptr;
    }
    
    while (true) {
        // 関数ポインタ呼び出しのチェック: *ptr(args)
        // primaryが DEREFERENCE (*演算子) で、次が'('の場合
        if (check(TokenType::TOK_LPAREN) && 
            primary && primary->node_type == ASTNodeType::AST_UNARY_OP &&
            primary->op == "DEREFERENCE") {
            
            // 関数ポインタ呼び出しに変換
            advance(); // '(' を消費
            
            ASTNode* funcPtrCall = new ASTNode(ASTNodeType::AST_FUNC_PTR_CALL);
            funcPtrCall->left = std::move(primary->left);  // ポインタ変数（*の対象）
            
            // 引数の解析
            if (!check(TokenType::TOK_RPAREN)) {
                do {
                    ASTNode* arg = parseExpression();
                    funcPtrCall->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                } while (match(TokenType::TOK_COMMA));
            }
            
            consume(TokenType::TOK_RPAREN, "Expected ')' after function pointer call arguments");
            
            primary = funcPtrCall;
            continue;
        }
        
        if (check(TokenType::TOK_LBRACKET)) {
            // 配列アクセス: arr[i]
            advance(); // consume '['
            ASTNode* index = parseExpression();
            if (!index) {
                std::cerr << "[PARSER ERROR] parseExpression returned null for array index" << std::endl;
                return nullptr;
            }
            consume(TokenType::TOK_RBRACKET, "Expected ']'");
            
            ASTNode* array_ref = new ASTNode(ASTNodeType::AST_ARRAY_REF);
            array_ref->left = std::unique_ptr<ASTNode>(primary); // 左側を設定
            array_ref->array_index = std::unique_ptr<ASTNode>(index);
            
            // デバッグ: 配列アクセスノード作成をログ出力
            if (primary && primary->node_type == ASTNodeType::AST_VARIABLE) {
                debug_msg(DebugMsgId::PARSE_EXPR_ARRAY_ACCESS, primary->name.c_str());
            } else if (primary && primary->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                std::string member_access_name = primary->name + " (member access)";
                debug_msg(DebugMsgId::PARSE_EXPR_ARRAY_ACCESS, member_access_name.c_str());
            } else if (primary && primary->node_type == ASTNodeType::AST_ARRAY_REF) {
                debug_msg(DebugMsgId::PARSE_EXPR_ARRAY_ACCESS, "nested array access");
            }
            
            primary = array_ref; // 次のアクセスのベースとして設定
        } else if (check(TokenType::TOK_DOT)) {
            // メンバアクセス: obj.member
            primary = parseMemberAccess(primary);
            if (!primary) {
                std::cerr << "[PARSER ERROR] parseMemberAccess returned null" << std::endl;
                return nullptr;
            }
        } else if (check(TokenType::TOK_ARROW)) {
            // アロー演算子: ptr->member
            primary = parseArrowAccess(primary);
            if (!primary) {
                std::cerr << "[PARSER ERROR] parseArrowAccess returned null" << std::endl;
                return nullptr;
            }
        } else {
            break; // どちらでもない場合はループを抜ける
        }
    }
    
    // Postfix operators: ++, --
    if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
        Token op = advance();
        ASTNode* postfix = new ASTNode(ASTNodeType::AST_POST_INCDEC);
        postfix->op = op.value; // "++" または "--"
        postfix->left = std::unique_ptr<ASTNode>(primary);
        return postfix;
    }
    
    return primary;
}

ASTNode* RecursiveParser::parsePrimary() {
    if (check(TokenType::TOK_NUMBER)) {
        Token token = advance();
        ASTNode* node = new ASTNode(ASTNodeType::AST_NUMBER);

        std::string literal = token.value;
        node->literal_text = literal;

        // サフィックス解析（f/F, d/D, q/Q）
        char suffix = '\0';
        if (!literal.empty()) {
            char last_char = literal.back();
            if (last_char == 'f' || last_char == 'F' ||
                last_char == 'd' || last_char == 'D' ||
                last_char == 'q' || last_char == 'Q') {
                suffix = last_char;
                literal.pop_back();
            }
        }

        // 指数表記の小文字e/E対応のため、末尾サフィックスを除外した後で判定
        auto contains_decimal = [](const std::string& value) {
            return value.find('.') != std::string::npos;
        };
        auto contains_exponent = [](const std::string& value) {
            return value.find('e') != std::string::npos || value.find('E') != std::string::npos;
        };

        bool is_float_literal = contains_decimal(literal) || contains_exponent(literal) || suffix != '\0';

        try {
            if (is_float_literal) {
                node->is_float_literal = true;

                if (suffix == 'f' || suffix == 'F') {
                    node->literal_type = TYPE_FLOAT;
                    node->type_info = TYPE_FLOAT;
                    node->double_value = std::stod(literal);
                    node->quad_value = static_cast<long double>(node->double_value);
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
                    node->quad_value = static_cast<long double>(node->double_value);
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
            error("Invalid number: " + token.value);
            delete node;
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
    
    if (check(TokenType::TOK_NULLPTR)) {
        Token token = advance();
        ASTNode* node = new ASTNode(ASTNodeType::AST_NULLPTR);
        setLocation(node, token.line, token.column);
        return node;
    }
    
    if (check(TokenType::TOK_SELF)) {
        Token token = advance();
        ASTNode* node = new ASTNode(ASTNodeType::AST_IDENTIFIER);
        node->name = "self";
        setLocation(node, token.line, token.column);
        return node;
    }
    
    if (check(TokenType::TOK_IDENTIFIER)) {
        Token token = advance();
        
        // enum値アクセス（EnumName::member）をチェック
        if (check(TokenType::TOK_SCOPE)) {
            advance(); // consume '::'
            
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected enum member name after '::'");
                return nullptr;
            }
            
            std::string member_name = current_token_.value;
            advance(); // consume member name
            
            ASTNode* enum_access = new ASTNode(ASTNodeType::AST_ENUM_ACCESS);
            enum_access->enum_name = token.value;
            enum_access->enum_member = member_name;
            setLocation(enum_access, token.line, token.column);
            
            return enum_access;
        }
        
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
            
            // チェーン呼び出しのサポート: func()() 形式
            // 最初の呼び出しが関数ポインタを返す場合、続けて呼び出し可能
            while (check(TokenType::TOK_LPAREN)) {
                advance(); // consume '('
                
                // チェーン呼び出しノードを作成
                ASTNode* chained_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
                chained_call->left = std::unique_ptr<ASTNode>(call_node);  // 前の呼び出し結果を左側に
                
                // 引数リストの解析
                if (!check(TokenType::TOK_RPAREN)) {
                    do {
                        ASTNode* arg = parseExpression();
                        chained_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                    } while (match(TokenType::TOK_COMMA));
                }
                
                consume(TokenType::TOK_RPAREN, "Expected ')' after chained function arguments");
                
                call_node = chained_call;  // 次のイテレーションのために更新
            }
            
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
    function_node->is_unsigned = (return_type.rfind("unsigned ", 0) == 0);
    if (function_node->return_type_name.empty()) {
        function_node->return_type_name = return_type;
    }
    
    // DEBUG: 関数作成をログ出力
    debug_msg(DebugMsgId::PARSE_FUNCTION_CREATED, function_name.c_str());
    
    // パラメータリストの解析
    if (!check(TokenType::TOK_RPAREN)) {
        do {
            // パラメータ型
            std::string param_type = parseType();
            ParsedTypeInfo param_parsed = getLastParsedTypeInfo();
            
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
            std::string resolved_param_type = resolveTypedefChain(param_type);

            if (resolved_param_type.empty()) {
                resolved_param_type = param_type;
            }
            // 元のtypedef名を保持（interpreterで解決するため）
            // param->type_name = resolved_param_type;  // この行をコメントアウト
            
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
            } else if (struct_definitions_.find(resolved_param_type) != struct_definitions_.end() ||
                       struct_definitions_.find(param_type) != struct_definitions_.end() ||
                       (resolved_param_type.substr(0, 7) == "struct " && 
                        struct_definitions_.find(resolved_param_type.substr(7)) != struct_definitions_.end())) {
                // struct型の場合（解決後の型名、元のtypedef名、または"struct Name"形式で検索）
                param->type_info = TYPE_STRUCT;
            } else if (enum_definitions_.find(resolved_param_type) != enum_definitions_.end() ||
                       enum_definitions_.find(param_type) != enum_definitions_.end()) {
                // enum型の場合  
                param->type_info = TYPE_ENUM;
            } else if (union_definitions_.find(resolved_param_type) != union_definitions_.end() ||
                       union_definitions_.find(param_type) != union_definitions_.end()) {
                // ユニオン型の場合
                param->type_info = TYPE_UNION;
            } else {
                param->type_info = TYPE_UNKNOWN;
            }

            if (param->type_info == TYPE_UNKNOWN) {
                param->type_info = getTypeInfoFromString(param_type);
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
        if (!resolved_type.empty() && resolved_type != return_type) {
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
            if (!function_node->return_types.empty() &&
                function_node->return_types.back() == TYPE_UNKNOWN) {
                function_node->return_types.back() =
                    getTypeInfoFromString(resolved_type);
            }
        } else {
            // typedef型でない場合、getTypeInfoFromStringを使って型を判定
            TypeInfo type_info = getTypeInfoFromString(return_type);
            function_node->return_types.push_back(type_info);
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
    // typedef <type> <alias>; または typedef struct {...} <alias>; または typedef enum {...} <alias>; または typedef union (TypeScript-like literal types) または 関数ポインタtypedef
    consume(TokenType::TOK_TYPEDEF, "Expected 'typedef'");
    
    // typedef struct の場合
    if (check(TokenType::TOK_STRUCT)) {
        return parseStructTypedefDeclaration();
    }
    
    // typedef enum の場合
    if (check(TokenType::TOK_ENUM)) {
        return parseEnumTypedefDeclaration();
    }
    
    // 関数ポインタtypedefの場合
    if (isFunctionPointerTypedef()) {
        return parseFunctionPointerTypedefDeclaration();
    }
    
    // Check for both old and new typedef syntaxes
    // Old syntax: typedef TYPE ALIAS;
    // New syntax: typedef ALIAS = TYPE | TYPE2 | ...;
    
    // Check if this is new union syntax: typedef ALIAS = TYPE | TYPE2 | ...
    if (check(TokenType::TOK_IDENTIFIER)) {
        // Temporarily save current token to check for new syntax
        Token saved_identifier = current_token_;
        advance(); // consume identifier
        
        if (check(TokenType::TOK_ASSIGN)) {
            // New syntax: typedef ALIAS = TYPE | TYPE2 | ...
            std::string alias_name = saved_identifier.value;
            
            // Union typedef declaration
            consume(TokenType::TOK_ASSIGN, "Expected '=' after union typedef alias name");
            
            UnionDefinition union_def;
            union_def.name = alias_name;
            
            // Parse first value
            if (!parseUnionValue(union_def)) {
                error("Expected value after '=' in union typedef");
                return nullptr;
            }
            
            // Check if this is actually a union (has '|' separator)
            bool is_actual_union = false;
            
            // Parse additional values separated by '|'
            while (check(TokenType::TOK_PIPE)) {
                is_actual_union = true;
                advance(); // consume '|'
                if (!parseUnionValue(union_def)) {
                    error("Expected value after '|' in union typedef");
                    return nullptr;
                }
            }
            
            consume(TokenType::TOK_SEMICOLON, "Expected ';' after typedef declaration");
            
            // If it's not an actual union (no '|' found), treat as regular typedef
            if (!is_actual_union) {
                // This is a single-type alias like: typedef StringOnly = string;
                // Treat as regular typedef, not union
                
                // Check for single basic type
                if (union_def.allowed_types.size() == 1 && union_def.allowed_custom_types.empty() && 
                    union_def.allowed_array_types.empty() && !union_def.has_literal_values) {
                    
                    TypeInfo single_type = union_def.allowed_types[0];
                    std::string type_name_str;
                    switch(single_type) {
                        case TYPE_INT: type_name_str = "int"; break;
                        case TYPE_LONG: type_name_str = "long"; break;
                        case TYPE_SHORT: type_name_str = "short"; break;
                        case TYPE_TINY: type_name_str = "tiny"; break;
                        case TYPE_BOOL: type_name_str = "bool"; break;
                        case TYPE_STRING: type_name_str = "string"; break;
                        case TYPE_CHAR: type_name_str = "char"; break;
                        default: type_name_str = "unknown"; break;
                    }
                    
                    // Register as regular typedef
                    typedef_map_[alias_name] = type_name_str;
                    
                    // Create regular typedef AST node
                    ASTNode* node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
                    node->name = alias_name;
                    node->type_name = type_name_str;
                    node->type_info = single_type;
                    
                    setLocation(node, current_token_);
                    return node;
                }
                // Check for single custom type - treat as union to preserve custom type validation
                else if (union_def.allowed_custom_types.size() == 1 && union_def.allowed_types.empty() && 
                         union_def.allowed_array_types.empty() && !union_def.has_literal_values) {
                    
                    // Single custom type should be treated as union for type validation
                    // This preserves the semantic that only the specific custom type is allowed
                    // (not any type that resolves to the same basic type)
                    
                    // Store union definition for type checking
                    union_definitions_[alias_name] = union_def;
                    
                    // Create union typedef AST node
                    ASTNode* node = new ASTNode(ASTNodeType::AST_UNION_TYPEDEF_DECL);
                    node->name = alias_name;
                    node->type_info = TYPE_UNION;
                    node->union_name = alias_name;
                    node->union_definition = union_def;
                    
                    setLocation(node, current_token_);
                    return node;
                }
            }
            
            // Store union definition
            union_definitions_[alias_name] = union_def;
            
            // Create AST node
            ASTNode* node = new ASTNode(ASTNodeType::AST_UNION_TYPEDEF_DECL);
            node->name = alias_name;
            node->type_info = TYPE_UNION;
            node->union_name = alias_name;
            node->union_definition = union_def;
            
            setLocation(node, current_token_);
            
            return node;
        } else {
            // This is old syntax: typedef TYPE ALIAS;
            // The identifier we consumed is actually the base type
            std::string base_type_name = saved_identifier.value;
            TypeInfo base_type = TYPE_UNKNOWN;
            
            // Check if it's a known typedef type
            if (typedef_map_.find(base_type_name) != typedef_map_.end()) {
                std::string resolved_type = resolveTypedefChain(base_type_name);
                if (resolved_type.empty()) {
                    error("Unknown typedef type: " + base_type_name);
                    throw std::runtime_error("Unknown typedef type: " + base_type_name);
                }
                base_type_name = resolved_type;
                base_type = getTypeInfoFromString(extractBaseType(resolved_type));
            }
            // Check if it's a struct type
            else if (struct_definitions_.find(base_type_name) != struct_definitions_.end()) {
                base_type = TYPE_STRUCT;
            }
            // Check if it's an enum type
            else if (enum_definitions_.find(base_type_name) != enum_definitions_.end()) {
                base_type = TYPE_INT; // enums are treated as int internally
            }
            else {
                error("Unknown type: " + base_type_name);
                throw std::runtime_error("Unknown type: " + base_type_name);
            }
            
            // Now parse the alias name
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected identifier for typedef alias");
                return nullptr;
            }
            
            std::string alias_name = current_token_.value;
            advance();
            
            consume(TokenType::TOK_SEMICOLON, "Expected ';' after typedef declaration");
            
            // Add typedef mapping
            typedef_map_[alias_name] = base_type_name;
            
            // Create AST node
            ASTNode* node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
            node->name = alias_name;
            node->type_info = base_type;
            node->type_name = base_type_name;
            
            setLocation(node, current_token_);
            
            return node;
        }
    }
    
    // Old syntax: typedef TYPE ALIAS;
    // Parse base type first
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
    } else if (check(TokenType::TOK_FLOAT)) {
        base_type = TYPE_FLOAT;
        base_type_name = "float";
        advance();
    } else if (check(TokenType::TOK_DOUBLE)) {
        base_type = TYPE_DOUBLE;
        base_type_name = "double";
        advance();
    } else if (check(TokenType::TOK_BIG)) {
        base_type = TYPE_BIG;
        base_type_name = "big";
        advance();
    } else if (check(TokenType::TOK_QUAD)) {
        base_type = TYPE_QUAD;
        base_type_name = "quad";
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
        // 既存のstruct/enum型またはtypedef型を参照する場合
        std::string identifier = advance().value;
        
        // struct定義が存在するかチェック
        if (struct_definitions_.find(identifier) != struct_definitions_.end()) {
            base_type = TYPE_STRUCT;
            base_type_name = identifier;
        }
        // enum定義が存在するかチェック
        else if (enum_definitions_.find(identifier) != enum_definitions_.end()) {
            base_type = TYPE_INT; // enumは内部的にintとして扱う
            base_type_name = identifier;
        }
        // 既存のtypedef型を参照する場合
        else if (typedef_map_.find(identifier) != typedef_map_.end()) {
            std::string resolved_type = resolveTypedefChain(identifier);
            if (resolved_type.empty()) {
                error("Unknown typedef type: " + identifier);
                throw std::runtime_error("Unknown typedef type: " + identifier);
            }
            base_type_name = resolved_type;
            base_type = getTypeInfoFromString(extractBaseType(resolved_type));
        } else {
            error("Unknown type: " + identifier);
            throw std::runtime_error("Unknown type: " + identifier);
        }
    } else {
        error("Expected type after typedef");
        return nullptr;
    }
    
    // Check for array type specification: TYPE[size], TYPE[SIZE_CONSTANT], or multidimensional TYPE[size1][size2]...
    while (check(TokenType::TOK_LBRACKET)) {
        advance(); // consume '['
        
        std::string array_size;
        if (check(TokenType::TOK_NUMBER)) {
            array_size = current_token_.value;
            advance(); // consume array size
        } else if (check(TokenType::TOK_IDENTIFIER)) {
            // Allow identifier (like const variable name) as array size
            array_size = current_token_.value;
            advance(); // consume identifier
        } else {
            error("Expected array size in typedef");
            return nullptr;
        }
        
        consume(TokenType::TOK_RBRACKET, "Expected ']' after array size");
        
        // Append array dimension to type name
        base_type_name = base_type_name + "[" + array_size + "]";
    }
    
    // Now parse the alias name
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected identifier for typedef alias");
        return nullptr;
    }
    
    std::string alias_name = current_token_.value;
    advance();
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after typedef declaration");
    
    // Add typedef mapping
    typedef_map_[alias_name] = base_type_name;
    
    // Create AST node
    ASTNode* node = new ASTNode(ASTNodeType::AST_TYPEDEF_DECL);
    node->name = alias_name;
    node->type_info = base_type;
    node->type_name = base_type_name;
    
    setLocation(node, current_token_);
    
    return node;
}

TypeInfo RecursiveParser::getTypeInfoFromString(const std::string& type_name) {
    if (type_name == "nullptr") {
        return TYPE_NULLPTR;
    }

    std::string working = type_name;
    bool is_unsigned = false;
    if (working.rfind("unsigned ", 0) == 0) {
        is_unsigned = true;
        working = working.substr(9);
    }

    if (working.find('*') != std::string::npos) {
        return TYPE_POINTER;
    }

    // 配列型のチェック（1次元・多次元両対応）
    if (working.find('[') != std::string::npos) {
        std::string base_type = working.substr(0, working.find('['));
        if (base_type == "int") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
        } else if (base_type == "string") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
        } else if (base_type == "bool") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL);
        } else if (base_type == "long") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_LONG);
        } else if (base_type == "short") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_SHORT);
        } else if (base_type == "tiny") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_TINY);
        } else if (base_type == "char") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_CHAR);
        } else if (base_type == "float") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_FLOAT);
        } else if (base_type == "double") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_DOUBLE);
        } else if (base_type == "big") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BIG);
        } else if (base_type == "quad") {
            return static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_QUAD);
        } else {
            return TYPE_UNKNOWN;
        }
    }
    
    if (working == "int") {
        return TYPE_INT;
    } else if (working == "long") {
        return TYPE_LONG;
    } else if (working == "short") {
        return TYPE_SHORT;
    } else if (working == "tiny") {
        return TYPE_TINY;
    } else if (working == "bool") {
        return TYPE_BOOL;
    } else if (working == "string") {
        return TYPE_STRING;
    } else if (working == "char") {
        return TYPE_CHAR;
    } else if (working == "float") {
        return TYPE_FLOAT;
    } else if (working == "double") {
        return TYPE_DOUBLE;
    } else if (working == "big") {
        return TYPE_BIG;
    } else if (working == "quad") {
        return TYPE_QUAD;
    } else if (working == "void") {
        return TYPE_VOID;
    } else if (working.substr(0, 7) == "struct " || struct_definitions_.find(working) != struct_definitions_.end()) {
        return TYPE_STRUCT;
    } else if (working.substr(0, 5) == "enum " || enum_definitions_.find(working) != enum_definitions_.end()) {
        return TYPE_ENUM;
    } else if (working.substr(0, 10) == "interface " || interface_definitions_.find(working) != interface_definitions_.end()) {
        return TYPE_INTERFACE;
    } else if (union_definitions_.find(working) != union_definitions_.end()) {
        return TYPE_UNION;
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

ASTNode* RecursiveParser::parseAssertStatement() {
    Token assert_token = advance(); // consume 'assert'
    
    consume(TokenType::TOK_LPAREN, "Expected '(' after assert");
    
    // 条件式をパース
    ASTNode* condition = parseExpression();
    
    consume(TokenType::TOK_RPAREN, "Expected ')' after assert condition");
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after assert statement");
    
    ASTNode* assert_node = new ASTNode(ASTNodeType::AST_ASSERT_STMT);
    assert_node->left = std::unique_ptr<ASTNode>(condition);
    assert_node->location.line = assert_token.line;
    
    return assert_node;
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
        
        // 自分自身を指している場合（匿名structのtypedef）
        if (next == current) {
            // 構造体定義が存在するかチェック
            if (struct_definitions_.find(current) != struct_definitions_.end()) {
                return current;
            }
            return "";
        }
        
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
    
    // "struct StructName"形式のチェック
    if (current.substr(0, 7) == "struct " && current.length() > 7) {
        std::string struct_name = current.substr(7);
        if (struct_definitions_.find(struct_name) != struct_definitions_.end()) {
            return current; // "struct StructName"のまま返す
        }
    }
    
    // 構造体型かチェック（裸の構造体名）
    if (struct_definitions_.find(current) != struct_definitions_.end()) {
        return current; // 構造体名をそのまま返す
    }
    
    // enum型かチェック（裸のenum名）
    if (enum_definitions_.find(current) != enum_definitions_.end()) {
        return current; // enum名をそのまま返す
    }
    
    // 配列型かチェック（int[2], int[2][2]など）
    if (current.find('[') != std::string::npos) {
        return current; // 配列型名をそのまま返す
    }
    
    // ユニオン型かチェック（裸のユニオン名）
    if (union_definitions_.find(current) != union_definitions_.end()) {
        return current; // ユニオン名をそのまま返す
    }
    
    // 未定義型の場合は空文字列を返す
    return "";
}

// 型名から基本型部分を抽出する

ASTNode* RecursiveParser::parseTypedefVariableDeclaration() {
    // typedef型名を取得
    std::string typedef_name = advance().value;
    std::string resolved_type = resolveTypedefChain(typedef_name);
    
    // ポインタ深度をチェック
    int pointer_depth = 0;
    while (check(TokenType::TOK_MUL)) {
        pointer_depth++;
        advance();
    }
    
    // 参照チェック
    bool is_reference = false;
    if (check(TokenType::TOK_BIT_AND)) {
        is_reference = true;
        advance();
    }
    
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
    
    // ポインタと参照の情報を設定
    if (pointer_depth > 0) {
        node->is_pointer = true;
        node->pointer_depth = pointer_depth;
        node->pointer_base_type_name = typedef_name;
        
        // typedefの解決済み型を取得
        TypeInfo base_type_info = TYPE_UNKNOWN;
        if (!resolved_type.empty()) {
            // union型の場合
            if (union_definitions_.find(resolved_type) != union_definitions_.end()) {
                base_type_info = TYPE_UNION;
            } else {
                base_type_info = getTypeInfoFromString(resolved_type);
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
        
        if (check(TokenType::TOK_LBRACE)) {
            // 構造体リテラル初期化
            node->init_expr = std::unique_ptr<ASTNode>(parseStructLiteral());
        } else if (check(TokenType::TOK_LBRACKET)) {
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
    
    // 前方宣言のチェック: struct Name; の形式
    if (check(TokenType::TOK_SEMICOLON)) {
        advance(); // セミコロンを消費
        
        // 前方宣言として登録（既に定義済みでなければ）
        if (struct_definitions_.find(struct_name) == struct_definitions_.end()) {
            StructDefinition forward_decl(struct_name);
            forward_decl.is_forward_declaration = true;
            struct_definitions_[struct_name] = forward_decl;
            
            debug_msg(DebugMsgId::PARSE_STRUCT_DEF, 
                     ("Forward declaration: " + struct_name).c_str());
        }
        
        // 前方宣言用のASTノードを作成
        ASTNode* node = new ASTNode(ASTNodeType::AST_STRUCT_DECL);
        node->name = struct_name;
        setLocation(node, current_token_);
        return node;
    }
    
    consume(TokenType::TOK_LBRACE, "Expected '{' after struct name");
    
    StructDefinition struct_def(struct_name);
    
    // 自己参照を許可するため、構造体名を前方宣言として登録
    // これにより、メンバーのパース中に "Node*" などの型を認識できる
    struct_definitions_[struct_name] = struct_def;
    
    // メンバ変数の解析
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        bool is_private_member = false;
        if (check(TokenType::TOK_PRIVATE)) {
            is_private_member = true;
            advance();
        }

        // const修飾子のチェック
        bool is_const_member = false;
        if (check(TokenType::TOK_CONST)) {
            is_const_member = true;
            advance();
        }

        // メンバの型を解析
        std::string member_type = parseType();
        
        if (member_type.empty()) {
            error("Expected member type in struct definition");
            return nullptr;
        }
        
        ParsedTypeInfo member_parsed = getLastParsedTypeInfo();

        // メンバ名のリストを解析（int[2][2] a, b; のような複数宣言に対応）
        do {
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected member name");
                return nullptr;
            }

            std::string member_name = current_token_.value;
            advance();

            ParsedTypeInfo var_parsed = member_parsed;
            TypeInfo member_type_info = resolveParsedTypeInfo(var_parsed);

            // 自己再帰構造体チェック: 自分自身の型のメンバーはポインタでなければならない
            std::string member_base_type = var_parsed.base_type;
            if (member_base_type.empty()) {
                member_base_type = var_parsed.full_type;
            }
            // "struct " プレフィックスを除去
            if (member_base_type.rfind("struct ", 0) == 0) {
                member_base_type = member_base_type.substr(7);
            }
            
            if (member_base_type == struct_name && !var_parsed.is_pointer) {
                error("Self-recursive struct member '" + member_name + 
                      "' must be a pointer type. Use '" + struct_name + "* " + 
                      member_name + ";' instead of '" + struct_name + " " + 
                      member_name + ";'");
                return nullptr;
            }

            struct_def.add_member(member_name,
                                  member_type_info,
                                  var_parsed.full_type,
                                  var_parsed.is_pointer,
                                  var_parsed.pointer_depth,
                                  var_parsed.base_type,
                                  var_parsed.base_type_info,
                                  is_private_member,
                                  var_parsed.is_reference,
                                  var_parsed.is_unsigned,
                                  is_const_member);

            if (var_parsed.is_array) {
                StructMember &added = struct_def.members.back();
                added.array_info = var_parsed.array_info;
            }

            // 旧式の配列宣言をチェック（int data[2][2];）- エラーとして処理
            if (check(TokenType::TOK_LBRACKET)) {
                error("Old-style array declaration is not supported in struct members. Use 'int[2][2] member_name;' instead of 'int member_name[2][2];'");
                return nullptr;
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
    // 前方宣言を完全な定義で上書き
    struct_def.is_forward_declaration = false;
    struct_definitions_[struct_name] = struct_def;
    
    // 循環参照チェック: 値メンバーによる循環を検出
    for (const auto& member : struct_def.members) {
        // ポインタメンバーと配列メンバーはスキップ
        if (member.is_pointer || member.array_info.is_array()) {
            continue;
        }
        
        std::unordered_set<std::string> visited;
        std::vector<std::string> path;
        path.push_back(struct_name);
        
        if (detectCircularReference(struct_name, member.type_alias, visited, path)) {
            std::string cycle_path = struct_name;
            for (size_t i = 1; i < path.size(); ++i) {
                cycle_path += " -> " + path[i];
            }
            error("Circular reference detected in struct value members: " + cycle_path + 
                  ". Use pointers to break the cycle.");
            return nullptr;
        }
    }
    
    debug_msg(DebugMsgId::PARSE_STRUCT_DEF, struct_name.c_str());
    
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
        member_node->is_pointer = member.is_pointer;
        member_node->pointer_depth = member.pointer_depth;
        member_node->pointer_base_type_name = member.pointer_base_type_name;
        member_node->pointer_base_type = member.pointer_base_type;
        member_node->is_reference = member.is_reference;
        member_node->is_unsigned = member.is_unsigned;
        member_node->is_private_member = member.is_private;
        member_node->is_const = member.is_const;
        member_node->array_type_info = member.array_info;  // 配列情報をコピー
        if (member.array_info.is_array()) {
            member_node->is_array = true;
        }
        node->arguments.push_back(std::unique_ptr<ASTNode>(member_node));
    }
    
    return node;
}

// typedef struct宣言の解析: typedef struct { members } alias;
ASTNode* RecursiveParser::parseStructTypedefDeclaration() {
    consume(TokenType::TOK_STRUCT, "Expected 'struct'");
    
    // オプションのタグ名をチェック: typedef struct Tag { ... } Alias;
    std::string tag_name;
    if (check(TokenType::TOK_IDENTIFIER)) {
        tag_name = current_token_.value;
        advance();
    }
    
    consume(TokenType::TOK_LBRACE, "Expected '{' after 'typedef struct'");
    
    StructDefinition struct_def;
    if (!tag_name.empty()) {
        struct_def.name = tag_name;
        // 自己参照を許可するため、タグ名を前方宣言として登録
        struct_definitions_[tag_name] = struct_def;
    }
    
    // メンバ変数の解析
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        bool is_private_member = false;
        if (check(TokenType::TOK_PRIVATE)) {
            is_private_member = true;
            advance();
        }

        // const修飾子のチェック
        bool is_const_member = false;
        if (check(TokenType::TOK_CONST)) {
            is_const_member = true;
            advance();
        }

        std::string member_type = parseType();
        if (member_type.empty()) {
            error("Expected member type in struct definition");
            return nullptr;
        }

        ParsedTypeInfo member_parsed = getLastParsedTypeInfo();

        do {
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected member name");
                return nullptr;
            }

            std::string member_name = current_token_.value;
            advance();

            ParsedTypeInfo var_parsed = member_parsed;
            TypeInfo member_type_info = resolveParsedTypeInfo(var_parsed);

            // 自己再帰構造体チェック (typedef structの場合はタグ名でチェック)
            if (!tag_name.empty()) {
                std::string member_base_type = var_parsed.base_type;
                if (member_base_type.empty()) {
                    member_base_type = var_parsed.full_type;
                }
                // "struct " プレフィックスを除去
                if (member_base_type.rfind("struct ", 0) == 0) {
                    member_base_type = member_base_type.substr(7);
                }
                
                if (member_base_type == tag_name && !var_parsed.is_pointer) {
                    error("Self-recursive struct member '" + member_name + 
                          "' must be a pointer type. Use '" + tag_name + "* " + 
                          member_name + ";' instead of '" + tag_name + " " + 
                          member_name + ";'");
                    return nullptr;
                }
            }

            struct_def.add_member(member_name,
                                  member_type_info,
                                  var_parsed.full_type,
                                  var_parsed.is_pointer,
                                  var_parsed.pointer_depth,
                                  var_parsed.base_type,
                                  var_parsed.base_type_info,
                                  is_private_member,
                                  var_parsed.is_reference,
                                  var_parsed.is_unsigned,
                                  is_const_member);

            if (var_parsed.is_array) {
                StructMember &added = struct_def.members.back();
                added.array_info = var_parsed.array_info;
            }

            if (check(TokenType::TOK_COMMA)) {
                advance();
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
    
    // タグ名がある場合は、タグ名でも再度登録（完全な定義で上書き）
    if (!tag_name.empty()) {
        struct_def.name = tag_name;
        struct_definitions_[tag_name] = struct_def;
    }
    
    // 循環参照チェック: 値メンバーによる循環を検出
    std::string check_name = !tag_name.empty() ? tag_name : alias_name;
    for (const auto& member : struct_def.members) {
        // ポインタメンバーと配列メンバーはスキップ
        if (member.is_pointer || member.array_info.is_array()) {
            continue;
        }
        
        std::unordered_set<std::string> visited;
        std::vector<std::string> path;
        path.push_back(check_name);
        
        if (detectCircularReference(check_name, member.type_alias, visited, path)) {
            std::string cycle_path = check_name;
            for (size_t i = 1; i < path.size(); ++i) {
                cycle_path += " -> " + path[i];
            }
            error("Circular reference detected in struct value members: " + cycle_path + 
                  ". Use pointers to break the cycle.");
            return nullptr;
        }
    }
    
    // typedef_map_に登録（タグ名がある場合はタグ名へ、ない場合はエイリアス名自身へ）
    if (!tag_name.empty()) {
        // タグ名がある場合: typedef struct Tag { ... } Alias;
        // Alias -> Tag のマッピング
        typedef_map_[alias_name] = tag_name;
    } else {
        // タグ名がない場合: typedef struct { ... } Alias;
        // Alias -> Alias のマッピング（エイリアス自身がstruct定義名）
        typedef_map_[alias_name] = alias_name;
    }
    
    // ASTノードを作成
    ASTNode* node = new ASTNode(ASTNodeType::AST_STRUCT_TYPEDEF_DECL);
    node->name = alias_name;
    setLocation(node, current_token_);
    
    // struct定義のメンバー情報をASTノードに保存
    for (const auto& member : struct_def.members) {
        ASTNode* member_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        member_node->name = member.name;
        member_node->type_name = member.type_alias;
        member_node->type_info = member.type;
        member_node->is_pointer = member.is_pointer;
        member_node->pointer_depth = member.pointer_depth;
        member_node->pointer_base_type_name = member.pointer_base_type_name;
        member_node->pointer_base_type = member.pointer_base_type;
    member_node->is_reference = member.is_reference;
    member_node->is_unsigned = member.is_unsigned;
        member_node->is_private_member = member.is_private;
        
        // 配列メンバーの場合はarray_type_infoを設定
        if (member.array_info.is_array()) {
            member_node->array_type_info = member.array_info;
            member_node->is_array = true;
        }
        
        node->arguments.push_back(std::unique_ptr<ASTNode>(member_node));
    }
    
    return node;
}

// typedef enum宣言の解析: typedef enum { values } alias;
ASTNode* RecursiveParser::parseEnumTypedefDeclaration() {
    consume(TokenType::TOK_ENUM, "Expected 'enum'");
    
    consume(TokenType::TOK_LBRACE, "Expected '{' after 'typedef enum'");
    
    EnumDefinition enum_def;
    int64_t next_value = 0;
    
    // enumメンバの解析
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        if (!check(TokenType::TOK_IDENTIFIER)) {
            error("Expected enum member name");
            return nullptr;
        }
        
        std::string member_name = current_token_.value;
        advance();
        
        int64_t member_value = next_value;
        bool explicit_value = false;
        
        // 明示的な値の指定があるかチェック
        if (check(TokenType::TOK_ASSIGN)) {
            advance(); // consume '='
            
            // 負の数をサポート
            bool is_negative = false;
            if (check(TokenType::TOK_MINUS)) {
                is_negative = true;
                advance(); // consume '-'
            }
            
            if (!check(TokenType::TOK_NUMBER)) {
                error("Expected number after '=' in enum member");
                return nullptr;
            }
            
            member_value = std::stoll(current_token_.value);
            if (is_negative) {
                member_value = -member_value;
            }
            explicit_value = true;
            advance();
        }
        
        // enumメンバを追加
        enum_def.add_member(member_name, member_value, explicit_value);
        next_value = member_value + 1;
        
        if (check(TokenType::TOK_COMMA)) {
            advance(); // consume ','
        } else if (!check(TokenType::TOK_RBRACE)) {
            error("Expected ',' or '}' after enum member");
            return nullptr;
        }
    }
    
    consume(TokenType::TOK_RBRACE, "Expected '}' after enum members");
    
    // typedef エイリアス名を取得
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected typedef alias name");
        return nullptr;
    }
    
    std::string alias_name = current_token_.value;
    advance();
    
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after typedef enum declaration");
    
    // enum定義を保存（エイリアス名で）
    enum_def.name = alias_name;
    enum_definitions_[alias_name] = enum_def;
    
    // typedef マッピングも追加
    typedef_map_[alias_name] = "enum " + alias_name;
    
    // ASTノードを作成
    ASTNode* node = new ASTNode(ASTNodeType::AST_ENUM_TYPEDEF_DECL);
    node->name = alias_name;
    node->type_info = TYPE_ENUM;
    
    // enum定義情報をASTに格納
    for (const auto& member : enum_def.members) {
        ASTNode* member_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        member_node->name = member.name;
        member_node->int_value = member.value;
        member_node->type_info = TYPE_INT; // enum値は整数
        node->arguments.push_back(std::unique_ptr<ASTNode>(member_node));
    }
    
    setLocation(node, current_token_);
    
    return node;
}

// typedef union宣言の解析 (TypeScript-like literal types): typedef NAME = value1 | value2 | ...
ASTNode* RecursiveParser::parseUnionTypedefDeclaration() {
    // This method is now integrated into parseTypedefDeclaration()
    // This function is kept for compatibility but should not be called directly
    error("parseUnionTypedefDeclaration should not be called directly");
    return nullptr;
}

// Union値の解析ヘルパー関数
bool RecursiveParser::parseUnionValue(UnionDefinition& union_def) {
    if (check(TokenType::TOK_NUMBER)) {
        // 数値リテラル (int)
        std::string value_str = current_token_.value;
        int64_t int_value = std::stoll(value_str);
        advance();
        
        UnionValue union_val(int_value);
        union_def.add_literal_value(union_val);
        return true;
        
    } else if (check(TokenType::TOK_STRING)) {
        // 文字列リテラル
        std::string str_value = current_token_.value;
        advance();
        
        UnionValue union_val(str_value);
        union_def.add_literal_value(union_val);
        return true;
        
    } else if (check(TokenType::TOK_CHAR)) {
        // 文字リテラル
        std::string char_str = current_token_.value;
        advance();
        
        // Lexer already removed quotes, so char_str contains just the character
        if (char_str.length() == 1) {
            char char_value = char_str[0];
            UnionValue union_val(char_value);
            union_def.add_literal_value(union_val);
            return true;
        } else if (char_str.length() >= 3 && char_str[0] == '\'' && char_str[char_str.length()-1] == '\'') {
            // Handle case where quotes are still present
            char char_value = char_str[1];
            UnionValue union_val(char_value);
            union_def.add_literal_value(union_val);
            return true;
        }
        error("Invalid character literal: '" + char_str + "' (length: " + std::to_string(char_str.length()) + ")");
        return false;
        
    } else if (check(TokenType::TOK_TRUE)) {
        // boolean true
        advance();
        
        UnionValue union_val(true);
        union_def.add_literal_value(union_val);
        return true;
        
    } else if (check(TokenType::TOK_FALSE)) {
        // boolean false
        advance();
        
        UnionValue union_val(false);
        union_def.add_literal_value(union_val);
        return true;
        
    } else if (check(TokenType::TOK_INT)) {
        // int type keyword
        advance();
        // Check for array type (int[size])
        if (check(TokenType::TOK_LBRACKET)) {
            advance(); // consume '['
            if (check(TokenType::TOK_NUMBER)) {
                std::string size = current_token_.value;
                advance(); // consume the size number
                if (check(TokenType::TOK_RBRACKET)) {
                    advance(); // consume ']'
                    union_def.add_allowed_array_type("int[" + size + "]");
                    return true;
                } else {
                    error("Expected ']' after array size");
                    return false;
                }
            } else {
                error("Expected array size after '[' in array type");
                return false;
            }
        }
        union_def.add_allowed_type(TYPE_INT);
        return true;
        
    } else if (check(TokenType::TOK_LONG)) {
        // long type keyword
        advance();
        union_def.add_allowed_type(TYPE_LONG);
        return true;
        
    } else if (check(TokenType::TOK_SHORT)) {
        // short type keyword
        advance();
        union_def.add_allowed_type(TYPE_SHORT);
        return true;
        
    } else if (check(TokenType::TOK_TINY)) {
        // tiny type keyword
        advance();
        union_def.add_allowed_type(TYPE_TINY);
        return true;
        
    } else if (check(TokenType::TOK_BOOL)) {
        // bool type keyword
        advance();
        // Check for array type (bool[size])
        if (check(TokenType::TOK_LBRACKET)) {
            advance(); // consume '['
            if (check(TokenType::TOK_NUMBER)) {
                std::string size = current_token_.value;
                advance(); // consume the size number
                if (check(TokenType::TOK_RBRACKET)) {
                    advance(); // consume ']'
                    union_def.add_allowed_array_type("bool[" + size + "]");
                    return true;
                } else {
                    error("Expected ']' after array size");
                    return false;
                }
            } else {
                error("Expected array size after '[' in array type");
                return false;
            }
        }
        union_def.add_allowed_type(TYPE_BOOL);
        return true;
        
    } else if (check(TokenType::TOK_STRING_TYPE)) {
        // string type keyword
        advance();
        // Check for array type (string[size])
        if (check(TokenType::TOK_LBRACKET)) {
            advance(); // consume '['
            if (check(TokenType::TOK_NUMBER)) {
                std::string size = current_token_.value;
                advance(); // consume the size number
                if (check(TokenType::TOK_RBRACKET)) {
                    advance(); // consume ']'
                    union_def.add_allowed_array_type("string[" + size + "]");
                    return true;
                } else {
                    error("Expected ']' after array size");
                    return false;
                }
            } else {
                error("Expected array size after '[' in array type");
                return false;
            }
        }
        union_def.add_allowed_type(TYPE_STRING);
        return true;
        
    } else if (check(TokenType::TOK_CHAR_TYPE)) {
        // char type keyword
        advance();
        union_def.add_allowed_type(TYPE_CHAR);
        return true;
        
    } else if (check(TokenType::TOK_VOID)) {
        // void type keyword
        advance();
        union_def.add_allowed_type(TYPE_VOID);
        return true;
        
    } else if (check(TokenType::TOK_IDENTIFIER)) {
        // Type name (for type unions like user-defined types)
        std::string type_name = current_token_.value;
        advance();
        
        // Check for array type (type_name[size])
        if (check(TokenType::TOK_LBRACKET)) {
            advance(); // consume '['
            if (check(TokenType::TOK_NUMBER)) {
                std::string size = current_token_.value;
                advance(); // consume the size number
                if (check(TokenType::TOK_RBRACKET)) {
                    advance(); // consume ']'
                    // This is an array type with size
                    union_def.add_allowed_array_type(type_name + "[" + size + "]");
                    return true;
                } else {
                    error("Expected ']' after array size");
                    return false;
                }
            } else {
                error("Expected array size after '[' in array type");
                return false;
            }
        }
        
        // Could be typedef, struct, or enum type
        if (typedef_map_.find(type_name) != typedef_map_.end()) {
            // This is a typedef - add as custom type
            union_def.add_allowed_custom_type(type_name);
            debug_print("UNION_PARSE_DEBUG: Added typedef custom type '%s' to union\n", type_name.c_str());
            return true;
        }
        
        if (struct_definitions_.find(type_name) != struct_definitions_.end()) {
            // This is a struct - add as custom type
            union_def.add_allowed_custom_type(type_name);
            debug_print("UNION_PARSE_DEBUG: Added struct custom type '%s' to union\n", type_name.c_str());
            return true;
        }
        
        if (enum_definitions_.find(type_name) != enum_definitions_.end()) {
            // This is an enum - add as custom type
            union_def.add_allowed_custom_type(type_name);
            debug_print("UNION_PARSE_DEBUG: Added enum custom type '%s' to union\n", type_name.c_str());
            return true;
        }
        
        // Unknown custom type - still add it (might be defined later)
        union_def.add_allowed_custom_type(type_name);
        debug_print("UNION_PARSE_DEBUG: Added unknown custom type '%s' to union\n", type_name.c_str());
        return true;
    }
    
    error("Expected literal value or type name in union");
    return false;
}

// メンバアクセスの解析: obj.member
ASTNode* RecursiveParser::parseMemberAccess(ASTNode* object) {
    consume(TokenType::TOK_DOT, "Expected '.'");
    
    std::string member_name;
    if (check(TokenType::TOK_IDENTIFIER)) {
        member_name = current_token_.value;
        advance();
    } else if (check(TokenType::TOK_PRINT) || check(TokenType::TOK_PRINTLN) || check(TokenType::TOK_PRINTF)) {
        // 予約キーワードだが、メソッド名として許可
        member_name = current_token_.value;
        advance();
    } else {
        error("Expected member name after '.'");
        return nullptr;
    }
    
    // メソッド呼び出しかチェック（obj.method()）
    if (check(TokenType::TOK_LPAREN)) {
        // メソッド呼び出し
        ASTNode* method_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
        method_call->name = member_name;
        method_call->left = std::unique_ptr<ASTNode>(object); // レシーバー
        setLocation(method_call, current_token_);
        
        // パラメータリストを解析
        advance(); // consume '('
        
        if (!check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode* arg = parseExpression();
                if (!arg) {
                    error("Expected argument expression");
                    return nullptr;
                }
                method_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                
                if (!check(TokenType::TOK_COMMA)) {
                    break;
                }
                advance(); // consume ','
            } while (!check(TokenType::TOK_RPAREN) && !isAtEnd());
        }
        
        consume(TokenType::TOK_RPAREN, "Expected ')' after method arguments");
        
        return method_call;
    }
    
    // 通常のメンバアクセス (連続ドット処理はparsePostfixループに任せる)
    ASTNode* member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
    member_access->left = std::unique_ptr<ASTNode>(object);
    member_access->name = member_name; // メンバ名を保存
    setLocation(member_access, current_token_);
    
    return member_access;
}

// アロー演算子アクセスの解析: ptr->member
ASTNode* RecursiveParser::parseArrowAccess(ASTNode* object) {
    consume(TokenType::TOK_ARROW, "Expected '->'");
    
    std::string member_name;
    if (check(TokenType::TOK_IDENTIFIER)) {
        member_name = current_token_.value;
        advance();
    } else if (check(TokenType::TOK_PRINT) || check(TokenType::TOK_PRINTLN) || check(TokenType::TOK_PRINTF)) {
        // 予約キーワードだが、メソッド名として許可
        member_name = current_token_.value;
        advance();
    } else {
        error("Expected member name after '->'");
        return nullptr;
    }
    
    // メソッド呼び出しかチェック（ptr->method()）
    if (check(TokenType::TOK_LPAREN)) {
        // メソッド呼び出し
        ASTNode* method_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
        method_call->name = member_name;
        method_call->left = std::unique_ptr<ASTNode>(object); // レシーバー（ポインタ）
        method_call->is_arrow_call = true; // アロー演算子経由のフラグ
        setLocation(method_call, current_token_);
        
        // パラメータリストを解析
        advance(); // consume '('
        
        if (!check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode* arg = parseExpression();
                if (!arg) {
                    error("Expected argument expression");
                    return nullptr;
                }
                method_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                
                if (!check(TokenType::TOK_COMMA)) {
                    break;
                }
                advance(); // consume ','
            } while (!check(TokenType::TOK_RPAREN) && !isAtEnd());
        }
        
        consume(TokenType::TOK_RPAREN, "Expected ')' after method arguments");
        
        return method_call;
    }
    
    // アロー演算子アクセス: ptr->member は (*ptr).member と等価
    ASTNode* arrow_access = new ASTNode(ASTNodeType::AST_ARROW_ACCESS);
    arrow_access->left = std::unique_ptr<ASTNode>(object);
    arrow_access->name = member_name; // メンバ名を保存
    setLocation(arrow_access, current_token_);
    
    return arrow_access;
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

ASTNode* RecursiveParser::parseEnumDeclaration() {
    consume(TokenType::TOK_ENUM, "Expected 'enum'");
    
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected enum name");
        return nullptr;
    }
    
    std::string enum_name = current_token_.value;
    advance(); // consume enum name
    
    consume(TokenType::TOK_LBRACE, "Expected '{' after enum name");
    
    ASTNode* enum_decl = new ASTNode(ASTNodeType::AST_ENUM_DECL);
    enum_decl->name = enum_name;
    setLocation(enum_decl, current_token_);
    
    EnumDefinition enum_def(enum_name);
    int64_t current_value = 0;  // デフォルトの開始値
    
    // 空のenumはエラー
    if (check(TokenType::TOK_RBRACE)) {
        error("Empty enum is not allowed");
        return nullptr;
    }
    
    // enumメンバーをパース
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        if (!check(TokenType::TOK_IDENTIFIER)) {
            error("Expected enum member name");
            return nullptr;
        }
        
        std::string member_name = current_token_.value;
        advance(); // consume member name
        
        bool explicit_value = false;
        int64_t member_value = current_value;
        
        // 明示的な値の指定をチェック
        if (match(TokenType::TOK_ASSIGN)) {
            // 負の数値をチェック
            bool is_negative = false;
            if (check(TokenType::TOK_MINUS)) {
                is_negative = true;
                advance(); // consume '-'
            }
            
            if (!check(TokenType::TOK_NUMBER)) {
                error("Expected number after '=' in enum member");
                return nullptr;
            }
            
            member_value = std::stoll(current_token_.value);
            if (is_negative) {
                member_value = -member_value;
            }
            explicit_value = true;
            current_value = member_value;
            advance(); // consume number
        }
        
        // enumメンバーを追加
        enum_def.add_member(member_name, member_value, explicit_value);
        current_value++; // 次の暗黙的な値を準備
        
        // カンマまたは }をチェック
        if (match(TokenType::TOK_COMMA)) {
            // 次のメンバーがある場合は続行
            if (check(TokenType::TOK_RBRACE)) {
                error("Trailing comma in enum is not allowed");
                return nullptr;
            }
        } else if (!check(TokenType::TOK_RBRACE)) {
            error("Expected ',' or '}' after enum member");
            return nullptr;
        }
    }
    
    consume(TokenType::TOK_RBRACE, "Expected '}' after enum members");
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after enum declaration");
    
    // 値の重複チェック
    if (enum_def.has_duplicate_values()) {
        error("Enum has duplicate values - this is not allowed");
        return nullptr;
    }
    
    // enum定義を保存
    enum_definitions_[enum_name] = enum_def;
    
    // ASTノードにenum定義情報を埋め込む
    enum_decl->enum_definition = enum_def;
    
    return enum_decl;
}

// interface宣言の解析: interface name { methods };
ASTNode* RecursiveParser::parseInterfaceDeclaration() {
    consume(TokenType::TOK_INTERFACE, "Expected 'interface'");
    
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected interface name");
        return nullptr;
    }
    
    std::string interface_name = current_token_.value;
    advance(); // interface名をスキップ
    
    consume(TokenType::TOK_LBRACE, "Expected '{' after interface name");
    
    InterfaceDefinition interface_def(interface_name);
    
    // メソッド宣言の解析
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        // メソッドの戻り値型を解析
        std::string return_type = parseType();
        ParsedTypeInfo return_parsed = getLastParsedTypeInfo();

        if (return_type.empty()) {
            error("Expected return type in interface method declaration");
            return nullptr;
        }
        
        // メソッド名を解析（予約キーワードも許可）
        std::string method_name;
        if (check(TokenType::TOK_IDENTIFIER)) {
            method_name = current_token_.value;
            advance();
        } else if (check(TokenType::TOK_PRINT) || check(TokenType::TOK_PRINTLN) || check(TokenType::TOK_PRINTF)) {
            // 予約キーワードだが、メソッド名として許可
            method_name = current_token_.value;
            advance();
        } else {
            error("Expected method name in interface declaration");
            return nullptr;
        }        // パラメータリストの解析
        consume(TokenType::TOK_LPAREN, "Expected '(' after method name");
        
        TypeInfo resolved_return_type = resolveParsedTypeInfo(return_parsed);
        if (resolved_return_type == TYPE_UNKNOWN) {
            resolved_return_type = getTypeInfoFromString(return_type);
        }

        InterfaceMember method(method_name, resolved_return_type,
                               return_parsed.is_unsigned);
        
        // パラメータの解析
        if (!check(TokenType::TOK_RPAREN)) {
            do {
                // パラメータの型
                std::string param_type = parseType();
                ParsedTypeInfo param_parsed = getLastParsedTypeInfo();
                if (param_type.empty()) {
                    error("Expected parameter type");
                    return nullptr;
                }
                
                // パラメータ名（オプション）
                std::string param_name = "";
                if (check(TokenType::TOK_IDENTIFIER)) {
                    param_name = current_token_.value;
                    advance();
                }
                
                TypeInfo param_type_info = resolveParsedTypeInfo(param_parsed);
                if (param_type_info == TYPE_UNKNOWN) {
                    param_type_info = getTypeInfoFromString(param_type);
                }

                method.add_parameter(param_name, param_type_info,
                                     param_parsed.is_unsigned);
                
                if (!check(TokenType::TOK_COMMA)) {
                    break;
                }
                advance(); // consume comma
            } while (!check(TokenType::TOK_RPAREN));
        }
        
        consume(TokenType::TOK_RPAREN, "Expected ')' after parameters");
        consume(TokenType::TOK_SEMICOLON, "Expected ';' after interface method declaration");
        
        interface_def.methods.push_back(method);
    }
    
    consume(TokenType::TOK_RBRACE, "Expected '}' after interface methods");
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after interface definition");
    
    // interface定義をパーサー内に保存
    interface_definitions_[interface_name] = interface_def;
    
    // ASTノードを作成
    ASTNode* node = new ASTNode(ASTNodeType::AST_INTERFACE_DECL);
    node->name = interface_name;
    setLocation(node, current_token_);
    
    // interface定義情報をASTノードに保存
    for (const auto& method : interface_def.methods) {
        ASTNode* method_node = new ASTNode(ASTNodeType::AST_FUNC_DECL);
        method_node->name = method.name;
        method_node->type_info = method.return_type;
            method_node->is_unsigned = method.return_is_unsigned;
            method_node->return_types.push_back(method.return_type);
        
        // パラメータ情報を保存
            for (size_t i = 0; i < method.parameters.size(); ++i) {
                const auto& param = method.parameters[i];
            ASTNode* param_node = new ASTNode(ASTNodeType::AST_PARAM_DECL);
            param_node->name = param.first;
            param_node->type_info = param.second;
                param_node->is_unsigned = method.get_parameter_is_unsigned(i);
            method_node->arguments.push_back(std::unique_ptr<ASTNode>(param_node));
        }
        
        node->arguments.push_back(std::unique_ptr<ASTNode>(method_node));
    }
    
    return node;
}

// impl宣言の解析: impl InterfaceName for StructName { methods }
ASTNode* RecursiveParser::parseImplDeclaration() {
    consume(TokenType::TOK_IMPL, "Expected 'impl'");
    
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected interface name after 'impl'");
        return nullptr;
    }
    
    std::string interface_name = current_token_.value;
    
    // ★ 課題1の解決: 存在しないinterfaceを実装しようとする場合のエラー検出
    if (interface_definitions_.find(interface_name) == interface_definitions_.end()) {
        error("Interface '" + interface_name + "' is not defined. Please declare the interface before implementing it.");
        return nullptr;
    }
    
    advance();
    
    // 'for' keyword
    if (!check(TokenType::TOK_FOR) && 
        !(check(TokenType::TOK_IDENTIFIER) && current_token_.value == "for")) {
        error("Expected 'for' after interface name in impl declaration");
        return nullptr;
    }
    advance(); // consume 'for'
    
    if (!check(TokenType::TOK_IDENTIFIER) && !check(TokenType::TOK_STRING_TYPE) && 
        !check(TokenType::TOK_INT) && !check(TokenType::TOK_LONG) && 
        !check(TokenType::TOK_SHORT) && !check(TokenType::TOK_TINY) && 
        !check(TokenType::TOK_BOOL) && !check(TokenType::TOK_CHAR_TYPE)) {
        error("Expected type name (struct or primitive type) after 'for'");
        return nullptr;
    }
    
    std::string struct_name;
    if (check(TokenType::TOK_STRING_TYPE)) {
        struct_name = "string";
    } else if (check(TokenType::TOK_INT)) {
        struct_name = "int";
    } else if (check(TokenType::TOK_LONG)) {
        struct_name = "long";
    } else if (check(TokenType::TOK_SHORT)) {
        struct_name = "short";
    } else if (check(TokenType::TOK_TINY)) {
        struct_name = "tiny";
    } else if (check(TokenType::TOK_BOOL)) {
        struct_name = "bool";
    } else if (check(TokenType::TOK_CHAR_TYPE)) {
        struct_name = "char";
    } else {
        struct_name = current_token_.value; // 識別子（構造体名またはtypedef名）
    }
    advance();
    
    // 生の配列型チェック - 配列記法が続く場合はエラー
    if (check(TokenType::TOK_LBRACKET)) {
        error("Cannot implement interface for raw array type '" + struct_name + "[...]'. Use typedef to define array type first.");
        return nullptr;
    }
    
    consume(TokenType::TOK_LBRACE, "Expected '{' after type name in impl declaration");
    
    ImplDefinition impl_def(interface_name, struct_name);
    std::vector<std::unique_ptr<ASTNode>> method_nodes; // 所有権は一時的に保持し、最終的にASTへ移動
    std::vector<std::unique_ptr<ASTNode>> static_var_nodes; // impl static変数
    
    // メソッド実装の解析
    while (!check(TokenType::TOK_RBRACE) && !isAtEnd()) {
        // static修飾子のチェック（impl staticの可能性）
        if (check(TokenType::TOK_STATIC)) {
            advance(); // consume 'static'
            
            // 次にconst修飾子が続く可能性もある
            bool is_const_static = false;
            if (check(TokenType::TOK_CONST)) {
                is_const_static = true;
                advance(); // consume 'const'
            }
            
            // 型名を取得
            std::string var_type = parseType();
            if (var_type.empty()) {
                error("Expected type after 'static' in impl block");
                return nullptr;
            }
            
            // 変数名を取得
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected variable name after type in impl static declaration");
                return nullptr;
            }
            std::string var_name = current_token_.value;
            advance();
            
            // 初期化式の解析（オプション）
            std::unique_ptr<ASTNode> init_expr;
            if (check(TokenType::TOK_ASSIGN)) {
                advance(); // consume '='
                ASTNode* expr_raw = parseExpression();
                if (!expr_raw) {
                    error("Expected expression after '=' in impl static variable initialization");
                    return nullptr;
                }
                init_expr.reset(expr_raw);
            }
            
            consume(TokenType::TOK_SEMICOLON, "Expected ';' after impl static variable declaration");
            
            // impl static変数ノードを作成
            ASTNode* static_var = new ASTNode(ASTNodeType::AST_VAR_DECL);
            static_var->name = var_name;
            static_var->type_name = var_type;
            static_var->type_info = getTypeInfoFromString(var_type);
            static_var->is_static = true;
            static_var->is_impl_static = true;
            static_var->is_const = is_const_static;
            if (init_expr) {
                static_var->init_expr = std::move(init_expr);
            }
            setLocation(static_var, current_token_);
            
            static_var_nodes.push_back(std::unique_ptr<ASTNode>(static_var));
            debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(), "impl_static_variable");
            
            continue; // 次の宣言へ
        }
        
        // private修飾子のチェック
        bool is_private_method = false;
        if (check(TokenType::TOK_PRIVATE)) {
            is_private_method = true;
            advance(); // consume 'private'
        }
        
        // メソッド実装をパース（関数宣言として）
        // impl内では戻り値の型から始まるメソッド定義
        std::string return_type = parseType();
        if (return_type.empty()) {
            error("Expected return type in method implementation");
            return nullptr;
        }
        
        // メソッド名を解析（予約キーワードも許可）
        std::string method_name;
        if (check(TokenType::TOK_IDENTIFIER)) {
            method_name = current_token_.value;
            advance();
        } else if (check(TokenType::TOK_PRINT) || check(TokenType::TOK_PRINTLN) || check(TokenType::TOK_PRINTF)) {
            // 予約キーワードだが、メソッド名として許可
            method_name = current_token_.value;
            advance();
        } else {
            error("Expected method name in method implementation");
            return nullptr;
        }
        
        // 関数宣言として解析
        ASTNode* method_impl_raw = parseFunctionDeclarationAfterName(return_type, method_name);
        if (method_impl_raw) {
            std::unique_ptr<ASTNode> method_impl(method_impl_raw);
            debug_print("[IMPL_PARSE] After method '%s', current token = %s (type %d)\n",
                        method_name.c_str(), current_token_.value.c_str(), (int)current_token_.type);
            // privateフラグを設定
            method_impl->is_private_method = is_private_method;
            // privateメソッドの場合はinterface署名チェックをスキップ
            if (!is_private_method) {
                // ★ 課題2の解決: メソッド署名の不一致の検出
                auto interface_it = interface_definitions_.find(interface_name);
                if (interface_it != interface_definitions_.end()) {
                    bool method_found = false;
                    for (const auto& interface_method : interface_it->second.methods) {
                        if (interface_method.name == method_name) {
                            method_found = true;
                            auto format_type = [](TypeInfo type, bool is_unsigned_flag) {
                                std::string base = type_info_to_string(type);
                                if (is_unsigned_flag) {
                                    return std::string("unsigned ") + base;
                                }
                                return base;
                            };

                            TypeInfo expected_return_type_info = interface_method.return_type;
                            bool expected_return_unsigned = interface_method.return_is_unsigned;

                            TypeInfo actual_return_type_info = TYPE_UNKNOWN;
                            if (!method_impl->return_types.empty()) {
                                actual_return_type_info = method_impl->return_types[0];
                            } else {
                                actual_return_type_info = getTypeInfoFromString(return_type);
                            }
                            bool actual_return_unsigned = method_impl->is_unsigned;

                            if (expected_return_type_info != actual_return_type_info ||
                                expected_return_unsigned != actual_return_unsigned) {
                                error("Method signature mismatch: Expected return type '" +
                                      format_type(expected_return_type_info, expected_return_unsigned) +
                                      "' but got '" +
                                      format_type(actual_return_type_info, actual_return_unsigned) +
                                      "' for method '" + method_name + "'");
                                return nullptr;
                            }
                            // 引数の数をチェック
                            if (interface_method.parameters.size() != method_impl->parameters.size()) {
                                error("Method signature mismatch: Expected " + 
                                      std::to_string(interface_method.parameters.size()) + 
                                      " parameter(s) but got " + 
                                      std::to_string(method_impl->parameters.size()) + 
                                      " for method '" + method_name + "'");
                                return nullptr;
                            }
                            // 引数の型をチエック
                            for (size_t i = 0; i < interface_method.parameters.size(); ++i) {
                                TypeInfo expected_param_type = interface_method.parameters[i].second;
                                bool expected_param_unsigned = interface_method.get_parameter_is_unsigned(i);
                                TypeInfo actual_param_type = method_impl->parameters[i]->type_info;
                                bool actual_param_unsigned = method_impl->parameters[i]->is_unsigned;

                                if (expected_param_type != actual_param_type ||
                                    expected_param_unsigned != actual_param_unsigned) {
                                    error("Method signature mismatch: Parameter " + std::to_string(i + 1) + 
                                          " expected type '" + format_type(expected_param_type, expected_param_unsigned) +
                                          "' but got '" + format_type(actual_param_type, actual_param_unsigned) +
                                          "' for method '" + method_name + "'");
                                    return nullptr;
                                }
                            }
                            break;
                        }
                    }
                    if (!method_found) {
                        // 警告: interfaceに定義されていないメソッドが実装されている
                        std::cerr << "[WARNING] Method '" << method_name << "' is implemented but not declared in interface '" << interface_name << "'" << std::endl;
                    }
                }
            }
            
            // メソッド情報を保存（ImplDefinitionにはポインタのみ保持）
            impl_def.add_method(method_impl.get());
            method_nodes.push_back(std::move(method_impl));
            debug_msg(DebugMsgId::PARSE_VAR_DECL, method_name.c_str(), "impl_method");
        }
    }
    
    consume(TokenType::TOK_RBRACE, "Expected '}' after impl methods");
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after impl declaration");
    
    // ★ interfaceの全メソッドが実装されているかチェック
    auto interface_it = interface_definitions_.find(interface_name);
    if (interface_it != interface_definitions_.end()) {
        for (const auto& interface_method : interface_it->second.methods) {
            bool implemented = false;
            for (const auto* impl_method : impl_def.methods) {
                if (impl_method->name == interface_method.name) {
                    implemented = true;
                    break;
                }
            }
            if (!implemented) {
                error("Incomplete implementation: Method '" + interface_method.name + 
                      "' declared in interface '" + interface_name + "' is not implemented");
                return nullptr;
            }
        }
    }
    
    // ★ 課題3の解決: 重複impl定義の検出
    for (const auto& existing_impl : impl_definitions_) {
        if (existing_impl.interface_name == interface_name && 
            existing_impl.struct_name == struct_name) {
            error("Duplicate implementation: Interface '" + interface_name + 
                  "' is already implemented for struct '" + struct_name + "'");
            return nullptr;
        }
    }
    
    // impl定義を保存（ポインタ参照のみ保持）
    impl_definitions_.push_back(impl_def);
    
    // ASTノードを作成
    ASTNode* node = new ASTNode(ASTNodeType::AST_IMPL_DECL);
    node->name = interface_name + "_for_" + struct_name;
    node->type_name = struct_name; // struct名を保存
    node->interface_name = interface_name; // interface名を保存
    node->struct_name = struct_name; // struct名を明示的に保存
    setLocation(node, current_token_);
    
    // impl static変数の所有権をASTノードに移動
    for (auto& static_var : static_var_nodes) {
        node->impl_static_variables.push_back(std::move(static_var));
    }
    
    // implメソッドの所有権をASTノードに移動
    for (auto& method_node : method_nodes) {
        node->arguments.push_back(std::move(method_node));
    }
    
    return node;
}

// 循環参照検出: 値メンバーによる循環を検出（ポインタメンバーは除外）
bool RecursiveParser::detectCircularReference(const std::string& struct_name, 
                                             const std::string& member_type,
                                             std::unordered_set<std::string>& visited,
                                             std::vector<std::string>& path) {
    // 型名を正規化（"struct " プレフィックスを除去）
    std::string normalized_type = member_type;
    if (normalized_type.rfind("struct ", 0) == 0) {
        normalized_type = normalized_type.substr(7);
    }
    
    // 構造体型でなければ循環なし
    if (struct_definitions_.find(normalized_type) == struct_definitions_.end()) {
        return false;
    }
    
    // 前方宣言のみの構造体はスキップ（定義が後で来る可能性がある）
    const StructDefinition& struct_def = struct_definitions_[normalized_type];
    if (struct_def.is_forward_declaration) {
        return false;
    }
    
    // 開始構造体に戻ってきたら循環検出
    if (normalized_type == struct_name) {
        path.push_back(normalized_type);
        return true;
    }
    
    // 既に訪問済みなら循環なし（異なる構造体への循環）
    if (visited.find(normalized_type) != visited.end()) {
        return false;
    }
    
    // 訪問マーク
    visited.insert(normalized_type);
    path.push_back(normalized_type);
    for (const auto& member : struct_def.members) {
        // ポインタメンバーはスキップ（メモリ発散しない）
        if (member.is_pointer) {
            continue;
        }
        
        // 配列メンバーはスキップ（固定サイズなので発散しない）
        if (member.array_info.is_array()) {
            continue;
        }
        
        // 値メンバーの型を再帰的にチェック
        std::string member_base_type = member.type_alias;
        if (member_base_type.empty()) {
            // TypeInfoから型名を復元（構造体の場合）
            if (member.type == TYPE_STRUCT) {
                // struct_type_nameまたはpointer_base_type_nameから取得
                member_base_type = member.pointer_base_type_name;
                if (member_base_type.empty()) {
                    continue;
                }
            } else {
                continue; // プリミティブ型はスキップ
            }
        }
        
        if (detectCircularReference(struct_name, member_base_type, visited, path)) {
            return true;
        }
    }
    
    // バックトラック
    path.pop_back();
    visited.erase(normalized_type);
    
    return false;
}

// 関数ポインタtypedef構文かどうかをチェック
// typedef <return_type> (*<name>)(<param_types>);
bool RecursiveParser::isFunctionPointerTypedef() {
    // 簡単なチェック：次のトークンの種類で判断
    // 型名の後に '(' が来たら関数ポインタtypedefの可能性
    // 型名の後に識別子が来たら通常のtypedef
    
    // 現在のトークンが型名かチェック
    bool is_type_token = (
        current_token_.type == TokenType::TOK_VOID ||
        current_token_.type == TokenType::TOK_INT ||
        current_token_.type == TokenType::TOK_LONG ||
        current_token_.type == TokenType::TOK_SHORT ||
        current_token_.type == TokenType::TOK_TINY ||
        current_token_.type == TokenType::TOK_CHAR ||
        current_token_.type == TokenType::TOK_STRING_TYPE ||
        current_token_.type == TokenType::TOK_BOOL ||
        current_token_.type == TokenType::TOK_FLOAT ||
        current_token_.type == TokenType::TOK_DOUBLE ||
        current_token_.type == TokenType::TOK_BIG ||
        current_token_.type == TokenType::TOK_QUAD ||
        current_token_.type == TokenType::TOK_IDENTIFIER
    );
    
    if (!is_type_token) {
        return false;
    }
    
    // レキサーから次のトークンを取得（先読み）
    Token next_token = lexer_.peekToken();
    
    // 型名の次が '(' なら関数ポインタtypedef
    return (next_token.type == TokenType::TOK_LPAREN);
}

// 関数ポインタtypedef宣言の解析
// typedef <return_type> (*<name>)(<param_types>);
ASTNode* RecursiveParser::parseFunctionPointerTypedefDeclaration() {
    // 戻り値型の解析
    std::string return_type_str = parseType();
    TypeInfo return_type = getTypeInfoFromString(return_type_str);
    
    // '(' の消費
    consume(TokenType::TOK_LPAREN, "Expected '(' in function pointer typedef");
    
    // '*' の消費
    consume(TokenType::TOK_MUL, "Expected '*' in function pointer typedef");
    
    // 関数ポインタ型名の取得
    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected identifier in function pointer typedef");
        return nullptr;
    }
    std::string typedef_name = current_token_.value;
    advance();
    
    // ')' の消費
    consume(TokenType::TOK_RPAREN, "Expected ')' after function pointer name");
    
    // '(' の消費（パラメータリスト開始）
    consume(TokenType::TOK_LPAREN, "Expected '(' for parameter list");
    
    // パラメータリストの解析
    std::vector<TypeInfo> param_types;
    std::vector<std::string> param_type_names;
    std::vector<std::string> param_names;
    
    if (!check(TokenType::TOK_RPAREN)) {
        do {
            // パラメータ型の解析
            std::string param_type_str = parseType();
            TypeInfo param_type = getTypeInfoFromString(param_type_str);
            param_types.push_back(param_type);
            param_type_names.push_back(param_type_str);
            
            // パラメータ名は省略可能
            if (check(TokenType::TOK_IDENTIFIER)) {
                param_names.push_back(current_token_.value);
                advance();
            } else {
                param_names.push_back(""); // 匿名パラメータ
            }
            
            if (check(TokenType::TOK_COMMA)) {
                advance();
            } else {
                break;
            }
        } while (true);
    }
    
    // ')' の消費（パラメータリスト終了）
    consume(TokenType::TOK_RPAREN, "Expected ')' after parameter list");
    
    // ';' の消費
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after function pointer typedef");
    
    // FunctionPointerTypeInfo の作成
    FunctionPointerTypeInfo fp_type_info(return_type, return_type_str, 
                                          param_types, param_type_names, param_names);
    
    // 関数ポインタtypedefマップに登録
    function_pointer_typedefs_[typedef_name] = fp_type_info;
    
    // typedef マップにも登録（型名として認識させる）
    typedef_map_[typedef_name] = "function_pointer:" + typedef_name;
    
    // ASTノードの作成
    ASTNode* node = new ASTNode(ASTNodeType::AST_FUNCTION_POINTER_TYPEDEF);
    node->name = typedef_name;
    node->type_info = TYPE_FUNCTION_POINTER;
    node->is_function_pointer = true;
    node->function_pointer_type = fp_type_info;
    
    setLocation(node, current_token_);
    
    return node;
}

