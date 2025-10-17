#pragma once
#include <string>
#include <vector>

namespace CbPreprocessor {

/**
 * マクロ定義の種類
 */
enum class MacroType {
    OBJECT_LIKE,  // #define PI 3.14
    FUNCTION_LIKE // #define SQUARE(x) ((x) * (x))
};

/**
 * マクロ定義を表す構造体
 *
 * 例:
 *   #define PI 3.14159
 *   #define SQUARE(x) ((x) * (x))
 *   #define MAX(a, b) ((a) > (b) ? (a) : (b))
 */
struct MacroDefinition {
    std::string name;                    // マクロ名
    MacroType type;                      // マクロの種類
    std::vector<std::string> parameters; // パラメータリスト（関数形式の場合）
    std::string body;                    // マクロ本体
    bool is_variadic;     // 可変長引数か（__VA_ARGS__、将来実装）
    int line;             // 定義された行番号
    std::string filename; // 定義されたファイル名

    MacroDefinition()
        : type(MacroType::OBJECT_LIKE), is_variadic(false), line(0) {}

    /**
     * 関数形式マクロかどうか判定
     */
    bool isFunctionLike() const { return type == MacroType::FUNCTION_LIKE; }

    /**
     * オブジェクト形式マクロかどうか判定
     */
    bool isObjectLike() const { return type == MacroType::OBJECT_LIKE; }

    /**
     * パラメータ数を取得
     */
    size_t getParameterCount() const { return parameters.size(); }

    /**
     * デバッグ用の文字列表現を取得
     */
    std::string toString() const {
        std::string result = "#define " + name;

        if (isFunctionLike()) {
            result += "(";
            for (size_t i = 0; i < parameters.size(); i++) {
                result += parameters[i];
                if (i < parameters.size() - 1) {
                    result += ", ";
                }
            }
            result += ")";
        }

        result += " " + body;
        return result;
    }
};

} // namespace CbPreprocessor
