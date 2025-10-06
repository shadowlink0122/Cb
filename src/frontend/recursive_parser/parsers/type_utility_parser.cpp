#include "type_utility_parser.h"
#include "../recursive_parser.h"
#include "../../../common/debug.h"
#include <stdexcept>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>

TypeUtilityParser::TypeUtilityParser(RecursiveParser* parser) 
    : parser_(parser) {}

std::string TypeUtilityParser::parseType() {
    ParsedTypeInfo parsed;
    parsed.array_info = ArrayTypeInfo();

    std::string base_type;
    std::string original_type;
    bool saw_unsigned = false;
    bool saw_const = false;

    // Check for const qualifier
    if (parser_->check(TokenType::TOK_CONST)) {
        saw_const = true;
        parser_->advance();
    }

    if (parser_->check(TokenType::TOK_UNSIGNED)) {
        saw_unsigned = true;
        parser_->advance();
    }

    auto set_base_type = [&](const std::string &type_name) {
        base_type = type_name;
        if (original_type.empty()) {
            original_type = type_name;
        }
    };

    if (parser_->check(TokenType::TOK_INT)) {
        parser_->advance();
        set_base_type("int");
    } else if (parser_->check(TokenType::TOK_LONG)) {
        parser_->advance();
        set_base_type("long");
    } else if (parser_->check(TokenType::TOK_SHORT)) {
        parser_->advance();
        set_base_type("short");
    } else if (parser_->check(TokenType::TOK_TINY)) {
        parser_->advance();
        set_base_type("tiny");
    } else if (parser_->check(TokenType::TOK_VOID)) {
        parser_->advance();
        set_base_type("void");
    } else if (parser_->check(TokenType::TOK_BOOL)) {
        parser_->advance();
        set_base_type("bool");
    } else if (parser_->check(TokenType::TOK_FLOAT)) {
        parser_->advance();
        set_base_type("float");
    } else if (parser_->check(TokenType::TOK_DOUBLE)) {
        parser_->advance();
        set_base_type("double");
    } else if (parser_->check(TokenType::TOK_BIG)) {
        parser_->advance();
        set_base_type("big");
    } else if (parser_->check(TokenType::TOK_QUAD)) {
        parser_->advance();
        set_base_type("quad");
    } else if (parser_->check(TokenType::TOK_STRING_TYPE)) {
        parser_->advance();
        set_base_type("string");
    } else if (parser_->check(TokenType::TOK_CHAR_TYPE)) {
        parser_->advance();
        set_base_type("char");
    } else if (parser_->check(TokenType::TOK_STRUCT)) {
        parser_->advance();
        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->error("Expected struct name after 'struct'");
            return "";
        }
        std::string struct_name = parser_->current_token_.value;
        parser_->advance();
        original_type = "struct " + struct_name;
        base_type = original_type;
    } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        std::string identifier = parser_->current_token_.value;
        if (parser_->typedef_map_.find(identifier) != parser_->typedef_map_.end()) {
            parser_->advance();
            original_type = identifier;
            std::string resolved = resolveTypedefChain(identifier);
            if (resolved.empty()) {
                parser_->error("Unknown type: " + identifier);
                throw std::runtime_error("Unknown type: " + identifier);
            }
            set_base_type(resolved);
        } else if (parser_->struct_definitions_.find(identifier) != parser_->struct_definitions_.end()) {
            parser_->advance();
            original_type = identifier;
            set_base_type(identifier);
        } else if (parser_->enum_definitions_.find(identifier) != parser_->enum_definitions_.end()) {
            parser_->advance();
            original_type = identifier;
            set_base_type(identifier);
        } else if (parser_->interface_definitions_.find(identifier) != parser_->interface_definitions_.end()) {
            parser_->advance();
            original_type = identifier;
            set_base_type(identifier);
        } else if (parser_->union_definitions_.find(identifier) != parser_->union_definitions_.end()) {
            parser_->advance();
            original_type = identifier;
            set_base_type(identifier);
        } else {
            // 未定義型 - 前方参照の可能性として許容
            // (ポインタまたは配列の場合に限る - 後でポインタ/配列チェックで判定)
            parser_->advance();
            original_type = identifier;
            set_base_type(identifier);
            // NOTE: 値メンバーとして使用された場合のエラーは後で検出される
        }
    } else {
        parser_->error("Expected type specifier");
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
                      << parser_->current_token_.line << std::endl;
            break;
        case TYPE_BIG:
            parsed.is_unsigned = true;
            break;
        default:
            parser_->error("'unsigned' modifier can only be applied to numeric types");
            return "";
        }
    }

    int pointer_depth = 0;
    while (parser_->check(TokenType::TOK_MUL)) {
        pointer_depth++;
        parser_->advance();
    }

    if (pointer_depth > 0) {
        parsed.is_pointer = true;
        parsed.pointer_depth = pointer_depth;
    }

    if (parser_->check(TokenType::TOK_BIT_AND)) {
        parsed.is_reference = true;
        parser_->advance();
    }

    std::vector<ArrayDimension> dimensions;
    std::vector<std::string> dimension_texts;

    while (parser_->check(TokenType::TOK_LBRACKET)) {
        parser_->advance();

        if (parser_->check(TokenType::TOK_NUMBER)) {
            Token size_token = parser_->advance();
            dimensions.emplace_back(std::stoi(size_token.value), false, "");
            dimension_texts.push_back("[" + size_token.value + "]");
        } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
            Token const_token = parser_->advance();
            dimensions.emplace_back(-1, true, const_token.value);
            dimension_texts.push_back("[" + const_token.value + "]");
        } else {
            dimensions.emplace_back(-1, true, "");
            dimension_texts.push_back("[]");
        }

        parser_->consume(TokenType::TOK_RBRACKET, "Expected ']' in array type");
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

    parser_->last_parsed_type_info_ = parsed;
    return parsed.full_type;
}

TypeInfo TypeUtilityParser::getTypeInfoFromString(const std::string& type_name) {
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
    } else if (working.substr(0, 7) == "struct " || parser_->struct_definitions_.find(working) != parser_->struct_definitions_.end()) {
        return TYPE_STRUCT;
    } else if (working.substr(0, 5) == "enum " || parser_->enum_definitions_.find(working) != parser_->enum_definitions_.end()) {
        return TYPE_ENUM;
    } else if (working.substr(0, 10) == "interface " || parser_->interface_definitions_.find(working) != parser_->interface_definitions_.end()) {
        return TYPE_INTERFACE;
    } else if (parser_->union_definitions_.find(working) != parser_->union_definitions_.end()) {
        return TYPE_UNION;
    } else {
        return TYPE_UNKNOWN;
    }
}

std::string TypeUtilityParser::resolveTypedefChain(const std::string& typedef_name) {
    std::unordered_set<std::string> visited;
    std::string current = typedef_name;
    
    while (parser_->typedef_map_.find(current) != parser_->typedef_map_.end()) {
        if (visited.find(current) != visited.end()) {
            // 循環参照検出
            return "";
        }
        visited.insert(current);
        
        std::string next = parser_->typedef_map_[current];
        
        // 自分自身を指している場合（匿名structのtypedef）
        if (next == current) {
            // 構造体定義が存在するかチェック
            if (parser_->struct_definitions_.find(current) != parser_->struct_definitions_.end()) {
                return current;
            }
            return "";
        }
        
        // 次がtypedef型かチェック
        if (parser_->typedef_map_.find(next) != parser_->typedef_map_.end()) {
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
        if (parser_->struct_definitions_.find(struct_name) != parser_->struct_definitions_.end()) {
            return current; // "struct StructName"のまま返す
        }
    }
    
    // 構造体型かチェック（裸の構造体名）
    if (parser_->struct_definitions_.find(current) != parser_->struct_definitions_.end()) {
        return current; // 構造体名をそのまま返す
    }
    
    // enum型かチェック（裸のenum名）
    if (parser_->enum_definitions_.find(current) != parser_->enum_definitions_.end()) {
        return current; // enum名をそのまま返す
    }
    
    // 配列型かチェック（int[2], int[2][2]など）
    if (current.find('[') != std::string::npos) {
        return current; // 配列型名をそのまま返す
    }
    
    // ユニオン型かチェック（裸のユニオン名）
    if (parser_->union_definitions_.find(current) != parser_->union_definitions_.end()) {
        return current; // ユニオン名をそのまま返す
    }
    
    // 未定義型の場合は空文字列を返す
    return "";
}

// 型名から基本型部分を抽出する

std::string TypeUtilityParser::extractBaseType(const std::string& type_name) {
    // 配列部分 [n] を除去して基本型を取得
    size_t bracket_pos = type_name.find('[');
    if (bracket_pos != std::string::npos) {
        return type_name.substr(0, bracket_pos);
    }
    return type_name;
}

bool TypeUtilityParser::detectCircularReference(const std::string& struct_name, 
                                             const std::string& member_type,
                                             std::unordered_set<std::string>& visited,
                                             std::vector<std::string>& path) {
    // 型名を正規化（"struct " プレフィックスを除去）
    std::string normalized_type = member_type;
    if (normalized_type.rfind("struct ", 0) == 0) {
        normalized_type = normalized_type.substr(7);
    }
    
    // 構造体型でなければ循環なし
    if (parser_->struct_definitions_.find(normalized_type) == parser_->struct_definitions_.end()) {
        return false;
    }
    
    // 前方宣言のみの構造体はスキップ（定義が後で来る可能性がある）
    const StructDefinition& struct_def = parser_->struct_definitions_[normalized_type];
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
