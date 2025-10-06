#pragma once
#include "../../../common/ast.h"
#include <string>
#include <cstdint>

// 前方宣言
class ExpressionEvaluator;
class Interpreter;

// ============================================================================
// 式評価のヘルパー関数群
// ============================================================================
// ExpressionEvaluatorから分離されたヘルパーメソッド群
// Tier 2リファクタリングで抽出されたメソッドをここに集約
// ============================================================================

namespace ExpressionHelpers {

// ============================================================================
// 二項演算のヘルパー
// ============================================================================

// 算術演算（+, -, *, /, %）の評価
int64_t evaluate_arithmetic_binary(const std::string& op, int64_t left, int64_t right);

// 比較演算（<, >, <=, >=, ==, !=）の評価
int64_t evaluate_comparison_binary(const std::string& op, int64_t left, int64_t right);

// 論理演算（&&, ||）の評価
int64_t evaluate_logical_binary(const std::string& op, int64_t left, int64_t right);

// ビット演算（&, |, ^, <<, >>）の評価
int64_t evaluate_bitwise_binary(const std::string& op, int64_t left, int64_t right);

// ============================================================================
// リテラル評価のヘルパー
// ============================================================================

// 数値リテラル（整数・浮動小数点）の評価
int64_t evaluate_number_literal(const ASTNode* node);

// 特殊なリテラル（nullptr, 文字列リテラル）の評価
int64_t evaluate_special_literal(const ASTNode* node);

// ============================================================================
// インクリメント/デクリメントのヘルパー
// ============================================================================

// 前置インクリメント/デクリメント（++x, --x）
int64_t evaluate_prefix_incdec(const ASTNode* node, Interpreter& interpreter);

// 後置インクリメント/デクリメント（x++, x--）
int64_t evaluate_postfix_incdec(const ASTNode* node, Interpreter& interpreter);

// ============================================================================
// 単項演算のヘルパー
// ============================================================================

// 単純な単項演算（+, -, !, ~）
int64_t evaluate_simple_unary(const std::string& op, int64_t operand);

} // namespace ExpressionHelpers
