// v0.14.0: HIR Generator Unit Test
//
// 【ユニットテストの目的】
// ========================================
// このテストはHIR（High-level Intermediate Representation）の
// 生成ロジックを詳細に検証します。
//
// テスト対象:
//   - ASTからHIRへの変換
//   - HIRノードの生成
//   - 型情報の伝播
//
// 統合テストとの違い:
//   - 統合テスト: Cb言語の機能をエンドツーエンドでテスト
//   - ユニットテスト: HIRなどの内部実装を詳細にテスト
//
// 詳細は tests/README.md を参照してください。
// ========================================

#include "../../../src/backend/ir/hir/hir_generator.h"
#include "../../../src/common/ast.h"
#include <cassert>
#include <iostream>
#include <memory>

// Test friend class to access private members
class HIRGeneratorTest {
  public:
    static cb::ir::hir::HIRExpr test_convert_expr(cb::ir::HIRGenerator &gen,
                                                  const ASTNode *node) {
        return gen.convert_expr(node);
    }

    static cb::ir::hir::HIRFunction
    test_convert_function(cb::ir::HIRGenerator &gen, const ASTNode *node) {
        return gen.convert_function(node);
    }
};

// テストカウンター
int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;

#define TEST_ASSERT(condition, message)                                        \
    do {                                                                       \
        total_tests++;                                                         \
        if (!(condition)) {                                                    \
            std::cerr << "[FAIL] " << message << std::endl;                    \
            std::cerr << "  at " << __FILE__ << ":" << __LINE__ << std::endl;  \
            failed_tests++;                                                    \
        } else {                                                               \
            passed_tests++;                                                    \
        }                                                                      \
    } while (0)

// テストヘルパー: 数値リテラルのASTノードを作成
std::unique_ptr<ASTNode> create_number_node(int value) {
    auto node = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    node->int_value = value;
    node->type_info = TYPE_INT;
    node->location = SourceLocation("test", 1, 1);
    return node;
}

// テストヘルパー: 変数参照のASTノードを作成
std::unique_ptr<ASTNode> create_variable_node(const std::string &name) {
    auto node = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    node->name = name;
    node->type_info = TYPE_INT;
    node->location = SourceLocation("test", 1, 1);
    return node;
}

// テスト1: リテラルの変換
void test_literal_conversion() {
    std::cout << "\n[TEST] Literal conversion" << std::endl;

    cb::ir::HIRGenerator gen;
    auto ast_node = create_number_node(42);
    auto hir_expr = HIRGeneratorTest::test_convert_expr(gen, ast_node.get());

    TEST_ASSERT(hir_expr.kind == cb::ir::hir::HIRExpr::ExprKind::Literal,
                "式の種類がLiteralであること");
    TEST_ASSERT(hir_expr.literal_value == "42", "リテラル値が42であること");

    std::cout << "[PASS] Literal conversion" << std::endl;
}

// テスト2: 変数参照の変換
void test_variable_conversion() {
    std::cout << "\n[TEST] Variable conversion" << std::endl;

    cb::ir::HIRGenerator gen;
    auto ast_node = create_variable_node("x");
    auto hir_expr = HIRGeneratorTest::test_convert_expr(gen, ast_node.get());

    TEST_ASSERT(hir_expr.kind == cb::ir::hir::HIRExpr::ExprKind::Variable,
                "式の種類がVariableであること");
    TEST_ASSERT(hir_expr.var_name == "x", "変数名がxであること");

    std::cout << "[PASS] Variable conversion" << std::endl;
}

// テスト3: 二項演算の変換
void test_binary_op_conversion() {
    std::cout << "\n[TEST] Binary operation conversion" << std::endl;

    cb::ir::HIRGenerator gen;

    // AST作成: 10 + 20
    auto left = create_number_node(10);
    auto right = create_number_node(20);

    auto binop = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    binop->op = "+";
    binop->type_info = TYPE_INT;
    binop->location = SourceLocation("test", 1, 1);
    binop->left = std::move(left);
    binop->right = std::move(right);

    // HIRに変換
    auto hir_expr = HIRGeneratorTest::test_convert_expr(gen, binop.get());

    TEST_ASSERT(hir_expr.kind == cb::ir::hir::HIRExpr::ExprKind::BinaryOp,
                "式の種類がBinaryOpであること");
    TEST_ASSERT(hir_expr.op == "+", "演算子が+であること");
    TEST_ASSERT(hir_expr.left != nullptr, "左辺が存在すること");
    TEST_ASSERT(hir_expr.right != nullptr, "右辺が存在すること");
    TEST_ASSERT(hir_expr.left->kind == cb::ir::hir::HIRExpr::ExprKind::Literal,
                "左辺がリテラルであること");
    TEST_ASSERT(hir_expr.right->kind == cb::ir::hir::HIRExpr::ExprKind::Literal,
                "右辺がリテラルであること");

    std::cout << "[PASS] Binary operation conversion" << std::endl;
}

// テスト4: 関数定義の変換
void test_function_conversion() {
    std::cout << "\n[TEST] Function definition conversion" << std::endl;

    cb::ir::HIRGenerator gen;

    // 関数定義のAST作成
    auto func = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    func->name = "add";
    func->type_info = TYPE_INT;
    func->location = SourceLocation("test", 1, 1);

    // パラメータ
    auto param1 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param1->name = "a";
    param1->type_info = TYPE_INT;

    auto param2 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param2->name = "b";
    param2->type_info = TYPE_INT;

    func->parameters.push_back(std::move(param1));
    func->parameters.push_back(std::move(param2));

    // 本体（空）
    auto body = std::make_unique<ASTNode>(ASTNodeType::AST_COMPOUND_STMT);
    func->body = std::move(body);

    // HIRに変換
    auto hir_func = HIRGeneratorTest::test_convert_function(gen, func.get());

    TEST_ASSERT(hir_func.name == "add", "関数名がaddであること");
    TEST_ASSERT(hir_func.parameters.size() == 2, "パラメータが2つであること");
    TEST_ASSERT(hir_func.parameters[0].name == "a",
                "第1パラメータ名がaであること");
    TEST_ASSERT(hir_func.parameters[1].name == "b",
                "第2パラメータ名がbであること");
    TEST_ASSERT(hir_func.body != nullptr, "関数本体が存在すること");

    std::cout << "[PASS] Function definition conversion" << std::endl;
}

// テスト5: プログラム全体の変換
void test_program_conversion() {
    std::cout << "\n[TEST] Program conversion" << std::endl;

    cb::ir::HIRGenerator gen;

    std::vector<std::unique_ptr<ASTNode>> statements;

    // 関数を2つ作成
    auto func1 = std::make_unique<ASTNode>();
    func1->node_type = ASTNodeType::AST_FUNC_DECL;
    func1->name = "foo";
    func1->type_info = TYPE_INT;
    func1->location = SourceLocation("test", 1, 1);
    auto body1 = std::make_unique<ASTNode>();
    body1->node_type = ASTNodeType::AST_COMPOUND_STMT;
    func1->body = std::move(body1);

    auto func2 = std::make_unique<ASTNode>();
    func2->node_type = ASTNodeType::AST_FUNC_DECL;
    func2->name = "bar";
    func2->type_info = TYPE_VOID;
    func2->location = SourceLocation("test", 5, 1);
    auto body2 = std::make_unique<ASTNode>();
    body2->node_type = ASTNodeType::AST_COMPOUND_STMT;
    func2->body = std::move(body2);

    statements.push_back(std::move(func1));
    statements.push_back(std::move(func2));

    // HIRプログラムに変換
    auto hir_program = gen.generate(statements);

    TEST_ASSERT(hir_program != nullptr, "HIRプログラムが生成されること");
    TEST_ASSERT(hir_program->functions.size() == 2, "関数が2つ含まれること");
    TEST_ASSERT(hir_program->functions[0].name == "foo",
                "第1関数名がfooであること");
    TEST_ASSERT(hir_program->functions[1].name == "bar",
                "第2関数名がbarであること");

    std::cout << "[PASS] Program conversion" << std::endl;
}

int main() {
    std::cout << "=== HIR Generator Unit Tests ===" << std::endl;
    std::cout << "Testing HIR generation from AST" << std::endl;

    try {
        test_literal_conversion();
        test_variable_conversion();
        test_binary_op_conversion();
        test_function_conversion();
        test_program_conversion();

        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total:  " << total_tests << std::endl;
        std::cout << "Passed: " << passed_tests << std::endl;
        std::cout << "Failed: " << failed_tests << std::endl;

        if (failed_tests == 0) {
            std::cout << "\n✓ All tests passed!" << std::endl;
            return 0;
        } else {
            std::cout << "\n✗ Some tests failed" << std::endl;
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << "\n[EXCEPTION] " << e.what() << std::endl;
        return 1;
    }
}
