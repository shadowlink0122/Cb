#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

int main() {
    // ファイルから実際の出力を読み込む
    std::ifstream f("/tmp/actual.txt");
    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string actual = buffer.str();
    
    // 期待値
    std::string expected = "1\n4\n2\n3\n3\n2\n4\n1\n5\n0\n6\n1\n7\n0\n8\n1\n9\n2\n10\n1\n11\n0\n12\n1\n13\n0\n14\n1\n15\n2\n16\n1\n17\n0\n18\n1\n19\n0\n20\n3\n21\n2\n22\n1\n23\n0\n24\n1\n25\n0\n26\n1\n27\n2\n28\n1\n29\n0\n30\n1\n31\n0\n32\n1\n33\n2\n34\n1\n35\n0\n36\n1\n37\n0\n3\n";
    
    std::cout << "Expected length: " << expected.length() << std::endl;
    std::cout << "Actual length: " << actual.length() << std::endl;
    
    if (expected == actual) {
        std::cout << "STRINGS MATCH!" << std::endl;
        return 0;
    } else {
        std::cout << "STRINGS DO NOT MATCH" << std::endl;
        
        // 最初の不一致を見つける
        for (size_t i = 0; i < std::min(expected.length(), actual.length()); i++) {
            if (expected[i] != actual[i]) {
                std::cout << "First mismatch at position " << i << std::endl;
                std::cout << "Expected char: '" << expected[i] << "' (0x" << std::hex << (int)(unsigned char)expected[i] << ")" << std::endl;
                std::cout << "Actual char: '" << actual[i] << "' (0x" << std::hex << (int)(unsigned char)actual[i] << ")" << std::endl;
                break;
            }
        }
        
        return 1;
    }
}
