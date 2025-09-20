#include "utf8_utils.h"

namespace utf8_utils {

int utf8_char_length(unsigned char byte) {
    if (byte < 0x80)
        return 1; // ASCII
    if ((byte >> 5) == 0x06)
        return 2; // 110xxxxx
    if ((byte >> 4) == 0x0E)
        return 3; // 1110xxxx
    if ((byte >> 3) == 0x1E)
        return 4; // 11110xxx
    return 1;     // 不正なバイトの場合は1バイトとして扱う
}

size_t utf8_char_count(const std::string &str) {
    size_t count = 0;
    for (size_t i = 0; i < str.size();) {
        int len = utf8_char_length(static_cast<unsigned char>(str[i]));
        i += len;
        count++;
    }
    return count;
}

std::string utf8_char_at(const std::string &str, size_t index) {
    size_t current_index = 0;
    for (size_t i = 0; i < str.size();) {
        int len = utf8_char_length(static_cast<unsigned char>(str[i]));
        if (current_index == index) {
            return str.substr(i, len);
        }
        i += len;
        current_index++;
    }
    return ""; // 範囲外の場合は空文字列を返す
}

int64_t utf8_char_to_int(const std::string &utf8_char) {
    if (utf8_char.empty()) {
        return 0;
    }

    unsigned char first_byte = static_cast<unsigned char>(utf8_char[0]);

    if (first_byte < 0x80) {
        // ASCII文字
        return static_cast<int64_t>(first_byte);
    } else if ((first_byte >> 5) == 0x06 && utf8_char.size() >= 2) {
        // 2バイト文字
        int64_t code = (first_byte & 0x1F) << 6;
        code |= (static_cast<unsigned char>(utf8_char[1]) & 0x3F);
        return code;
    } else if ((first_byte >> 4) == 0x0E && utf8_char.size() >= 3) {
        // 3バイト文字
        int64_t code = (first_byte & 0x0F) << 12;
        code |= (static_cast<unsigned char>(utf8_char[1]) & 0x3F) << 6;
        code |= (static_cast<unsigned char>(utf8_char[2]) & 0x3F);
        return code;
    } else if ((first_byte >> 3) == 0x1E && utf8_char.size() >= 4) {
        // 4バイト文字
        int64_t code = (first_byte & 0x07) << 18;
        code |= (static_cast<unsigned char>(utf8_char[1]) & 0x3F) << 12;
        code |= (static_cast<unsigned char>(utf8_char[2]) & 0x3F) << 6;
        code |= (static_cast<unsigned char>(utf8_char[3]) & 0x3F);
        return code;
    }

    return static_cast<int64_t>(first_byte); // フォールバック
}

} // namespace utf8_utils
