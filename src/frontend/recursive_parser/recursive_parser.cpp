#include "recursive_parser.h"
#include "../../backend/interpreter/core/error_handler.h"
#include "../../backend/interpreter/evaluator/functions/generic_instantiation.h"
#include "../../common/debug.h"
#include "../../common/debug_messages.h"
#include "parsers/declaration_parser.h"
#include "parsers/enum_parser.h"
#include "parsers/expression_parser.h"
#include "parsers/interface_parser.h"
#include "parsers/statement_parser.h"
#include "parsers/struct_parser.h"
#include "parsers/type_parser.h"
#include "parsers/type_utility_parser.h"
#include "parsers/union_parser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_set>

// 外部関数宣言
extern const char *type_info_to_string(TypeInfo type);

using namespace RecursiveParserNS;

RecursiveParser::RecursiveParser(const std::string &source,
                                 const std::string &filename)
    : lexer_(source), current_token_(TokenType::TOK_EOF, "", 0, 0),
      filename_(filename), source_(source), debug_mode_(false),
      has_split_gt_token_(false),
      split_gt_token_(TokenType::TOK_EOF, "", 0, 0) {
    // ソースコードを行ごとに分割
    std::istringstream iss(source);
    std::string line;
    while (std::getline(iss, line)) {
        source_lines_.push_back(line);
    }

    // impl定義用のメモリを事前確保（リサイズによるポインタ無効化を防ぐ）
    impl_definitions_.reserve(100);

    // 分離されたパーサーのインスタンスを初期化
    // Phase 2: 全パーサーの有効化（委譲パターン）
    expression_parser_ = std::make_unique<ExpressionParser>(this);
    statement_parser_ = std::make_unique<StatementParser>(this);
    declaration_parser_ = std::make_unique<DeclarationParser>(this);
    type_parser_ = std::make_unique<TypeParser>(this);
    struct_parser_ = std::make_unique<StructParser>(this);
    enum_parser_ = std::make_unique<EnumParser>(this);
    interface_parser_ = std::make_unique<InterfaceParser>(this);
    union_parser_ = std::make_unique<UnionParser>(this);
    type_utility_parser_ = std::make_unique<TypeUtilityParser>(this);

    // v0.11.0: 組み込み型（Option<T>, Result<T, E>）の定義を登録
    initialize_builtin_types();

    advance();
}

// デストラクタ - unique_ptrの不完全型対応のため、実装ファイルで定義
RecursiveParser::~RecursiveParser() = default;

ASTNode *RecursiveParser::parse() { return parseProgram(); }

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

    // v0.11.0: >> トークン分割処理（ネストしたジェネリクス対応）
    // 分割された > トークンがある場合、それを返す
    if (has_split_gt_token_) {
        current_token_ = split_gt_token_;
        has_split_gt_token_ = false;
        return previous;
    }

    current_token_ = lexer_.nextToken();

    // >> トークンをジェネリクスのコンテキストで2つの > に分割
    // type_parameter_stack_ が空でない = ジェネリクス型のパース中
    if (current_token_.type == TokenType::TOK_RIGHT_SHIFT &&
        !type_parameter_stack_.empty()) {
        // >> を最初の > として扱い、2番目の > を保存
        Token first_gt(TokenType::TOK_GT, ">", current_token_.line,
                       current_token_.column);
        split_gt_token_ = Token(TokenType::TOK_GT, ">", current_token_.line,
                                current_token_.column + 1);
        has_split_gt_token_ = true;
        current_token_ = first_gt;
    }

    return previous;
}

Token RecursiveParser::peek() { return current_token_; }

bool RecursiveParser::isAtEnd() {
    return current_token_.type == TokenType::TOK_EOF;
}

void RecursiveParser::consume(TokenType type, const std::string &message) {
    if (check(type)) {
        advance();
        return;
    }
    error(message);
}

void RecursiveParser::error(const std::string &message) {
    // 詳細なエラー表示
    std::string source_line = getSourceLine(current_token_.line);
    print_error_with_location(message, filename_, current_token_.line,
                              current_token_.column, source_line);

    throw DetailedErrorException(message);
}

ASTNode *RecursiveParser::parseProgram() {
    debug_msg(DebugMsgId::PARSE_PROGRAM_START, filename_.c_str());
    ASTNode *program = new ASTNode(ASTNodeType::AST_STMT_LIST);

    while (!isAtEnd()) {
        debug_msg(DebugMsgId::PARSE_STATEMENT_START, current_token_.line,
                  current_token_.column);
        ASTNode *stmt = parseStatement();
        if (stmt != nullptr) {
            debug_msg(DebugMsgId::PARSE_STATEMENT_SUCCESS,
                      std::to_string((int)stmt->node_type).c_str(),
                      stmt->name.c_str());
            program->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }

    debug_msg(DebugMsgId::PARSE_PROGRAM_COMPLETE, program->statements.size());
    return program;
}

ASTNode *RecursiveParser::parseStatement() {
    return statement_parser_->parseStatement();
}

ASTNode *RecursiveParser::parseVariableDeclaration() {
    return declaration_parser_->parseVariableDeclaration();
}

std::string RecursiveParser::parseType() {
    return type_utility_parser_->parseType();
}

TypeInfo
RecursiveParser::resolveParsedTypeInfo(const ParsedTypeInfo &parsed) const {
    TypeInfo resolved = parsed.base_type_info;

    std::string base_lookup = parsed.base_type;
    if (base_lookup.rfind("struct ", 0) == 0) {
        base_lookup = base_lookup.substr(7);
    }

    if (resolved == TYPE_UNKNOWN && !base_lookup.empty()) {
        if (struct_definitions_.find(base_lookup) !=
            struct_definitions_.end()) {
            resolved = TYPE_STRUCT;
        } else if (enum_definitions_.find(base_lookup) !=
                   enum_definitions_.end()) {
            resolved = TYPE_ENUM;
        } else if (interface_definitions_.find(base_lookup) !=
                   interface_definitions_.end()) {
            resolved = TYPE_INTERFACE;
        } else if (union_definitions_.find(base_lookup) !=
                   union_definitions_.end()) {
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

ASTNode *RecursiveParser::cloneAstNode(const ASTNode *node) {
    if (!node) {
        return nullptr;
    }

    ASTNode *clone = new ASTNode(node->node_type);
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
    clone->original_type_name = node->original_type_name;
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
    for (const auto &child : node->children) {
        clone->children.push_back(
            std::unique_ptr<ASTNode>(cloneAstNode(child.get())));
    }

    clone->parameters.reserve(node->parameters.size());
    for (const auto &param : node->parameters) {
        clone->parameters.push_back(
            std::unique_ptr<ASTNode>(cloneAstNode(param.get())));
    }

    clone->arguments.reserve(node->arguments.size());
    for (const auto &arg : node->arguments) {
        clone->arguments.push_back(
            std::unique_ptr<ASTNode>(cloneAstNode(arg.get())));
    }

    clone->statements.reserve(node->statements.size());
    for (const auto &stmt : node->statements) {
        clone->statements.push_back(
            std::unique_ptr<ASTNode>(cloneAstNode(stmt.get())));
    }

    clone->array_dimensions.reserve(node->array_dimensions.size());
    for (const auto &dim : node->array_dimensions) {
        clone->array_dimensions.push_back(
            std::unique_ptr<ASTNode>(cloneAstNode(dim.get())));
    }

    clone->array_indices.reserve(node->array_indices.size());
    for (const auto &idx : node->array_indices) {
        clone->array_indices.push_back(
            std::unique_ptr<ASTNode>(cloneAstNode(idx.get())));
    }

    return clone;
}

ASTNode *RecursiveParser::parseExpression() {
    return expression_parser_->parseExpression();
}

ASTNode *RecursiveParser::parseAssignment() {
    return expression_parser_->parseAssignment();
}

ASTNode *RecursiveParser::parseTernary() {
    ASTNode *condition = parseLogicalOr();

    if (check(TokenType::TOK_QUESTION)) {
        advance(); // consume '?'
        ASTNode *true_expr = parseExpression();
        consume(TokenType::TOK_COLON, "Expected ':' in ternary expression");
        ASTNode *false_expr = parseTernary();

        ASTNode *ternary = new ASTNode(ASTNodeType::AST_TERNARY_OP);
        ternary->left = std::unique_ptr<ASTNode>(condition);
        ternary->right = std::unique_ptr<ASTNode>(true_expr);
        ternary->third = std::unique_ptr<ASTNode>(false_expr);

        return ternary;
    }

    return condition;
}

ASTNode *RecursiveParser::parseLogicalOr() {
    return expression_parser_->parseLogicalOr();
}

ASTNode *RecursiveParser::parseLogicalAnd() {
    return expression_parser_->parseLogicalAnd();
}

ASTNode *RecursiveParser::parseBitwiseOr() {
    return expression_parser_->parseBitwiseOr();
}

ASTNode *RecursiveParser::parseBitwiseXor() {
    return expression_parser_->parseBitwiseXor();
}

ASTNode *RecursiveParser::parseBitwiseAnd() {
    return expression_parser_->parseBitwiseAnd();
}

ASTNode *RecursiveParser::parseComparison() {
    return expression_parser_->parseComparison();
}

ASTNode *RecursiveParser::parseShift() {
    return expression_parser_->parseShift();
}

ASTNode *RecursiveParser::parseAdditive() {
    return expression_parser_->parseAdditive();
}

ASTNode *RecursiveParser::parseMultiplicative() {
    return expression_parser_->parseMultiplicative();
}

ASTNode *RecursiveParser::parseUnary() {
    return expression_parser_->parseUnary();
}

ASTNode *RecursiveParser::parsePostfix() {
    return expression_parser_->parsePostfix();
}

ASTNode *RecursiveParser::parsePrimary() {
    return expression_parser_->parsePrimary();
}

ASTNode *RecursiveParser::parseFunctionDeclarationAfterName(
    const std::string &return_type, const std::string &function_name) {
    return declaration_parser_->parseFunctionDeclarationAfterName(
        return_type, function_name);
}

ASTNode *RecursiveParser::parseFunctionDeclaration() {
    return declaration_parser_->parseFunctionDeclaration();
}

ASTNode *RecursiveParser::parseTypedefDeclaration() {
    return declaration_parser_->parseTypedefDeclaration();
}

TypeInfo RecursiveParser::getTypeInfoFromString(const std::string &type_name) {
    return type_utility_parser_->getTypeInfoFromString(type_name);
}

ASTNode *RecursiveParser::parseReturnStatement() {
    return statement_parser_->parseReturnStatement();
}

ASTNode *RecursiveParser::parseAssertStatement() {
    return statement_parser_->parseAssertStatement();
}

ASTNode *RecursiveParser::parseBreakStatement() {
    return statement_parser_->parseBreakStatement();
}

ASTNode *RecursiveParser::parseContinueStatement() {
    return statement_parser_->parseContinueStatement();
}

ASTNode *RecursiveParser::parseIfStatement() {
    return statement_parser_->parseIfStatement();
}

ASTNode *RecursiveParser::parseForStatement() {
    return statement_parser_->parseForStatement();
}

ASTNode *RecursiveParser::parseWhileStatement() {
    return statement_parser_->parseWhileStatement();
}

ASTNode *RecursiveParser::parseCompoundStatement() {
    return statement_parser_->parseCompoundStatement();
}

ASTNode *RecursiveParser::parsePrintlnStatement() {
    return statement_parser_->parsePrintlnStatement();
}

ASTNode *RecursiveParser::parsePrintStatement() {
    return statement_parser_->parsePrintStatement();
}

// 位置情報設定のヘルパーメソッド
void RecursiveParser::setLocation(ASTNode *node, const Token &token) {
    if (node) {
        node->location.filename = filename_;
        node->location.line = token.line;
        node->location.column = token.column;
        node->location.source_line = getSourceLine(token.line);
    }
}

void RecursiveParser::setLocation(ASTNode *node, int line, int column) {
    if (node) {
        node->location.filename = filename_;
        node->location.line = line;
        node->location.column = column;
        node->location.source_line = getSourceLine(line);
    }
}

std::string RecursiveParser::getSourceLine(int line_number) {
    // 1-based line numberを0-basedインデックスに変換
    if (line_number < 1 ||
        line_number > static_cast<int>(source_lines_.size())) {
        return "";
    }
    return source_lines_[line_number - 1];
}

// typedef型のチェーンを解決する
std::string
RecursiveParser::resolveTypedefChain(const std::string &typedef_name) {
    return type_utility_parser_->resolveTypedefChain(typedef_name);
}

ASTNode *RecursiveParser::parseTypedefVariableDeclaration() {
    return declaration_parser_->parseTypedefVariableDeclaration();
}

std::string RecursiveParser::extractBaseType(const std::string &type_name) {
    return type_utility_parser_->extractBaseType(type_name);
}

// DELETED: 10 lines moved to type_utility_parser.cpp

// struct宣言の解析: struct name { members };
// v0.11.0: ジェネリクス対応 struct Box<T> { T value; };
ASTNode *RecursiveParser::parseStructDeclaration() {
    consume(TokenType::TOK_STRUCT, "Expected 'struct'");

    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected struct name");
        return nullptr;
    }

    std::string struct_name = current_token_.value;
    advance(); // struct名をスキップ

    // v0.11.0: 型パラメータリストのチェック <T> または <T, E>
    // v0.11.0 Phase 1a: 複数インターフェース境界のサポート <T, A: Allocator +
    // Clone>
    std::vector<std::string> type_parameters;
    std::unordered_map<std::string, std::vector<std::string>> interface_bounds;
    bool is_generic = false;

    if (check(TokenType::TOK_LT)) {
        is_generic = true;
        advance(); // '<' を消費

        // 型パラメータのリストを解析
        do {
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected type parameter name after '<'");
                return nullptr;
            }

            std::string param_name = current_token_.value;
            type_parameters.push_back(param_name);
            advance();

            // インターフェース境界のチェック: A: Allocator または A: Allocator
            // + Clone + Debug
            if (check(TokenType::TOK_COLON)) {
                advance(); // ':' を消費

                std::vector<std::string> bounds;
                do {
                    if (!check(TokenType::TOK_IDENTIFIER)) {
                        error("Expected interface name after ':' or '+' in "
                              "type parameter "
                              "bound");
                        return nullptr;
                    }

                    bounds.push_back(current_token_.value);
                    advance();

                    // '+' があれば次のインターフェース境界を読む
                    if (check(TokenType::TOK_PLUS)) {
                        advance(); // '+' を消費
                    } else {
                        break;
                    }
                } while (true);

                interface_bounds[param_name] = bounds;
            }

            if (check(TokenType::TOK_COMMA)) {
                advance(); // ',' を消費
            } else {
                break;
            }
        } while (true);

        if (!check(TokenType::TOK_GT)) {
            error("Expected '>' after type parameters");
            return nullptr;
        }
        advance(); // '>' を消費
    }

    // 前方宣言のチェック: struct Name; の形式
    // ジェネリクス構造体の前方宣言もサポート: struct Box<T>;
    if (check(TokenType::TOK_SEMICOLON)) {
        advance(); // セミコロンを消費

        // 前方宣言として登録（既に定義済みでなければ）
        if (struct_definitions_.find(struct_name) ==
            struct_definitions_.end()) {
            StructDefinition forward_decl(struct_name);
            forward_decl.is_forward_declaration = true;
            forward_decl.is_generic = is_generic;
            forward_decl.type_parameters = type_parameters;
            struct_definitions_[struct_name] = forward_decl;

            debug_msg(DebugMsgId::PARSE_STRUCT_DEF,
                      ("Forward declaration: " + struct_name).c_str());
        }

        // 前方宣言用のASTノードを作成
        ASTNode *node =
            new ASTNode(is_generic ? ASTNodeType::AST_GENERIC_STRUCT_DECL
                                   : ASTNodeType::AST_STRUCT_DECL);
        node->name = struct_name;
        node->is_generic = is_generic;
        node->type_parameters = type_parameters;
        node->interface_bounds = interface_bounds;
        setLocation(node, current_token_);
        return node;
    }

    consume(TokenType::TOK_LBRACE, "Expected '{' after struct name");

    StructDefinition struct_def(struct_name);
    struct_def.is_generic = is_generic;
    struct_def.type_parameters = type_parameters;
    struct_def.interface_bounds = interface_bounds;

    // v0.11.0: ジェネリック構造体の場合、型パラメータをスタックにプッシュ
    // メンバーの型解析時に型パラメータを認識できるようにする
    if (is_generic) {
        type_parameter_stack_.push_back(type_parameters);
    }

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

        // default修飾子のチェック
        bool is_default_member = false;
        if (check(TokenType::TOK_DEFAULT)) {
            is_default_member = true;
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

            // 自己再帰構造体チェック:
            // 自分自身の型のメンバーはポインタでなければならない
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

            struct_def.add_member(
                member_name, member_type_info, var_parsed.full_type,
                var_parsed.is_pointer, var_parsed.pointer_depth,
                var_parsed.base_type, var_parsed.base_type_info,
                is_private_member, var_parsed.is_reference,
                var_parsed.is_unsigned, is_const_member);

            if (var_parsed.is_array) {
                StructMember &added = struct_def.members.back();
                added.array_info = var_parsed.array_info;
            }

            // デフォルトメンバーの設定
            if (is_default_member) {
                StructMember &added = struct_def.members.back();
                added.is_default = true;
            }

            // 旧式の配列宣言をチェック（int data[2][2];）- エラーとして処理
            if (check(TokenType::TOK_LBRACKET)) {
                error("Old-style array declaration is not supported in struct "
                      "members. Use 'int[2][2] member_name;' instead of 'int "
                      "member_name[2][2];'");
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

    // v0.11.0: ジェネリック構造体の型パラメータをスタックからポップ
    if (is_generic) {
        type_parameter_stack_.pop_back();
    }

    // デフォルトメンバーの検証: 複数のdefaultメンバーがないか確認
    int default_count = 0;
    std::string default_member;
    for (const auto &member : struct_def.members) {
        if (member.is_default) {
            default_count++;
            default_member = member.name;
            if (default_count > 1) {
                error("Struct '" + struct_name +
                      "' has multiple default members. Only one default member "
                      "is allowed per struct.");
                return nullptr;
            }
        }
    }
    if (default_count == 1) {
        struct_def.has_default_member = true;
        struct_def.default_member_name = default_member;

        if (debug_mode) {
            std::cerr << "[PARSER] Struct " << struct_name
                      << " has default member: " << default_member << std::endl;
        }
    }

    // struct定義をパーサー内に保存（変数宣言の認識のため）
    // 前方宣言を完全な定義で上書き
    struct_def.is_forward_declaration = false;
    struct_definitions_[struct_name] = struct_def;

    // 循環参照チェック: 値メンバーによる循環を検出
    for (const auto &member : struct_def.members) {
        // ポインタメンバーと配列メンバーはスキップ
        if (member.is_pointer || member.array_info.is_array()) {
            continue;
        }

        std::unordered_set<std::string> visited;
        std::vector<std::string> path;
        path.push_back(struct_name);

        if (detectCircularReference(struct_name, member.type_alias, visited,
                                    path)) {
            std::string cycle_path = struct_name;
            for (size_t i = 1; i < path.size(); ++i) {
                cycle_path += " -> " + path[i];
            }
            error("Circular reference detected in struct value members: " +
                  cycle_path + ". Use pointers to break the cycle.");
            return nullptr;
        }
    }

    debug_msg(DebugMsgId::PARSE_STRUCT_DEF, struct_name.c_str());

    // ASTノードを作成してstruct定義情報を保存
    ASTNode *node =
        new ASTNode(is_generic ? ASTNodeType::AST_GENERIC_STRUCT_DECL
                               : ASTNodeType::AST_STRUCT_DECL);
    node->name = struct_name;
    node->is_generic = is_generic;
    node->type_parameters = type_parameters;
    node->interface_bounds = interface_bounds;
    setLocation(node, current_token_);

    // struct定義情報をASTノードに保存
    for (const auto &member : struct_def.members) {
        ASTNode *member_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
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
        member_node->is_default_member = member.is_default;
        member_node->array_type_info = member.array_info; // 配列情報をコピー
        if (member.array_info.is_array()) {
            member_node->is_array = true;
        }
        node->arguments.push_back(std::unique_ptr<ASTNode>(member_node));
    }

    return node;
}

// typedef struct宣言の解析: typedef struct { members } alias;
ASTNode *RecursiveParser::parseStructTypedefDeclaration() {
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

            struct_def.add_member(
                member_name, member_type_info, var_parsed.full_type,
                var_parsed.is_pointer, var_parsed.pointer_depth,
                var_parsed.base_type, var_parsed.base_type_info,
                is_private_member, var_parsed.is_reference,
                var_parsed.is_unsigned, is_const_member);

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

    consume(TokenType::TOK_SEMICOLON,
            "Expected ';' after typedef struct declaration");

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
    for (const auto &member : struct_def.members) {
        // ポインタメンバーと配列メンバーはスキップ
        if (member.is_pointer || member.array_info.is_array()) {
            continue;
        }

        std::unordered_set<std::string> visited;
        std::vector<std::string> path;
        path.push_back(check_name);

        if (detectCircularReference(check_name, member.type_alias, visited,
                                    path)) {
            std::string cycle_path = check_name;
            for (size_t i = 1; i < path.size(); ++i) {
                cycle_path += " -> " + path[i];
            }
            error("Circular reference detected in struct value members: " +
                  cycle_path + ". Use pointers to break the cycle.");
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
    ASTNode *node = new ASTNode(ASTNodeType::AST_STRUCT_TYPEDEF_DECL);
    node->name = alias_name;
    setLocation(node, current_token_);

    // struct定義のメンバー情報をASTノードに保存
    for (const auto &member : struct_def.members) {
        ASTNode *member_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
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
ASTNode *RecursiveParser::parseEnumTypedefDeclaration() {
    return enum_parser_->parseEnumTypedefDeclaration();
}

// typedef union宣言の解析 (TypeScript-like literal types): typedef NAME =
// value1 | value2 | ...
ASTNode *RecursiveParser::parseUnionTypedefDeclaration() {
    // This method is now integrated into parseTypedefDeclaration()
    // This function is kept for compatibility but should not be called directly
    error("parseUnionTypedefDeclaration should not be called directly");
    return nullptr;
}

// Union値の解析ヘルパー関数
bool RecursiveParser::parseUnionValue(UnionDefinition &union_def) {
    return union_parser_->parseUnionValue(union_def);
}

// DELETED: 221 lines moved to union_parser.cpp

// メンバアクセスの解析: obj.member
ASTNode *RecursiveParser::parseMemberAccess(ASTNode *object) {
    return expression_parser_->parseMemberAccess(object);
}

// アロー演算子アクセスの解析: ptr->member
ASTNode *RecursiveParser::parseArrowAccess(ASTNode *object) {
    return expression_parser_->parseArrowAccess(object);
}

// 構造体リテラルの解析: {member: value, member2: value2}
ASTNode *RecursiveParser::parseStructLiteral() {
    return expression_parser_->parseStructLiteral();
}

// 配列リテラルの解析: [{}, {}, ...] または [1, 2, 3]
ASTNode *RecursiveParser::parseArrayLiteral() {
    return expression_parser_->parseArrayLiteral();
}

ASTNode *RecursiveParser::parseEnumDeclaration() {
    return enum_parser_->parseEnumDeclaration();
}

// interface宣言の解析: interface name { methods };
ASTNode *RecursiveParser::parseInterfaceDeclaration() {
    return interface_parser_->parseInterfaceDeclaration();
}

// DELETED: 122 lines moved to interface_parser.cpp
/*
 * OLD CODE: consume(TokenType::TOK_INTERFACE, "Expected 'interface'");

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
        } else if (check(TokenType::TOK_PRINT) || check(TokenType::TOK_PRINTLN)
 || check(TokenType::TOK_PRINTF)) {
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
        consume(TokenType::TOK_SEMICOLON, "Expected ';' after interface method
 declaration");

        interface_def.methods.push_back(method);
    }

    consume(TokenType::TOK_RBRACE, "Expected '}' after interface methods");
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after interface
 definition");

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

*/

// impl宣言の解析: impl InterfaceName for StructName { methods }
ASTNode *RecursiveParser::parseImplDeclaration() {
    return interface_parser_->parseImplDeclaration();
}

// DELETED: 298 lines moved to interface_parser.cpp
/*
OLD CODE: consume(TokenType::TOK_IMPL, "Expected 'impl'");

    if (!check(TokenType::TOK_IDENTIFIER)) {
        error("Expected interface name after 'impl'");
        return nullptr;
    }

    std::string interface_name = current_token_.value;

    // ★ 課題1の解決: 存在しないinterfaceを実装しようとする場合のエラー検出
    if (interface_definitions_.find(interface_name) ==
interface_definitions_.end()) { error("Interface '" + interface_name + "' is not
defined. Please declare the interface before implementing it."); return nullptr;
    }

    advance();

    // 'for' keyword
    if (!check(TokenType::TOK_FOR) &&
        !(check(TokenType::TOK_IDENTIFIER) && current_token_.value == "for")) {
        error("Expected 'for' after interface name in impl declaration");
        return nullptr;
    }
    advance(); // consume 'for'

    if (!check(TokenType::TOK_IDENTIFIER) && !check(TokenType::TOK_STRING_TYPE)
&& !check(TokenType::TOK_INT) && !check(TokenType::TOK_LONG) &&
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
        error("Cannot implement interface for raw array type '" + struct_name +
"[...]'. Use typedef to define array type first."); return nullptr;
    }

    consume(TokenType::TOK_LBRACE, "Expected '{' after type name in impl
declaration");

    ImplDefinition impl_def(interface_name, struct_name);
    std::vector<std::unique_ptr<ASTNode>> method_nodes; //
所有権は一時的に保持し、最終的にASTへ移動 std::vector<std::unique_ptr<ASTNode>>
static_var_nodes; // impl static変数

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
                error("Expected variable name after type in impl static
declaration"); return nullptr;
            }
            std::string var_name = current_token_.value;
            advance();

            // 初期化式の解析（オプション）
            std::unique_ptr<ASTNode> init_expr;
            if (check(TokenType::TOK_ASSIGN)) {
                advance(); // consume '='
                ASTNode* expr_raw = parseExpression();
                if (!expr_raw) {
                    error("Expected expression after '=' in impl static variable
initialization"); return nullptr;
                }
                init_expr.reset(expr_raw);
            }

            consume(TokenType::TOK_SEMICOLON, "Expected ';' after impl static
variable declaration");

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
            debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(),
"impl_static_variable");

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
        } else if (check(TokenType::TOK_PRINT) || check(TokenType::TOK_PRINTLN)
|| check(TokenType::TOK_PRINTF)) {
            // 予約キーワードだが、メソッド名として許可
            method_name = current_token_.value;
            advance();
        } else {
            error("Expected method name in method implementation");
            return nullptr;
        }

        // 関数宣言として解析
        ASTNode* method_impl_raw =
parseFunctionDeclarationAfterName(return_type, method_name); if
(method_impl_raw) { std::unique_ptr<ASTNode> method_impl(method_impl_raw);
            debug_print("[IMPL_PARSE] After method '%s', current token = %s
(type %d)\n", method_name.c_str(), current_token_.value.c_str(),
(int)current_token_.type);
            // privateフラグを設定
            method_impl->is_private_method = is_private_method;
            // privateメソッドの場合はinterface署名チェックをスキップ
            if (!is_private_method) {
                // ★ 課題2の解決: メソッド署名の不一致の検出
                auto interface_it = interface_definitions_.find(interface_name);
                if (interface_it != interface_definitions_.end()) {
                    bool method_found = false;
                    for (const auto& interface_method :
interface_it->second.methods) { if (interface_method.name == method_name) {
                            method_found = true;
                            auto format_type = [](TypeInfo type, bool
is_unsigned_flag) { std::string base = type_info_to_string(type); if
(is_unsigned_flag) { return std::string("unsigned ") + base;
                                }
                                return base;
                            };

                            TypeInfo expected_return_type_info =
interface_method.return_type; bool expected_return_unsigned =
interface_method.return_is_unsigned;

                            TypeInfo actual_return_type_info = TYPE_UNKNOWN;
                            if (!method_impl->return_types.empty()) {
                                actual_return_type_info =
method_impl->return_types[0]; } else { actual_return_type_info =
getTypeInfoFromString(return_type);
                            }
                            bool actual_return_unsigned =
method_impl->is_unsigned;

                            if (expected_return_type_info !=
actual_return_type_info || expected_return_unsigned != actual_return_unsigned) {
                                error("Method signature mismatch: Expected
return type '" + format_type(expected_return_type_info,
expected_return_unsigned) +
                                      "' but got '" +
                                      format_type(actual_return_type_info,
actual_return_unsigned) +
                                      "' for method '" + method_name + "'");
                                return nullptr;
                            }
                            // 引数の数をチェック
                            if (interface_method.parameters.size() !=
method_impl->parameters.size()) { error("Method signature mismatch: Expected " +
                                      std::to_string(interface_method.parameters.size())
+ " parameter(s) but got " + std::to_string(method_impl->parameters.size()) + "
for method '" + method_name + "'"); return nullptr;
                            }
                            // 引数の型をチエック
                            for (size_t i = 0; i <
interface_method.parameters.size(); ++i) { TypeInfo expected_param_type =
interface_method.parameters[i].second; bool expected_param_unsigned =
interface_method.get_parameter_is_unsigned(i); TypeInfo actual_param_type =
method_impl->parameters[i]->type_info; bool actual_param_unsigned =
method_impl->parameters[i]->is_unsigned;

                                if (expected_param_type != actual_param_type ||
                                    expected_param_unsigned !=
actual_param_unsigned) { error("Method signature mismatch: Parameter " +
std::to_string(i + 1) + " expected type '" + format_type(expected_param_type,
expected_param_unsigned) +
                                          "' but got '" +
format_type(actual_param_type, actual_param_unsigned) +
                                          "' for method '" + method_name + "'");
                                    return nullptr;
                                }
                            }
                            break;
                        }
                    }
                    if (!method_found) {
                        // 警告:
interfaceに定義されていないメソッドが実装されている std::cerr << "[WARNING]
Method '" << method_name << "' is implemented but not declared in interface '"
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
                error("Incomplete implementation: Method '" +
interface_method.name +
                      "' declared in interface '" + interface_name + "' is not
implemented"); return nullptr;
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
    // v0.13.0: unique_ptrを含むため move
    impl_definitions_.push_back(std::move(impl_def));

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
*/

// 循環参照検出: 値メンバーによる循環を検出（ポインタメンバーは除外）
bool RecursiveParser::detectCircularReference(
    const std::string &struct_name, const std::string &member_type,
    std::unordered_set<std::string> &visited, std::vector<std::string> &path) {
    return type_utility_parser_->detectCircularReference(
        struct_name, member_type, visited, path);
}

// 関数ポインタtypedef構文かどうかをチェック
bool RecursiveParser::isFunctionPointerTypedef() {
    // 簡単なチェック：次のトークンの種類で判断
    // 型名の後に '(' が来たら関数ポインタtypedefの可能性
    // 型名の後に識別子が来たら通常のtypedef

    // 現在のトークンが型名かチェック
    bool is_type_token = (current_token_.type == TokenType::TOK_VOID ||
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
                          current_token_.type == TokenType::TOK_IDENTIFIER);

    if (!is_type_token) {
        return false;
    }

    // レキサーから次のトークンを取得（先読み）
    Token next_token = lexer_.peekToken();

    // 型名の次が '(' なら関数ポインタtypedef
    return (next_token.type == TokenType::TOK_LPAREN);
}

// 関数ポインタtypedef宣言の解析
ASTNode *RecursiveParser::parseFunctionPointerTypedefDeclaration() {
    return declaration_parser_->parseFunctionPointerTypedefDeclaration();
}

// v0.11.0: ジェネリック構造体のインスタンス化
// 例: Box<int> をインスタンス化 -> Box_int という具体的な構造体を生成
// v0.11.0: ジェネリック型のインスタンス化

// 型名を正規化してインスタンス名に使用可能な形式に変換
// 例: "int*" -> "int_ptr", "int[3]" -> "int_array_3"
// NOTE: Currently unused, but kept for potential future use
[[maybe_unused]] static std::string
normalizeTypeNameForInstantiation(const std::string &type_name) {
    std::string normalized = type_name;

    // ポインタの '*' を '_ptr' に置換
    size_t star_pos = normalized.find('*');
    while (star_pos != std::string::npos) {
        normalized.replace(star_pos, 1, "_ptr");
        star_pos = normalized.find('*', star_pos + 4);
    }

    // 配列の '[N]' を '_array_N' に置換
    size_t bracket_pos = normalized.find('[');
    if (bracket_pos != std::string::npos) {
        size_t end_bracket = normalized.find(']', bracket_pos);
        if (end_bracket != std::string::npos) {
            std::string size = normalized.substr(bracket_pos + 1,
                                                 end_bracket - bracket_pos - 1);
            std::string replacement = "_array";
            if (!size.empty()) {
                replacement += "_" + size;
            }
            normalized.replace(bracket_pos, end_bracket - bracket_pos + 1,
                               replacement);
        }
    }

    // 参照の '&' を '_ref' に置換
    size_t amp_pos = normalized.find('&');
    while (amp_pos != std::string::npos) {
        normalized.replace(amp_pos, 1, "_ref");
        amp_pos = normalized.find('&', amp_pos + 4);
    }

    return normalized;
}

void RecursiveParser::instantiateGenericStruct(
    const std::string &base_name,
    const std::vector<std::string> &type_arguments) {

    // v0.11.0: インスタンス化された型名を生成: Queue<int> 形式のまま
    // マングリングしない（シンプルで、typeofの実装が容易）
    std::string instantiated_name = base_name + "<";
    for (size_t i = 0; i < type_arguments.size(); ++i) {
        if (i > 0)
            instantiated_name += ", ";
        instantiated_name += type_arguments[i];
    }
    instantiated_name += ">";

    // 既にインスタンス化済みかチェック
    if (struct_definitions_.find(instantiated_name) !=
        struct_definitions_.end()) {
        return; // 既に存在する
    }

    // ジェネリック基底の定義を取得
    auto it = struct_definitions_.find(base_name);
    if (it == struct_definitions_.end()) {
        error("Generic struct '" + base_name + "' not found");
        return;
    }

    const StructDefinition &generic_base = it->second;
    if (!generic_base.is_generic) {
        error("Struct '" + base_name + "' is not a generic type");
        return;
    }

    // 型パラメータと型引数のマッピングを作成
    std::unordered_map<std::string, std::string> type_map;
    for (size_t i = 0; i < generic_base.type_parameters.size(); ++i) {
        type_map[generic_base.type_parameters[i]] = type_arguments[i];
    }

    // インスタンス化された構造体定義を作成
    StructDefinition instantiated_struct(instantiated_name);
    instantiated_struct.is_generic = false; // インスタンス化後は通常の構造体
    instantiated_struct.is_forward_declaration = false;

    // v0.11.0 Phase 1a: インターフェース境界情報を保持
    // 実際の型チェックはインタープリター側で行う
    instantiated_struct.interface_bounds = generic_base.interface_bounds;
    instantiated_struct.type_parameters = generic_base.type_parameters;
    instantiated_struct.type_parameter_bindings =
        type_map; // 型引数のバインディングを保存

    // メンバーを型置換してコピー
    for (const auto &member : generic_base.members) {
        std::string member_type_alias = member.type_alias;
        TypeInfo member_type_info = member.type;

        // 型パラメータを具体的な型に置換
        if (type_map.find(member_type_alias) != type_map.end()) {
            member_type_alias = type_map[member_type_alias];
            // 具体的な型のTypeInfoを取得
            member_type_info =
                type_utility_parser_->getTypeInfoFromString(member_type_alias);
        }

        instantiated_struct.add_member(
            member.name, member_type_info, member_type_alias, member.is_pointer,
            member.pointer_depth, member.pointer_base_type_name,
            member.pointer_base_type, member.is_private, member.is_reference,
            member.is_unsigned, member.is_const);

        // 配列情報もコピー
        if (member.array_info.is_array()) {
            instantiated_struct.members.back().array_info = member.array_info;
        }
    }

    // インスタンス化された構造体を登録
    struct_definitions_[instantiated_name] = instantiated_struct;

    if (debug_mode_) {
        std::cerr << "[GENERICS] Instantiated " << base_name << "<";
        for (size_t i = 0; i < type_arguments.size(); ++i) {
            if (i > 0)
                std::cerr << ", ";
            std::cerr << type_arguments[i];
        }
        std::cerr << "> as " << instantiated_name << std::endl;
    }
}

// v0.11.0: ジェネリックenumのインスタンス化
void RecursiveParser::instantiateGenericEnum(
    const std::string &base_name,
    const std::vector<std::string> &type_arguments) {

    // v0.11.0: インスタンス化された型名を生成: Option<int> 形式のまま
    // マングリングしない（シンプルで、typeofの実装が容易）
    std::string instantiated_name = base_name + "<";
    for (size_t i = 0; i < type_arguments.size(); ++i) {
        if (i > 0)
            instantiated_name += ", ";
        instantiated_name += type_arguments[i];
    }
    instantiated_name += ">";

    // 既にインスタンス化済みかチェック
    if (enum_definitions_.find(instantiated_name) != enum_definitions_.end()) {
        return; // 既に存在する
    }

    // ジェネリック基底の定義を取得
    auto it = enum_definitions_.find(base_name);
    if (it == enum_definitions_.end()) {
        error("Generic enum '" + base_name + "' not found");
        return;
    }

    const EnumDefinition &generic_base = it->second;
    if (!generic_base.is_generic) {
        error("Enum '" + base_name + "' is not a generic type");
        return;
    }

    // 型パラメータの数をチェック
    if (type_arguments.size() != generic_base.type_parameters.size()) {
        error("Generic enum '" + base_name + "' expects " +
              std::to_string(generic_base.type_parameters.size()) +
              " type argument(s), but got " +
              std::to_string(type_arguments.size()));
        return;
    }

    // 型パラメータと型引数のマッピングを作成
    std::unordered_map<std::string, std::string> type_map;
    for (size_t i = 0; i < generic_base.type_parameters.size(); ++i) {
        type_map[generic_base.type_parameters[i]] = type_arguments[i];
    }

    // インスタンス化されたenum定義を作成
    EnumDefinition instantiated_enum(instantiated_name);
    instantiated_enum.is_generic = false; // インスタンス化後は通常のenum
    instantiated_enum.has_associated_values =
        generic_base.has_associated_values;

    // メンバーを型置換してコピー
    for (const auto &member : generic_base.members) {
        EnumMember new_member = member;

        if (member.has_associated_value) {
            // 型パラメータを具体的な型に置換
            if (type_map.find(member.associated_type_name) != type_map.end()) {
                std::string concrete_type =
                    type_map[member.associated_type_name];
                new_member.associated_type_name = concrete_type;
                // 具体的な型のTypeInfoを取得
                new_member.associated_type =
                    type_utility_parser_->getTypeInfoFromString(concrete_type);
            }
        }

        instantiated_enum.members.push_back(new_member);
    }

    // インスタンス化されたenumを登録
    enum_definitions_[instantiated_name] = instantiated_enum;

    if (debug_mode_) {
        std::cerr << "[GENERICS] Instantiated enum " << base_name << "<";
        for (size_t i = 0; i < type_arguments.size(); ++i) {
            if (i > 0)
                std::cerr << ", ";
            std::cerr << type_arguments[i];
        }
        std::cerr << "> as " << instantiated_name << std::endl;
    }
}

// ========================================================================
// v0.11.0: 組み込み型の初期化
// Option<T>, Result<T, E> を自動的にenum定義として登録
// ========================================================================
void RecursiveParser::initialize_builtin_types() {
    // Option<T> enum定義
    EnumDefinition option_def;
    option_def.name = "Option";
    option_def.is_generic = true;
    option_def.has_associated_values = true;
    option_def.type_parameters.push_back("T");

    // Some(T) variant
    EnumMember some_member;
    some_member.name = "Some";
    some_member.value = 0;
    some_member.explicit_value = true;
    some_member.has_associated_value = true;
    some_member.associated_type_name = "T";
    option_def.members.push_back(some_member);

    // None variant
    EnumMember none_member;
    none_member.name = "None";
    none_member.value = 1;
    none_member.explicit_value = true;
    none_member.has_associated_value = false;
    option_def.members.push_back(none_member);

    enum_definitions_["Option"] = option_def;

    // Result<T, E> enum定義
    EnumDefinition result_def;
    result_def.name = "Result";
    result_def.is_generic = true;
    result_def.has_associated_values = true;
    result_def.type_parameters.push_back("T");
    result_def.type_parameters.push_back("E");

    // Ok(T) variant
    EnumMember ok_member;
    ok_member.name = "Ok";
    ok_member.value = 0;
    ok_member.explicit_value = true;
    ok_member.has_associated_value = true;
    ok_member.associated_type_name = "T";
    result_def.members.push_back(ok_member);

    // Err(E) variant
    EnumMember err_member;
    err_member.name = "Err";
    err_member.value = 1;
    err_member.explicit_value = true;
    err_member.has_associated_value = true;
    err_member.associated_type_name = "E";
    result_def.members.push_back(err_member);

    enum_definitions_["Result"] = result_def;

    if (debug_mode_) {
        std::cerr << "[BUILTIN_TYPES] Registered Option<T> and Result<T, E> as "
                     "builtin enum types"
                  << std::endl;
    }
}

/**
 * @brief ソースファイルのディレクトリを取得
 */
std::string RecursiveParser::getSourceDirectory() const {
    size_t last_slash = filename_.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        return filename_.substr(0, last_slash + 1);
    }
    return "./";
}

/**
 * @brief モジュールパスを実際のファイルパスに解決
 *
 * ドット記法（stdlib.collections.vector）をスラッシュ区切り（stdlib/collections/vector.cb）に変換
 * また、検索パス（stdlib/, modules/など）を試行して実際のファイルを探す
 */
std::string RecursiveParser::resolveModulePath(const std::string &module_path) {
    std::string file_path = module_path;

    // .cbが既に含まれているか確認
    bool has_extension = (file_path.find(".cb") != std::string::npos);

    if (!has_extension) {
        // ドット記法の場合、パスに変換
        // stdlib.collections.vector -> stdlib/collections/vector.cb
        if (module_path.find('.') != std::string::npos &&
            module_path.find('/') == std::string::npos &&
            module_path.find("..") == std::string::npos) {
            std::replace(file_path.begin(), file_path.end(), '.', '/');
            file_path += ".cb";
        } else {
            // 拡張子がない場合は追加
            file_path += ".cb";
        }
    }

    // Note: .cbが既に含まれている場合も、検索パスの対象とする
    // （早期リターンせず、source directoryなどを試行する）

    // 検索パスの優先順位
    std::vector<std::string> search_paths;
    std::string source_dir = getSourceDirectory();

    // 相対パス（../ や ./）の場合、そのまま試す
    if (file_path.find("../") == 0 || file_path.find("./") == 0) {
        search_paths.push_back(file_path);
    } else {
        // 通常のモジュール検索パス
        search_paths = {
            source_dir +
                file_path, // ソースファイルと同じディレクトリ（最優先）
            file_path,                    // カレントディレクトリ
            "stdlib/" + file_path,        // stdlibディレクトリ
            "modules/" + file_path,       // modulesディレクトリ
            "../modules/" + file_path,    // 1つ上のmodulesディレクトリ
            "../../modules/" + file_path, // 2つ上のmodulesディレクトリ
            "../" + file_path,            // 1つ上のディレクトリ
            "../../" + file_path,         // 2つ上のディレクトリ
        };
    }

    // ファイルが存在するパスを探す
    for (const auto &path : search_paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            file.close();
            if (debug_mode_) {
                std::cerr << "[IMPORT] Found module at: " << path << std::endl;
            }
            return path;
        }
    }

    // 見つからない場合は元のパスを返す（エラーは後で処理）
    return file_path;
}

/**
 * @brief import文をパース時に処理し、モジュールの定義を取り込む
 *
 * これにより、import後に型を使用する際にParser側で型が認識される
 */
void RecursiveParser::processImport(
    const std::string &module_path,
    const std::vector<std::string> &import_items) {
    // パス解決
    std::string resolved_path = resolveModulePath(module_path);

    if (debug_mode_) {
        std::cerr << "[IMPORT] Processing import: " << module_path << " -> "
                  << resolved_path << std::endl;
        if (!import_items.empty()) {
            std::cerr << "[IMPORT] Selective import: {";
            for (size_t i = 0; i < import_items.size(); ++i) {
                if (i > 0)
                    std::cerr << ", ";
                std::cerr << import_items[i];
            }
            std::cerr << "}" << std::endl;
        }
    }

    // ファイルを開く
    std::ifstream file(resolved_path);
    if (!file.is_open()) {
        // ファイルが見つからない場合は警告のみ
        // 実行時にInterpreter側でロードされる可能性があるため
        if (debug_mode_) {
            std::cerr << "[IMPORT] Warning: Module file not found: "
                      << module_path << " (tried: " << resolved_path << ")"
                      << " (will try runtime import)" << std::endl;
        }
        return;
    }

    // ファイル内容を読み込む
    std::string source_code((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();

    // 新しいParserインスタンスでモジュールをパース
    RecursiveParser module_parser(source_code, resolved_path);
    module_parser.setDebugMode(debug_mode_);

    ASTNode *module_ast = nullptr;
    try {
        module_ast = module_parser.parseProgram();
    } catch (const std::exception &e) {
        error("Failed to parse module '" + module_path + "': " + e.what());
        return;
    }

    if (!module_ast) {
        error("Failed to parse module: " + module_path);
        return;
    }

    // 選択的importの場合は、指定された項目のみをチェック
    // 空の場合は全てimport（デフォルトimport）
    bool is_selective = !import_items.empty();

    // 選択的importで必要な依存関係を自動的に追加するための一時リスト
    std::set<std::string> items_to_import;
    if (is_selective) {
        items_to_import.insert(import_items.begin(), import_items.end());
    }

    auto should_import_item = [&](const std::string &name) -> bool {
        if (!is_selective) {
            return true; // デフォルトimport: 全てimport
        }
        // 選択的import: 指定された項目または依存関係に含まれるもの
        return items_to_import.find(name) != items_to_import.end();
    };

    // 選択的importの場合、依存関係を解決
    // 例: SystemAllocatorをimportする場合、Allocatorインターフェースも必要
    if (is_selective) {
        // struct定義をチェックして、依存するinterfaceを追加
        for (const auto &item_name : import_items) {
            auto it = module_parser.struct_definitions_.find(item_name);
            if (it != module_parser.struct_definitions_.end()) {
                const StructDefinition &def = it->second;

                // 型パラメータの境界インターフェースをチェック
                // 例: A: Allocator の場合、Allocatorを自動的にimport
                for (const auto &bound_pair : def.interface_bounds) {
                    for (const auto &bound : bound_pair.second) {
                        items_to_import.insert(bound);
                        if (debug_mode_) {
                            std::cerr << "[IMPORT] Auto-importing dependency: "
                                      << bound << " (required by " << item_name
                                      << ")" << std::endl;
                        }
                    }
                }
            }
        }
    }

    // モジュールからexportされた定義を取り込む
    // 1. interface定義（structより先に処理して依存関係を解決）
    for (const auto &pair : module_parser.interface_definitions_) {
        const std::string &name = pair.first;
        const InterfaceDefinition &def = pair.second;

        // 選択的importの場合、指定された項目かチェック
        if (!should_import_item(name)) {
            continue;
        }

        bool is_exported = false;
        if (module_ast && !module_ast->statements.empty()) {
            for (const auto &stmt : module_ast->statements) {
                if (stmt && stmt->name == name && stmt->is_exported) {
                    is_exported = true;
                    break;
                }
            }
        }

        if (is_exported) {
            interface_definitions_[name] = def;
            if (debug_mode_) {
                std::cerr << "[IMPORT] Imported interface: " << name
                          << std::endl;
            }
        }
    }

    // 2. struct定義
    for (const auto &pair : module_parser.struct_definitions_) {
        const std::string &name = pair.first;
        const StructDefinition &def = pair.second;

        // 選択的importの場合、指定された項目かチェック
        if (!should_import_item(name)) {
            continue;
        }

        // 元のASTでis_exportedフラグをチェック
        bool is_exported = false;
        if (module_ast && !module_ast->statements.empty()) {
            for (const auto &stmt : module_ast->statements) {
                if (stmt && stmt->name == name && stmt->is_exported) {
                    is_exported = true;
                    break;
                }
            }
        }

        if (is_exported) {
            struct_definitions_[name] = def;
            if (debug_mode_) {
                std::cerr << "[IMPORT] Imported struct: " << name;
                if (def.is_generic) {
                    std::cerr << " (generic, type_params: "
                              << def.type_parameters.size() << ")";
                }
                std::cerr << std::endl;
            }
        }
    }

    // 3. enum定義
    for (const auto &pair : module_parser.enum_definitions_) {
        const std::string &name = pair.first;
        const EnumDefinition &def = pair.second;

        // 選択的importの場合、指定された項目かチェック
        if (!should_import_item(name)) {
            continue;
        }

        bool is_exported = false;
        if (module_ast && !module_ast->statements.empty()) {
            for (const auto &stmt : module_ast->statements) {
                if (stmt && stmt->name == name && stmt->is_exported) {
                    is_exported = true;
                    break;
                }
            }
        }

        if (is_exported) {
            enum_definitions_[name] = def;
            if (debug_mode_) {
                std::cerr << "[IMPORT] Imported enum: " << name << std::endl;
            }
        }
    }

    // 4. impl定義も取り込む（struct/interfaceと関連）
    // implブロックはexportできず、自動的に実装される
    // exportされたstruct/interfaceに関連するimplは全て取り込む
    for (const auto &impl : module_parser.impl_definitions_) {
        // exportされたstructまたはinterfaceに関連するimplのみを取り込む
        bool should_import = false;

        if (debug_mode_) {
            std::cerr << "[IMPORT] Checking impl: struct_name='"
                      << impl.struct_name << "' interface_name='"
                      << impl.interface_name << "'" << std::endl;
        }

        // struct_nameがexportされているかチェック
        // ジェネリック型の場合、"Vector<int,
        // SystemAllocator>"や"Vector_int_SystemAllocator"から"Vector"を抽出
        std::string base_struct_name = impl.struct_name;

        // パターン1: "Vector<int, SystemAllocator>" -> "Vector"
        size_t bracket_pos = impl.struct_name.find('<');
        if (bracket_pos != std::string::npos) {
            base_struct_name = impl.struct_name.substr(0, bracket_pos);
        } else {
            // パターン2: "Vector_int_SystemAllocator" -> "Vector"
            size_t underscore_pos = impl.struct_name.find('_');
            if (underscore_pos != std::string::npos) {
                base_struct_name = impl.struct_name.substr(0, underscore_pos);
            }
        }

        if (struct_definitions_.find(impl.struct_name) !=
            struct_definitions_.end()) {
            should_import = true;
            if (debug_mode_) {
                std::cerr << "[IMPORT]   -> Matched by full name" << std::endl;
            }
        } else if (base_struct_name != impl.struct_name &&
                   struct_definitions_.find(base_struct_name) !=
                       struct_definitions_.end()) {
            // ジェネリック型の場合、ベース名でもチェック
            should_import = true;
            if (debug_mode_) {
                std::cerr << "[IMPORT]   -> Matched by base name '"
                          << base_struct_name << "'" << std::endl;
            }
        }

        // interface_nameがexportされているかチェック
        if (!should_import &&
            interface_definitions_.find(impl.interface_name) !=
                interface_definitions_.end()) {
            should_import = true;
            if (debug_mode_) {
                std::cerr << "[IMPORT]   -> Matched by interface name"
                          << std::endl;
            }
        }

        if (should_import) {
            // v0.11.0:
            // impl定義はimpl_nodes転送後に追加する（ポインタ更新のため）
            // imported_implは後で処理
        } else if (debug_mode_) {
            std::cerr << "[IMPORT]   -> NOT imported (no match)" << std::endl;
        }
    }

    // v0.11.0: implノードの所有権を転送（use-after-free対策）
    // module_parserのimpl_nodes_をこのparserに移動
    auto &module_impl_nodes = module_parser.get_impl_nodes_for_transfer();
    // size_t transferred_node_count = module_impl_nodes.size(); (unused)
    size_t impl_node_start_idx = impl_nodes_.size();

    if (!module_impl_nodes.empty()) {
        if (debug_mode_) {
            std::cerr << "[IMPORT] Transferring " << module_impl_nodes.size()
                      << " impl nodes from module parser" << std::endl;
        }

        for (auto &node : module_impl_nodes) {
            impl_nodes_.push_back(std::move(node));
        }
        module_impl_nodes.clear();
    }

    // v0.11.0: impl定義を追加し、impl_nodeポインタを更新済みノードに設定
    // すべてのimpl定義を追加（元のループですでにフィルタリング済み）
    const auto &module_impl_defs = module_parser.get_impl_definitions();
    size_t node_idx = impl_node_start_idx;
    for (size_t impl_idx = 0;
         impl_idx < module_impl_defs.size() && node_idx < impl_nodes_.size();
         ++impl_idx) {
        const auto &impl = module_impl_defs[impl_idx];

        // v0.11.0: 常にインポート（元のループで既にフィルタリング済み）
        if (true) {
            ImplDefinition imported_impl = impl; // コピー

            // impl_nodeポインタを転送済みノードに更新
            imported_impl.impl_node = impl_nodes_[node_idx].get();

            // methods/constructors/destructorも更新済みノードから取得
            imported_impl.methods.clear();
            imported_impl.constructors.clear();
            imported_impl.destructor = nullptr;
            for (const auto &arg : imported_impl.impl_node->arguments) {
                if (arg->node_type == ASTNodeType::AST_FUNC_DECL) {
                    imported_impl.methods.push_back(arg.get());
                } else if (arg->node_type ==
                           ASTNodeType::AST_CONSTRUCTOR_DECL) {
                    imported_impl.constructors.push_back(arg.get());
                } else if (arg->node_type == ASTNodeType::AST_DESTRUCTOR_DECL) {
                    imported_impl.destructor = arg.get();
                }
            }

            if (debug_mode_) {
                std::cerr << "[IMPORT] Imported impl: "
                          << imported_impl.struct_name << " for "
                          << imported_impl.interface_name << std::endl;
            }

            impl_definitions_.push_back(imported_impl);
            node_idx++;
        }
    }

    // v0.11.0:
    // module_parserのimpl_definitions_をクリア（破棄時のuse-after-free対策）
    auto &module_impl_defs_clear =
        module_parser.get_impl_definitions_for_clear();
    module_impl_defs_clear.clear();

    // NOTE: ASTノードの所有権はimpl_nodes_に移動済み
    // impl定義内のimpl_nodeポインタはこのimpl_nodes_内のノードを参照
    // 最終的にInterpreter::sync_impl_definitions_from_parser()で
    // Interpreterに転送され、プログラム終了まで保持される
    if (debug_mode_ && module_ast) {
        std::cerr << "[IMPORT] impl nodes transferred to parser: "
                  << module_path << std::endl;
    }
}
