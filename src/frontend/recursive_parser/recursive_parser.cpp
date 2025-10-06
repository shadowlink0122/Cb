#include "recursive_parser.h"
#include "parsers/expression_parser.h"
#include "parsers/statement_parser.h"
#include "parsers/declaration_parser.h"
#include "parsers/type_parser.h"
#include "parsers/struct_parser.h"
#include "parsers/enum_parser.h"
#include "parsers/interface_parser.h"
#include "parsers/union_parser.h"
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
    enum_parser_ = std::make_unique<EnumParser>(this);
    interface_parser_ = std::make_unique<InterfaceParser>(this);
    union_parser_ = std::make_unique<UnionParser>(this);
    
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
    return statement_parser_->parseStatement();
}


ASTNode* RecursiveParser::parseVariableDeclaration() {
    return declaration_parser_->parseVariableDeclaration();
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
    return expression_parser_->parseExpression();
}

ASTNode* RecursiveParser::parseAssignment() {
    return expression_parser_->parseAssignment();
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
    return expression_parser_->parseLogicalOr();
}

ASTNode* RecursiveParser::parseLogicalAnd() {
    return expression_parser_->parseLogicalAnd();
}

ASTNode* RecursiveParser::parseBitwiseOr() {
    return expression_parser_->parseBitwiseOr();
}

ASTNode* RecursiveParser::parseBitwiseXor() {
    return expression_parser_->parseBitwiseXor();
}

ASTNode* RecursiveParser::parseBitwiseAnd() {
    return expression_parser_->parseBitwiseAnd();
}

ASTNode* RecursiveParser::parseComparison() {
    return expression_parser_->parseComparison();
}

ASTNode* RecursiveParser::parseShift() {
    return expression_parser_->parseShift();
}

ASTNode* RecursiveParser::parseAdditive() {
    return expression_parser_->parseAdditive();
}

ASTNode* RecursiveParser::parseMultiplicative() {
    return expression_parser_->parseMultiplicative();
}

ASTNode* RecursiveParser::parseUnary() {
    return expression_parser_->parseUnary();
}

ASTNode* RecursiveParser::parsePostfix() {
    return expression_parser_->parsePostfix();
}

ASTNode* RecursiveParser::parsePrimary() {
    return expression_parser_->parsePrimary();
}

ASTNode* RecursiveParser::parseFunctionDeclarationAfterName(const std::string& return_type, const std::string& function_name) {
    return declaration_parser_->parseFunctionDeclarationAfterName(return_type, function_name);
}

ASTNode* RecursiveParser::parseFunctionDeclaration() {
    return declaration_parser_->parseFunctionDeclaration();
}


ASTNode* RecursiveParser::parseTypedefDeclaration() {
    return declaration_parser_->parseTypedefDeclaration();
}

TypeInfo RecursiveParser::getTypeInfoFromString(const std::string& type_name) {
    if (type_name == "nullptr") {
        return TYPE_NULLPTR;
    }

    std::string working = type_name;
    // bool is_unsigned = false;  // 将来の拡張用に保持
    if (working.rfind("unsigned ", 0) == 0) {
        // is_unsigned = true;
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
    return statement_parser_->parseReturnStatement();
}

ASTNode* RecursiveParser::parseAssertStatement() {
    return statement_parser_->parseAssertStatement();
}

ASTNode* RecursiveParser::parseBreakStatement() {
    return statement_parser_->parseBreakStatement();
}

ASTNode* RecursiveParser::parseContinueStatement() {
    return statement_parser_->parseContinueStatement();
}

ASTNode* RecursiveParser::parseIfStatement() {
    return statement_parser_->parseIfStatement();
}

ASTNode* RecursiveParser::parseForStatement() {
    return statement_parser_->parseForStatement();
}

ASTNode* RecursiveParser::parseWhileStatement() {
    return statement_parser_->parseWhileStatement();
}

ASTNode* RecursiveParser::parseCompoundStatement() {
    return statement_parser_->parseCompoundStatement();
}

ASTNode* RecursiveParser::parsePrintlnStatement() {
    return statement_parser_->parsePrintlnStatement();
}

ASTNode* RecursiveParser::parsePrintStatement() {
    return statement_parser_->parsePrintStatement();
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
    return declaration_parser_->parseTypedefVariableDeclaration();
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
    return enum_parser_->parseEnumTypedefDeclaration();
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
    return union_parser_->parseUnionValue(union_def);
}

// DELETED: 221 lines moved to union_parser.cpp

// メンバアクセスの解析: obj.member
ASTNode* RecursiveParser::parseMemberAccess(ASTNode* object) {
    return expression_parser_->parseMemberAccess(object);
}

// アロー演算子アクセスの解析: ptr->member
ASTNode* RecursiveParser::parseArrowAccess(ASTNode* object) {
    return expression_parser_->parseArrowAccess(object);
}

// 構造体リテラルの解析: {member: value, member2: value2}
ASTNode* RecursiveParser::parseStructLiteral() {
    return expression_parser_->parseStructLiteral();
}

// 配列リテラルの解析: [{}, {}, ...] または [1, 2, 3]
ASTNode* RecursiveParser::parseArrayLiteral() {
    return expression_parser_->parseArrayLiteral();
}

ASTNode* RecursiveParser::parseEnumDeclaration() {
    return enum_parser_->parseEnumDeclaration();
}

// interface宣言の解析: interface name { methods };
ASTNode* RecursiveParser::parseInterfaceDeclaration() {
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
        
*/

// impl宣言の解析: impl InterfaceName for StructName { methods }
ASTNode* RecursiveParser::parseImplDeclaration() {
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
*/

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
    return declaration_parser_->parseFunctionPointerTypedefDeclaration();
}

