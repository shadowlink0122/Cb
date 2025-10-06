// Type Parser - 型解析を担当
// Phase 2-3: RecursiveParserへの委譲実装 + ドキュメント化
//
// このファイルは、型情報の解析と検証を担当します。
//
// 【サポートする型】:
// 1. 基本型: int, float, string, bool, void
// 2. 配列型: int[10], float[5][3]
// 3. ポインタ型: int*, int**, int***
// 4. 構造体型: struct Point
// 5. Enum型: enum Color
// 6. Typedef型: カスタム型エイリアス
//
#include "type_parser.h"
#include "../recursive_parser.h"

TypeParser::TypeParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// ========================================
// 型解析のエントリーポイント
// ========================================

/**
 * @brief 型を解析してParsedTypeInfo構造体を返す
 * @return 解析された型情報
 * 
 * サポートする型構文:
 * - 基本型: int, float, string, bool, void, long long, unsigned
 * - 配列型: int[10], int[5][3]（多次元配列）
 * - ポインタ型: int*, int**, int***（任意レベル）
 * - 構造体型: struct Point, Point（typedefされた場合）
 * - Enum型: enum Color, Color（typedefされた場合）
 * - Typedef型: カスタムエイリアス
 * 
 * 機能:
 * - typedef解決: カスタム型を基本型に変換
 * - 配列次元の解析
 * - ポインタレベルの計算
 * - 型の存在チェック
 * 
 * 注意: parseType()は内部でlast_parsed_type_info_を更新するため、
 * 解析後にgetLastParsedTypeInfo()で詳細情報を取得できます。
 */
ParsedTypeInfo TypeParser::parseType() {
    // RecursiveParser::parseType()は型名の文字列を返す
    // ParsedTypeInfoはlast_parsed_type_info_として内部に保存される
    parser_->parseType();
    return parser_->getLastParsedTypeInfo();
}

// ========================================
// 型解決
// ========================================

/**
 * @brief 解析済みの型情報を解決する
 * @param parsed 解析された型情報
 * @return 解決された型情報（typedef展開後）
 * 
 * Typedefエイリアスを実際の型に変換します。
 * 
 * 例:
 * - typedef int MyInt; → MyInt は int に解決
 * - typedef int[10] IntArray; → IntArray は int[10] に解決
 * - typedef struct Point Point; → Point は struct Point に解決
 * 
 * 再帰的解決:
 * typedef A = B;
 * typedef B = int;
 * → A は最終的に int に解決される
 */
TypeInfo TypeParser::resolveParsedTypeInfo(const ParsedTypeInfo& parsed) {
    return parser_->resolveParsedTypeInfo(parsed);
}

/**
 * @brief 配列型を解決する
 * @param base_type ベース型名
 * @param dimensions 配列の次元情報（サイズと動的フラグのペア）
 * @return 解決された配列型の文字列表現
 * 
 * 配列のtypedef型を解決します。
 * 
 * 例:
 * typedef int[10] IntArray;
 * IntArray arr; → int[10] arr; に解決
 * 
 * 多次元配列の場合:
 * typedef int[5][10] Matrix;
 * Matrix m; → int[5][10] m; に解決
 * 
 * 注意: この機能は現在Phase 2では委譲のみで実装されていません。
 * Phase 5以降で実装を移行予定です。
 */
std::string TypeParser::resolveArrayType(
    const std::string& base_type,
    const std::vector<std::pair<int, bool>>& dimensions
) {
    // TODO: Phase 5で実装を移行
    // 現時点ではダミー実装
    return base_type;
}

// ========================================
// 型チェック
// ========================================

/**
 * @brief ポインタのレベルを取得
 * @param type_info 型情報
 * @return ポインタのレベル（0 = 値型、1 = ポインタ、2 = ダブルポインタ...）
 * 
 * 例:
 * - int → 0
 * - int* → 1
 * - int** → 2
 * - int*** → 3
 */
int TypeParser::getPointerLevel(const ParsedTypeInfo& type_info) {
    // ParsedTypeInfoに含まれるpointer_depthを返す
    return type_info.pointer_depth;
}

/**
 * @brief 型が有効かどうかをチェック
 * @param type_name 型名
 * @return 型が有効ならtrue
 * 
 * 以下をチェックします:
 * - 基本型が存在するか
 * - 構造体/Enumが定義されているか
 * - Typedef型が解決可能か
 * 
 * 注意: 現在Phase 2では委譲のみで実装されていません。
 * Phase 5以降で実装を移行予定です。
 */
bool TypeParser::isValidType(const std::string& type_name) {
    // TODO: Phase 5で実装を移行
    return true;  // ダミー実装
}

/**
 * @brief 構造体型かどうかをチェック
 * @param type_name 型名
 * @return 構造体型ならtrue
 * 
 * 構造体型の判定:
 * - struct キーワードが付いている
 * - または typedef struct として定義されている
 * 
 * 注意: 現在Phase 2では委譲のみで実装されていません。
 * Phase 5以降で実装を移行予定です。
 */
bool TypeParser::isStructType(const std::string& type_name) {
    // TODO: Phase 5で実装を移行
    return type_name.find("struct") != std::string::npos;  // ダミー実装
}

/**
 * @brief Enum型かどうかをチェック
 * @param type_name 型名
 * @return Enum型ならtrue
 * 
 * Enum型の判定:
 * - enum キーワードが付いている
 * - または typedef enum として定義されている
 * 
 * 注意: 現在Phase 2では委譲のみで実装されていません。
 * Phase 5以降で実装を移行予定です。
 */
bool TypeParser::isEnumType(const std::string& type_name) {
    // TODO: Phase 5で実装を移行
    return type_name.find("enum") != std::string::npos;  // ダミー実装
}
