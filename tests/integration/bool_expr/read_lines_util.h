#ifndef READ_LINES_UTIL_H_
#define READ_LINES_UTIL_H_
#include <string>
#include <vector>
#include <cstdio>
inline std::vector<std::string> read_lines(const char* filename) {
    std::vector<std::string> lines;
    FILE* fp = fopen(filename, "r");
    if (!fp) return lines;
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) {
        std::string line(buf);
        // 改行・CR除去
        while (!line.empty() && (line[line.size()-1] == '\n' || line[line.size()-1] == '\r')) line.erase(line.size()-1);
        if (!line.empty()) lines.push_back(line);
    }
    fclose(fp);
    return lines;
}
#endif
