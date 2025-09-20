#pragma once
#include <cstdint>
#include <string>

namespace utf8_utils {

/**
 * UTF-8バイト数を取得
 * @param byte 最初のバイト
 * @return バイト数（1-4）
 */
int utf8_char_length(unsigned char byte);

/**
 * UTF-8文字列の文字数をカウント
 * @param str UTF-8文字列
 * @return 文字数
 */
size_t utf8_char_count(const std::string &str);

/**
 * UTF-8文字列の指定位置の文字を取得
 * @param str UTF-8文字列
 * @param index 文字位置（0ベース）
 * @return 指定位置の文字（UTF-8）
 */
std::string utf8_char_at(const std::string &str, size_t index);

/**
 * UTF-8文字を整数に変換
 * @param utf8_char UTF-8文字（1文字）
 * @return 文字コード
 */
int64_t utf8_char_to_int(const std::string &utf8_char);

} // namespace utf8_utils
