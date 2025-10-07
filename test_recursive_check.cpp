#include <iostream>
#include <string>

int main() {
    std::string expected = "1\n4\n2\n3\n3\n2\n4\n1\n5\n0\n6\n1\n7\n0\n8\n1\n9\n2\n10\n1\n11\n0\n12\n1\n13\n0\n14\n1\n15\n2\n16\n1\n17\n0\n18\n1\n19\n0\n20\n3\n21\n2\n22\n1\n23\n0\n24\n1\n25\n0\n26\n1\n27\n2\n28\n1\n29\n0\n30\n1\n31\n0\n32\n1\n33\n2\n34\n1\n35\n0\n36\n1\n37\n0\n3\n";
    
    std::cout << "Expected length: " << expected.length() << std::endl;
    std::cout << "Expected last 20 chars (hex): ";
    for (size_t i = expected.length() >= 20 ? expected.length() - 20 : 0; i < expected.length(); i++) {
        printf("%02x ", (unsigned char)expected[i]);
    }
    std::cout << std::endl;
    
    return 0;
}
