// Cb -> Cコード変換用メイン
#include "../src/common/ast.h"
#include "../src/frontend/parser_utils.h"
#include <iostream>
#include <fstream>
#include <string>

// ASTノードをC言語コードに変換するクラス
class CCodeGenerator : public CodeGeneratorInterface {
public:
    std::string generate_code(const ASTNode* ast) override {
        std::ostringstream oss;
        generate_c_code(ast, oss);
        return oss.str();
    }
    
    void emit_to_file(const ASTNode* ast, const std::string& filename) override {
        std::ofstream ofs(filename);
        if (!ofs) {
            throw std::runtime_error("Failed to open output file: " + filename);
        }
        generate_c_code(ast, ofs);
    }

private:
    void generate_c_code(const ASTNode* ast, std::ostream& out);
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: cgen_main <input.cb> <output.c>\n";
        return 1;
    }
    std::string input_file = argv[1];
    std::string output_file = argv[2];
    // AST生成
    ASTNode* root = parse_to_ast(input_file);
    if (!root) {
        std::cerr << "Failed to parse input file: " << input_file << "\n";
        return 1;
    }
    std::ofstream ofs(output_file);
    if (!ofs) {
        std::cerr << "Failed to open output file: " << output_file << "\n";
        delete root;
        return 1;
    }
    generate_c_code(root, ofs);
    ofs.close();
    std::cout << "C code generated: " << output_file << std::endl;
    delete root;
    return 0;
}

// 仮実装: ASTノードをC言語コードに変換する関数
void generate_c_code(ASTNode* root, std::ostream& out) {
    out << "#include <stdio.h>\n";
    out << "int main() {\n";
    // 仮: ASTがnullの場合は何も出力しない
    if (!root) {
        out << "    return 0;\n";
        out << "}\n";
        return;
    }
    // ステートメントリストのみ対応
    if (root->type == ASTNode::AST_STMTLIST) {
        for (auto stmt : root->stmts) {
            // 変数宣言・代入
            if (stmt->type == ASTNode::AST_ASSIGN) {
                std::string ctype = "int";
                switch (stmt->type_info) {
                    case 1: ctype = "signed char"; break; // tiny
                    case 2: ctype = "short"; break;
                    case 3: ctype = "int"; break;
                    case 4: ctype = "long long"; break;
                }
                out << "    " << ctype << " " << stmt->sval << " = ";
                // 右辺（数値 or 二項演算のみ対応）
                if (stmt->rhs->type == ASTNode::AST_NUM) {
                    out << stmt->rhs->lval64;
                } else if (stmt->rhs->type == ASTNode::AST_BINOP) {
                    // 左右が変数または数値のみ対応
                    auto l = stmt->rhs->lhs;
                    auto r = stmt->rhs->rhs;
                    std::string lop = (l->type == ASTNode::AST_VAR) ? l->sval : std::to_string(l->lval64);
                    std::string rop = (r->type == ASTNode::AST_VAR) ? r->sval : std::to_string(r->lval64);
                    out << lop << " " << stmt->rhs->op << " " << rop;
                }
                out << ";\n";
            } else if (stmt->type == ASTNode::AST_PRINT) {
                // print文
                out << "    printf(\"%lld\\n\", (long long)";
                if (stmt->lhs->type == ASTNode::AST_VAR) {
                    out << stmt->lhs->sval;
                } else if (stmt->lhs->type == ASTNode::AST_NUM) {
                    out << stmt->lhs->lval64;
                }
                out << ");\n";
            }
        }
    }
    out << "    return 0;\n";
    out << "}\n";
}
