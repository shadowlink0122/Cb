// generic_type_parser.h
// ジェネリック型のパース用ヘルパー関数

#ifndef GENERIC_TYPE_PARSER_H
#define GENERIC_TYPE_PARSER_H

#include <string>
#include <vector>

namespace GenericTypeParser {

// ジェネリック型のパース結果
struct ParsedGenericType {
    std::string base_name;                // 例: "MapNode", "Map"
    std::vector<std::string> type_params; // 例: ["K", "V"], ["int", "string"]
    bool is_pointer;                      // *で終わるか
    int pointer_depth;                    // ポインタの深さ

    ParsedGenericType() : is_pointer(false), pointer_depth(0) {}
};

// ジェネリック型文字列をパースする
// 例: "MapNode<K, V>*" -> base_name="MapNode", type_params=["K", "V"],
// is_pointer=true 例: "Map<int, Vector<double>>*" -> base_name="Map",
// type_params=["int", "Vector<double>"] 例: "Tuple<int, string, bool>" ->
// base_name="Tuple", type_params=["int", "string", "bool"]
inline ParsedGenericType parse_generic_type(const std::string &type_str) {
    ParsedGenericType result;

    if (type_str.empty()) {
        return result;
    }

    std::string trimmed = type_str;

    // 末尾のスペースを削除
    while (!trimmed.empty() && trimmed.back() == ' ') {
        trimmed.pop_back();
    }

    // ポインタ記号を処理
    while (!trimmed.empty() && trimmed.back() == '*') {
        result.is_pointer = true;
        result.pointer_depth++;
        trimmed.pop_back();
        // *の前のスペースも削除
        while (!trimmed.empty() && trimmed.back() == ' ') {
            trimmed.pop_back();
        }
    }

    // ジェネリック型パラメータがあるかチェック
    size_t angle_open = trimmed.find('<');
    if (angle_open == std::string::npos) {
        // ジェネリックでない場合
        result.base_name = trimmed;
        return result;
    }

    // ベース名を抽出
    result.base_name = trimmed.substr(0, angle_open);

    // 閉じ括弧を探す（ネストに対応）
    size_t angle_close = trimmed.rfind('>');
    if (angle_close == std::string::npos || angle_close <= angle_open) {
        // 不正な形式
        result.base_name = trimmed;
        result.type_params.clear();
        return result;
    }

    // パラメータ部分を抽出
    std::string params =
        trimmed.substr(angle_open + 1, angle_close - angle_open - 1);

    // パラメータを分割（ネストした<>を考慮）
    size_t start = 0;
    int depth = 0;
    for (size_t i = 0; i < params.size(); ++i) {
        char c = params[i];

        if (c == '<') {
            depth++;
        } else if (c == '>') {
            depth--;
        } else if (c == ',' && depth == 0) {
            // カンマで分割
            std::string param = params.substr(start, i - start);

            // 先頭と末尾の空白を削除
            while (!param.empty() && param.front() == ' ') {
                param.erase(param.begin());
            }
            while (!param.empty() && param.back() == ' ') {
                param.pop_back();
            }

            if (!param.empty()) {
                result.type_params.push_back(param);
            }

            start = i + 1;
        }
    }

    // 最後のパラメータを追加
    if (start < params.size()) {
        std::string param = params.substr(start);

        // 先頭と末尾の空白を削除
        while (!param.empty() && param.front() == ' ') {
            param.erase(param.begin());
        }
        while (!param.empty() && param.back() == ' ') {
            param.pop_back();
        }

        if (!param.empty()) {
            result.type_params.push_back(param);
        }
    }

    return result;
}

// ジェネリック型かどうかをチェック
inline bool is_generic_type(const std::string &type_str) {
    return type_str.find('<') != std::string::npos;
}

// ポインタ型かどうかをチェック（ジェネリック型も含む）
inline bool is_pointer_type(const std::string &type_str) {
    size_t star_pos = type_str.find('*');
    return star_pos != std::string::npos;
}

} // namespace GenericTypeParser

#endif // GENERIC_TYPE_PARSER_H
