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

DeclarationParser::DeclarationParser(RecursiveParser* parser) 
    : parser_(parser) {
}

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
ASTNode* DeclarationParser::parseVariableDeclaration() {
    return parser_->parseVariableDeclaration();
}

/**
 * @brief typedef付き変数宣言を解析
 * @return 解析されたAST変数宣言ノード
 * 
 * 例: MyInt x = 10; (MyIntはtypedef)
 */
ASTNode* DeclarationParser::parseTypedefVariableDeclaration() {
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
            if (parser_->union_definitions_.find(resolved_type) != parser_->union_definitions_.end()) {
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
        std::string base_type = resolved_type.substr(0, resolved_type.find("["));
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
        if (parser_->debug_mode_) {
            std::cerr << "DEBUG: Parser setting array_type_info for " << node->name 
                      << " with base_type=" << node->array_type_info.base_type << std::endl;
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
            node->init_expr = std::unique_ptr<ASTNode>(parser_->parseStructLiteral());
        } else if (parser_->check(TokenType::TOK_LBRACKET)) {
            // 配列リテラル初期化
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
            node->init_expr = std::unique_ptr<ASTNode>(array_literal);
        } else {
            // 通常の式
            ASTNode* expr = parser_->parseExpression();
            node->init_expr = std::unique_ptr<ASTNode>(expr);
        }
    }
    
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after variable declaration");
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
ASTNode* DeclarationParser::parseFunctionDeclaration() {
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
    parser_->consume(TokenType::TOK_LBRACE, "Expected '{' to start function body");
    
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
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        ASTNode* stmt = parser_->parseStatement();
        if (stmt != nullptr) {
            body_node->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }
    
    // 関数本体の終了 '}'
    parser_->consume(TokenType::TOK_RBRACE, "Expected '}' to end function body");
    
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
ASTNode* DeclarationParser::parseFunctionDeclarationAfterName(
    const std::string& return_type,
    const std::string& function_name
) {
    return parser_->parseFunctionDeclarationAfterName(return_type, function_name);
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
ASTNode* DeclarationParser::parseTypedefDeclaration() {
    return parser_->parseTypedefDeclaration();
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
ASTNode* DeclarationParser::parseFunctionPointerTypedefDeclaration() {
    // 戻り値型の解析
    std::string return_type_str = parser_->parseType();
    TypeInfo return_type = parser_->getTypeInfoFromString(return_type_str);
    
    // '(' の消費
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' in function pointer typedef");
    
    // '*' の消費
    parser_->consume(TokenType::TOK_MUL, "Expected '*' in function pointer typedef");
    
    // 関数ポインタ型名の取得
    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected identifier in function pointer typedef");
        return nullptr;
    }
    std::string typedef_name = parser_->current_token_.value;
    parser_->advance();
    
    // ')' の消費
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after function pointer name");
    
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
            TypeInfo param_type = parser_->getTypeInfoFromString(param_type_str);
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
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after parameter list");
    
    // ';' の消費
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after function pointer typedef");
    
    // FunctionPointerTypeInfo の作成
    FunctionPointerTypeInfo fp_type_info(return_type, return_type_str, 
                                          param_types, param_type_names, param_names);
    
    // 関数ポインタtypedefマップに登録
    parser_->function_pointer_typedefs_[typedef_name] = fp_type_info;
    
    // typedef マップにも登録（型名として認識させる）
    parser_->typedef_map_[typedef_name] = "function_pointer:" + typedef_name;
    
    // ASTノードの作成
    ASTNode* node = new ASTNode(ASTNodeType::AST_FUNCTION_POINTER_TYPEDEF);
    node->name = typedef_name;
    node->type_info = TYPE_FUNCTION_POINTER;
    node->is_function_pointer = true;
    node->function_pointer_type = fp_type_info;
    
    parser_->setLocation(node, parser_->current_token_);
    
    return node;
}
