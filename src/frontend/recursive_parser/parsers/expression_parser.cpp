// Expression Parser - 式解析を担当
// Phase 2-3: RecursiveParserへの委譲実装 + ドキュメント化
//
// このファイルは、式（Expression）の解析を担当します。
// 演算子の優先順位に従って、トップダウン方式（再帰下降法）で解析します。
//
// 【演算子の優先順位（高い順）】:
//   1. Primary (リテラル、変数、括弧、配列リテラル、構造体リテラル)
//   2. Postfix (配列アクセス[], 関数呼び出し(), メンバーアクセス., アロー->,
//   後置++/--)
//   3. Unary (単項演算子: !, -, ~, &, *, 前置++/--)
//   4. Multiplicative (*, /, %)
//   5. Additive (+, -)
//   6. Shift (<<, >>)
//   7. Comparison (<, >, <=, >=, ==, !=)
//   8. Bitwise AND (&)
//   9. Bitwise XOR (^)
//   10. Bitwise OR (|)
//   11. Logical AND (&&)
//   12. Logical OR (||)
//   13. Ternary (?:)
//   14. Assignment (=, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=)
//
// 【設計パターン】:
// - 委譲パターン: 現時点ではRecursiveParserの実装を呼び出す
// - friend宣言: RecursiveParserの内部状態にアクセス可能
// - 将来の拡張: Phase 3以降で完全な実装に置き換え予定
//
#include "expression_parser.h"
#include "../recursive_parser.h"
#include "primary_expression_parser.h"
#include "src/common/debug.h"
#include <cstdio>

ExpressionParser::ExpressionParser(RecursiveParser *parser)
    : parser_(parser), primary_expression_parser_(
                           std::make_unique<PrimaryExpressionParser>(parser)) {}

// ========================================
// 最上位の式解析
// ========================================

/**
 * @brief 式解析のエントリーポイント
 * @return 解析されたAST式ノード
 *
 * 式の最上位から解析を開始します。
 * 実際には代入式（parseAssignment）を呼び出します。
 */
ASTNode *ExpressionParser::parseExpression() { return parseAssignment(); }

// ========================================
// 代入式（優先順位: 最低）
// ========================================

/**
 * @brief 代入式を解析
 * @return 解析されたAST代入ノード
 *
 * 代入演算子:
 * - 単純代入: =
 * - 複合代入: +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
 *
 * 複合代入は、自動的に二項演算に展開されます。
 * 例: a += 5  →  a = a + 5
 */
ASTNode *ExpressionParser::parseAssignment() {
    ASTNode *left = parseTernary();

    auto getBinaryOpForCompound = [](TokenType op_type) -> std::string {
        switch (op_type) {
        case TokenType::TOK_PLUS_ASSIGN:
            return "+";
        case TokenType::TOK_MINUS_ASSIGN:
            return "-";
        case TokenType::TOK_MUL_ASSIGN:
            return "*";
        case TokenType::TOK_DIV_ASSIGN:
            return "/";
        case TokenType::TOK_MOD_ASSIGN:
            return "%";
        case TokenType::TOK_AND_ASSIGN:
            return "&";
        case TokenType::TOK_OR_ASSIGN:
            return "|";
        case TokenType::TOK_XOR_ASSIGN:
            return "^";
        case TokenType::TOK_LSHIFT_ASSIGN:
            return "<<";
        case TokenType::TOK_RSHIFT_ASSIGN:
            return ">>";
        default:
            return "";
        }
    };

    // 通常の代入と複合代入演算子をチェック
    if (parser_->check(TokenType::TOK_ASSIGN) ||
        parser_->check(TokenType::TOK_PLUS_ASSIGN) ||
        parser_->check(TokenType::TOK_MINUS_ASSIGN) ||
        parser_->check(TokenType::TOK_MUL_ASSIGN) ||
        parser_->check(TokenType::TOK_DIV_ASSIGN) ||
        parser_->check(TokenType::TOK_MOD_ASSIGN) ||
        parser_->check(TokenType::TOK_AND_ASSIGN) ||
        parser_->check(TokenType::TOK_OR_ASSIGN) ||
        parser_->check(TokenType::TOK_XOR_ASSIGN) ||
        parser_->check(TokenType::TOK_LSHIFT_ASSIGN) ||
        parser_->check(TokenType::TOK_RSHIFT_ASSIGN)) {

        TokenType op_type = parser_->current_token_.type;
        std::string op_value = parser_->current_token_.value;
        parser_->advance(); // consume assignment operator

        ASTNode *right = parseAssignment(); // Right associative

        ASTNode *assign = new ASTNode(ASTNodeType::AST_ASSIGN);

        // leftが変数か配列参照でない場合はエラー
        if (left->node_type == ASTNodeType::AST_VARIABLE) {
            // 複合代入の場合、a += b を a = a + b に変換
            if (op_type != TokenType::TOK_ASSIGN) {
                std::string binary_op = getBinaryOpForCompound(op_type);

                // a = a op b の形に変換
                ASTNode *var_ref = new ASTNode(ASTNodeType::AST_VARIABLE);
                var_ref->name = left->name;

                ASTNode *binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                binop->op = binary_op;
                binop->left = std::unique_ptr<ASTNode>(var_ref);
                binop->right = std::unique_ptr<ASTNode>(right);

                assign->name = left->name;
                assign->right = std::unique_ptr<ASTNode>(binop);
            } else {
                assign->name = left->name;
                assign->right = std::unique_ptr<ASTNode>(right);
            }
            delete left; // leftノードはもう不要
        } else if (left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 配列要素への複合代入の場合
            if (op_type != TokenType::TOK_ASSIGN) {
                std::string binary_op = getBinaryOpForCompound(op_type);

                // arr[i] = arr[i] op b の形に変換
                ASTNode *array_ref_copy =
                    new ASTNode(ASTNodeType::AST_ARRAY_REF);
                // 配列参照をコピー（左辺と右辺で同じものを参照）
                ASTNode *var_copy = new ASTNode(ASTNodeType::AST_VARIABLE);
                var_copy->name = static_cast<ASTNode *>(left->left.get())->name;
                array_ref_copy->left = std::unique_ptr<ASTNode>(var_copy);

                // インデックス式をディープコピー
                ASTNode *index_copy = nullptr;
                if (left->array_index) {
                    // 簡単なケースのみサポート（変数やリテラル）
                    if (left->array_index->node_type ==
                        ASTNodeType::AST_VARIABLE) {
                        index_copy = new ASTNode(ASTNodeType::AST_VARIABLE);
                        index_copy->name = left->array_index->name;
                    } else if (left->array_index->node_type ==
                               ASTNodeType::AST_NUMBER) {
                        index_copy = new ASTNode(ASTNodeType::AST_NUMBER);
                        index_copy->int_value = left->array_index->int_value;
                    }
                }
                array_ref_copy->array_index =
                    std::unique_ptr<ASTNode>(index_copy);

                ASTNode *binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                binop->op = binary_op;
                binop->left = std::unique_ptr<ASTNode>(array_ref_copy);
                binop->right = std::unique_ptr<ASTNode>(right);

                assign->left = std::unique_ptr<ASTNode>(left);
                assign->right = std::unique_ptr<ASTNode>(binop);
            } else {
                assign->left = std::unique_ptr<ASTNode>(left);
                assign->right = std::unique_ptr<ASTNode>(right);
            }
        } else if (left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // メンバアクセス代入: obj.member = value
            if (op_type != TokenType::TOK_ASSIGN) {
                std::string binary_op = getBinaryOpForCompound(op_type);
                ASTNode *left_copy = parser_->cloneAstNode(left);
                assign->left = std::unique_ptr<ASTNode>(left);

                ASTNode *binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                binop->op = binary_op;
                binop->left = std::unique_ptr<ASTNode>(left_copy);
                binop->right = std::unique_ptr<ASTNode>(right);

                assign->right = std::unique_ptr<ASTNode>(binop);
            } else {
                assign->left = std::unique_ptr<ASTNode>(left);
                assign->right = std::unique_ptr<ASTNode>(right);
            }
        } else if (left->node_type == ASTNodeType::AST_ARROW_ACCESS) {
            // アロー演算子代入: ptr->member = value
            if (op_type != TokenType::TOK_ASSIGN) {
                std::string binary_op = getBinaryOpForCompound(op_type);
                ASTNode *left_copy = parser_->cloneAstNode(left);
                assign->left = std::unique_ptr<ASTNode>(left);

                ASTNode *binop = new ASTNode(ASTNodeType::AST_BINARY_OP);
                binop->op = binary_op;
                binop->left = std::unique_ptr<ASTNode>(left_copy);
                binop->right = std::unique_ptr<ASTNode>(right);

                assign->right = std::unique_ptr<ASTNode>(binop);
            } else {
                assign->left = std::unique_ptr<ASTNode>(left);
                assign->right = std::unique_ptr<ASTNode>(right);
            }
        } else if (left->node_type == ASTNodeType::AST_UNARY_OP &&
                   left->op == "DEREFERENCE") {
            // 間接参照への代入: *ptr = value
            // 複合代入はサポートしない（*ptr += value は未実装）
            if (op_type != TokenType::TOK_ASSIGN) {
                parser_->error("Compound assignment to dereferenced pointer is "
                               "not yet supported");
                return nullptr;
            }
            assign->left = std::unique_ptr<ASTNode>(left);
            assign->right = std::unique_ptr<ASTNode>(right);
        } else {
            parser_->error("Invalid assignment target");
            return nullptr;
        }

        return assign;
    }

    return left;
}

// ========================================
// 三項演算子（優先順位: 13）
// ========================================

/**
 * @brief 三項演算子 (条件演算子) を解析
 * @return 解析されたAST三項演算ノード
 *
 * 構文: condition ? true_value : false_value
 */
ASTNode *ExpressionParser::parseTernary() { return parser_->parseTernary(); }

// ========================================
// 論理演算子（優先順位: 11-12）
// ========================================

/**
 * @brief 論理OR演算子 (||) を解析
 * @return 解析されたAST二項演算ノード
 *
 * 短絡評価を行います（左辺がtrueなら右辺を評価しない）
 */
ASTNode *ExpressionParser::parseLogicalOr() {
    ASTNode *left = parseLogicalAnd();

    while (parser_->check(TokenType::TOK_OR)) {
        Token op = parser_->advance();
        ASTNode *right = parseLogicalAnd();

        ASTNode *binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);

        left = binary;
    }

    return left;
}

/**
 * @brief 論理AND演算子 (&&) を解析
 * @return 解析されたAST二項演算ノード
 *
 * 短絡評価を行います（左辺がfalseなら右辺を評価しない）
 */
ASTNode *ExpressionParser::parseLogicalAnd() {
    ASTNode *left = parseBitwiseOr();

    while (parser_->check(TokenType::TOK_AND)) {
        Token op = parser_->advance();
        ASTNode *right = parseBitwiseOr();

        ASTNode *binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);

        left = binary;
    }

    return left;
}

// ========================================
// ビット演算子（優先順位: 8-10）
// ========================================

/**
 * @brief ビットOR演算子 (|) を解析
 * @return 解析されたAST二項演算ノード
 */
ASTNode *ExpressionParser::parseBitwiseOr() {
    ASTNode *left = parseBitwiseXor();

    while (parser_->check(TokenType::TOK_BIT_OR)) {
        Token op = parser_->advance();
        ASTNode *right = parseBitwiseXor();

        ASTNode *binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);

        left = binary;
    }

    return left;
}

/**
 * @brief ビットXOR演算子 (^) を解析
 * @return 解析されたAST二項演算ノード
 */
ASTNode *ExpressionParser::parseBitwiseXor() {
    ASTNode *left = parseBitwiseAnd();

    while (parser_->check(TokenType::TOK_BIT_XOR)) {
        Token op = parser_->advance();
        ASTNode *right = parseBitwiseAnd();

        ASTNode *binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);

        left = binary;
    }

    return left;
}

/**
 * @brief ビットAND演算子 (&) を解析
 * @return 解析されたAST二項演算ノード
 *
 * 注意: アドレス演算子(&)とは異なります
 */
ASTNode *ExpressionParser::parseBitwiseAnd() {
    ASTNode *left = parseComparison();

    while (parser_->check(TokenType::TOK_BIT_AND)) {
        Token op = parser_->advance();
        ASTNode *right = parseComparison();

        ASTNode *binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);

        left = binary;
    }

    return left;
}

// ========================================
// 比較演算子（優先順位: 7）
// ========================================

/**
 * @brief 比較演算子を解析
 * @return 解析されたAST二項演算ノード
 *
 * サポートする演算子:
 * - ==, != (等価比較)
 * - <, >, <=, >= (大小比較)
 */
ASTNode *ExpressionParser::parseComparison() {
    ASTNode *left = parseShift();

    while (parser_->check(TokenType::TOK_EQ) ||
           parser_->check(TokenType::TOK_NE) ||
           parser_->check(TokenType::TOK_LT) ||
           parser_->check(TokenType::TOK_LE) ||
           parser_->check(TokenType::TOK_GT) ||
           parser_->check(TokenType::TOK_GE)) {
        Token op = parser_->advance();
        ASTNode *right = parseShift();

        ASTNode *binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        if (parser_->debug_mode_) {
            std::fprintf(stderr,
                         "[EXPR_DEBUG] comparison op=%s left=%p right=%p\n",
                         op.value.c_str(), static_cast<void *>(left),
                         static_cast<void *>(right));
            std::fflush(stderr);
        }
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);

        left = binary;
    }

    return left;
}

// ========================================
// シフト演算子（優先順位: 6）
// ========================================

/**
 * @brief ビットシフト演算子を解析
 * @return 解析されたAST二項演算ノード
 *
 * サポートする演算子:
 * - << (左シフト)
 * - >> (右シフト)
 */
ASTNode *ExpressionParser::parseShift() {
    ASTNode *left = parseAdditive();

    while (parser_->check(TokenType::TOK_LEFT_SHIFT) ||
           parser_->check(TokenType::TOK_RIGHT_SHIFT)) {
        Token op = parser_->advance();
        ASTNode *right = parseAdditive();

        ASTNode *binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);

        left = binary;
    }

    return left;
}

// ========================================
// 加減算（優先順位: 5）
// ========================================

/**
 * @brief 加算・減算演算子を解析
 * @return 解析されたAST二項演算ノード
 *
 * サポートする演算子: +, -
 *
 * 左結合の二項演算子として処理します。
 * 例: a + b - c は (a + b) - c として解析されます。
 */
ASTNode *ExpressionParser::parseAdditive() {
    ASTNode *left = parseMultiplicative();

    while (parser_->check(TokenType::TOK_PLUS) ||
           parser_->check(TokenType::TOK_MINUS)) {
        Token op = parser_->advance();
        ASTNode *right = parseMultiplicative();

        ASTNode *binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);

        left = binary;
    }

    return left;
}

// ========================================
// 乗除算（優先順位: 4）
// ========================================

/**
 * @brief 乗算・除算・剰余演算子を解析
 * @return 解析されたAST二項演算ノード
 *
 * サポートする演算子: *, /, %
 *
 * 左結合の二項演算子として処理します。
 * 例: a * b / c は (a * b) / c として解析されます。
 */
ASTNode *ExpressionParser::parseMultiplicative() {
    ASTNode *left = parseUnary();

    while (parser_->check(TokenType::TOK_MUL) ||
           parser_->check(TokenType::TOK_DIV) ||
           parser_->check(TokenType::TOK_MOD)) {
        Token op = parser_->advance();
        ASTNode *right = parseUnary();

        ASTNode *binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);

        left = binary;
    }

    return left;
}

// ========================================
// 単項演算子（優先順位: 3）
// ========================================

/**
 * @brief 単項演算子を解析
 * @return 解析されたAST単項演算ノード
 *
 * サポートする演算子:
 * - ! (論理NOT)
 * - - (符号反転)
 * - ~ (ビットNOT)
 * - & (アドレス演算子)
 * - * (間接参照演算子)
 * - ++, -- (前置インクリメント・デクリメント)
 */
ASTNode *ExpressionParser::parseUnary() {
    // v0.12.0: await式のパース
    if (parser_->check(TokenType::TOK_AWAIT)) {
        parser_->advance(); // consume 'await'
        ASTNode *operand = parseUnary();

        ASTNode *await_node = new ASTNode(ASTNodeType::AST_UNARY_OP);
        await_node->op = "await";
        await_node->is_await_expression = true;
        await_node->left = std::unique_ptr<ASTNode>(operand);

        return await_node;
    }

    // Prefix operators: !, -, ~, &, *
    if (parser_->check(TokenType::TOK_NOT) ||
        parser_->check(TokenType::TOK_MINUS) ||
        parser_->check(TokenType::TOK_BIT_NOT) ||
        parser_->check(TokenType::TOK_BIT_AND) ||
        parser_->check(TokenType::TOK_MUL)) {
        Token op = parser_->advance();

        // & 演算子の場合、関数アドレス取得かチェック
        ASTNode *operand = parseUnary();

        ASTNode *unary = new ASTNode(ASTNodeType::AST_UNARY_OP);
        // & はアドレス演算子、* は間接参照演算子として扱う
        if (op.type == TokenType::TOK_BIT_AND) {
            unary->op = "ADDRESS_OF"; // アドレス演算子

            // operandから識別子名を取得して is_function_address フラグを設定
            // インタプリタ側で関数か変数かを判断する
            if (operand) {
                if (operand->node_type == ASTNodeType::AST_VARIABLE ||
                    operand->node_type == ASTNodeType::AST_IDENTIFIER) {
                    unary->is_function_address = true;
                    unary->function_address_name = operand->name;
                } else if (operand->node_type == ASTNodeType::AST_ARRAY_REF &&
                           operand->left &&
                           operand->left->node_type ==
                               ASTNodeType::AST_VARIABLE) {
                    // &arr[0] の場合、arr の名前を保存
                    unary->is_function_address = true;
                    unary->function_address_name = operand->left->name;
                }
            }
        } else if (op.type == TokenType::TOK_MUL) {
            unary->op = "DEREFERENCE"; // 間接参照演算子
        } else {
            unary->op = op.value;
        }
        unary->left = std::unique_ptr<ASTNode>(operand);

        return unary;
    }

    // ++ と -- は別処理: AST_PRE_INCDEC を生成
    if (parser_->check(TokenType::TOK_INCR) ||
        parser_->check(TokenType::TOK_DECR)) {
        Token op = parser_->advance();
        ASTNode *operand =
            parsePostfix(); // parsePostfix()を直接呼ぶことでメンバーアクセスを取得

        // AST_PRE_INCDEC ノードを生成
        ASTNode *incdec = new ASTNode(ASTNodeType::AST_PRE_INCDEC);
        incdec->op = op.value;
        incdec->left = std::unique_ptr<ASTNode>(operand);

        return incdec;
    }

    return parsePostfix();
}

// ========================================
// 後置演算子（優先順位: 2）
// ========================================

/**
 * @brief 後置演算子を解析
 * @return 解析されたASTノード
 *
 * サポートする演算子:
 * - [] (配列アクセス)
 * - () (関数呼び出し)
 * - . (メンバーアクセス)
 * - -> (アロー演算子)
 * - ++, -- (後置インクリメント・デクリメント)
 */
ASTNode *ExpressionParser::parsePostfix() {
    ASTNode *primary = parsePrimary();

    if (!primary) {
        std::cerr << "[PARSER ERROR] parsePrimary returned null" << std::endl;
        return nullptr;
    }

    while (true) {
        // 関数ポインタ呼び出しのチェック: *ptr(args)
        // primaryが DEREFERENCE (*演算子) で、次が'('の場合
        if (parser_->check(TokenType::TOK_LPAREN) && primary &&
            primary->node_type == ASTNodeType::AST_UNARY_OP &&
            primary->op == "DEREFERENCE") {

            // 関数ポインタ呼び出しに変換
            parser_->advance(); // '(' を消費

            ASTNode *funcPtrCall = new ASTNode(ASTNodeType::AST_FUNC_PTR_CALL);
            funcPtrCall->left =
                std::move(primary->left); // ポインタ変数（*の対象）

            // 引数の解析
            if (!parser_->check(TokenType::TOK_RPAREN)) {
                do {
                    ASTNode *arg = parser_->parseExpression();
                    funcPtrCall->arguments.push_back(
                        std::unique_ptr<ASTNode>(arg));
                } while (parser_->match(TokenType::TOK_COMMA));
            }

            parser_->consume(
                TokenType::TOK_RPAREN,
                "Expected ')' after function pointer call arguments");

            primary = funcPtrCall;
            continue;
        }

        if (parser_->check(TokenType::TOK_LBRACKET)) {
            // 配列アクセス: arr[i]
            parser_->advance(); // consume '['
            ASTNode *index = parser_->parseExpression();
            if (!index) {
                std::cerr << "[PARSER ERROR] parseExpression returned null for "
                             "array index"
                          << std::endl;
                return nullptr;
            }
            parser_->consume(TokenType::TOK_RBRACKET, "Expected ']'");

            ASTNode *array_ref = new ASTNode(ASTNodeType::AST_ARRAY_REF);
            array_ref->left = std::unique_ptr<ASTNode>(primary); // 左側を設定
            array_ref->array_index = std::unique_ptr<ASTNode>(index);

            // デバッグ: 配列アクセスノード作成をログ出力
            if (primary && primary->node_type == ASTNodeType::AST_VARIABLE) {
                debug_msg(DebugMsgId::PARSE_EXPR_ARRAY_ACCESS,
                          primary->name.c_str());
            } else if (primary &&
                       primary->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                std::string member_access_name =
                    primary->name + " (member access)";
                debug_msg(DebugMsgId::PARSE_EXPR_ARRAY_ACCESS,
                          member_access_name.c_str());
            } else if (primary &&
                       primary->node_type == ASTNodeType::AST_ARRAY_REF) {
                debug_msg(DebugMsgId::PARSE_EXPR_ARRAY_ACCESS,
                          "nested array access");
            }

            primary = array_ref; // 次のアクセスのベースとして設定
        } else if (parser_->check(TokenType::TOK_DOT)) {
            // メンバアクセス: obj.member
            primary = parseMemberAccess(primary);
            if (!primary) {
                std::cerr << "[PARSER ERROR] parseMemberAccess returned null"
                          << std::endl;
                return nullptr;
            }
        } else if (parser_->check(TokenType::TOK_ARROW)) {
            // アロー演算子: ptr->member
            primary = parseArrowAccess(primary);
            if (!primary) {
                std::cerr << "[PARSER ERROR] parseArrowAccess returned null"
                          << std::endl;
                return nullptr;
            }
        } else {
            break; // どちらでもない場合はループを抜ける
        }
    }

    // Postfix operators: ++, --
    if (parser_->check(TokenType::TOK_INCR) ||
        parser_->check(TokenType::TOK_DECR)) {
        Token op = parser_->advance();
        ASTNode *postfix = new ASTNode(ASTNodeType::AST_POST_INCDEC);
        postfix->op = op.value; // "++" または "--"
        postfix->left = std::unique_ptr<ASTNode>(primary);
        return postfix;
    }

    // NOTE: ? operator is NOT parsed here because it conflicts with ternary.
    // Error propagation (expr?) is checked AFTER all binary operators but
    // BEFORE ternary at the parseTernary level to avoid ambiguity.

    return primary;
}

// ========================================
// 基本式（優先順位: 1、最高）
// ========================================

/**
 * @brief 基本式（プライマリ式）を解析
 * @return 解析されたASTノード
 *
 * PrimaryExpressionParserに委譲します。
 * サポートする要素:
 * - 数値リテラル (整数、浮動小数点数)
 * - 文字列リテラル
 * - 文字リテラル
 * - 真偽値リテラル (true, false)
 * - nullptr
 * - 識別子（変数、関数）
 * - 括弧式 (expr)
 * - 配列リテラル [1, 2, 3]
 * - 構造体リテラル {member: value}
 * - enum値アクセス EnumName::member
 */
ASTNode *ExpressionParser::parsePrimary() {
    return primary_expression_parser_->parsePrimary();
}

// ========================================
// メンバーアクセス
// ========================================

/**
 * @brief メンバーアクセス演算子 (.) を解析
 * @param object アクセス対象のオブジェクト
 * @return 解析されたASTメンバーアクセスノード
 *
 * 構文: object.member
 * ネストしたアクセス可能: obj.member.submember
 */
ASTNode *ExpressionParser::parseMemberAccess(ASTNode *object) {
    parser_->consume(TokenType::TOK_DOT, "Expected '.'");

    std::string member_name;
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        member_name = parser_->current_token_.value;
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_PRINT) ||
               parser_->check(TokenType::TOK_PRINTLN) ||
               parser_->check(TokenType::TOK_PRINTF)) {
        // 予約キーワードだが、メソッド名として許可
        member_name = parser_->current_token_.value;
        parser_->advance();
    } else {
        parser_->error("Expected member name after '.'");
        return nullptr;
    }

    // メソッド呼び出しかチェック（obj.method()）
    if (parser_->check(TokenType::TOK_LPAREN)) {
        // メソッド呼び出し
        ASTNode *method_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
        method_call->name = member_name;
        method_call->left = std::unique_ptr<ASTNode>(object); // レシーバー
        parser_->setLocation(method_call, parser_->current_token_);

        // パラメータリストを解析
        parser_->advance(); // consume '('

        if (!parser_->check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode *arg = parser_->parseExpression();
                if (!arg) {
                    parser_->error("Expected argument expression");
                    return nullptr;
                }
                method_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));

                if (!parser_->check(TokenType::TOK_COMMA)) {
                    break;
                }
                parser_->advance(); // consume ','
            } while (!parser_->check(TokenType::TOK_RPAREN) &&
                     !parser_->isAtEnd());
        }

        parser_->consume(TokenType::TOK_RPAREN,
                         "Expected ')' after method arguments");

        return method_call;
    }

    // 通常のメンバアクセス (連続ドット処理はparsePostfixループに任せる)
    ASTNode *member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
    member_access->left = std::unique_ptr<ASTNode>(object);
    member_access->name = member_name; // メンバ名を保存
    parser_->setLocation(member_access, parser_->current_token_);

    return member_access;
}

/**
 * @brief アロー演算子 (->) を解析
 * @param object アクセス対象のポインタ
 * @return 解析されたASTアロー演算子ノード
 *
 * 構文: pointer->member
 * (*pointer).member の糖衣構文
 */
ASTNode *ExpressionParser::parseArrowAccess(ASTNode *object) {
    parser_->consume(TokenType::TOK_ARROW, "Expected '->'");

    std::string member_name;
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        member_name = parser_->current_token_.value;
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_PRINT) ||
               parser_->check(TokenType::TOK_PRINTLN) ||
               parser_->check(TokenType::TOK_PRINTF)) {
        // 予約キーワードだが、メソッド名として許可
        member_name = parser_->current_token_.value;
        parser_->advance();
    } else {
        parser_->error("Expected member name after '->'");
        return nullptr;
    }

    // メソッド呼び出しかチェック（ptr->method()）
    if (parser_->check(TokenType::TOK_LPAREN)) {
        // メソッド呼び出し
        ASTNode *method_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
        method_call->name = member_name;
        method_call->left =
            std::unique_ptr<ASTNode>(object); // レシーバー（ポインタ）
        method_call->is_arrow_call = true; // アロー演算子経由のフラグ
        parser_->setLocation(method_call, parser_->current_token_);

        // パラメータリストを解析
        parser_->advance(); // consume '('

        if (!parser_->check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode *arg = parser_->parseExpression();
                if (!arg) {
                    parser_->error("Expected argument expression");
                    return nullptr;
                }
                method_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));

                if (!parser_->check(TokenType::TOK_COMMA)) {
                    break;
                }
                parser_->advance(); // consume ','
            } while (!parser_->check(TokenType::TOK_RPAREN) &&
                     !parser_->isAtEnd());
        }

        parser_->consume(TokenType::TOK_RPAREN,
                         "Expected ')' after method arguments");

        return method_call;
    }

    // アロー演算子アクセス: ptr->member は (*ptr).member と等価
    ASTNode *arrow_access = new ASTNode(ASTNodeType::AST_ARROW_ACCESS);
    arrow_access->left = std::unique_ptr<ASTNode>(object);
    arrow_access->name = member_name; // メンバ名を保存
    parser_->setLocation(arrow_access, parser_->current_token_);

    return arrow_access;
}

// ========================================
// リテラル
// ========================================

/**
 * @brief 構造体リテラルを解析
 * @return 解析されたAST構造体リテラルノード
 *
 * PrimaryExpressionParserに委譲します。
 * 構文: {member1: value1, member2: value2, ...}
 * 末尾カンマ対応
 */
ASTNode *ExpressionParser::parseStructLiteral() {
    return primary_expression_parser_->parseStructLiteral();
}

/**
 * @brief 配列リテラルを解析
 * @return 解析されたAST配列リテラルノード
 *
 * PrimaryExpressionParserに委譲します。
 * 構文: [element1, element2, ...]
 * 空配列もサポート: []
 */
ASTNode *ExpressionParser::parseArrayLiteral() {
    return primary_expression_parser_->parseArrayLiteral();
}
