#pragma once

#include <cstdint>
#include <string>

namespace cb {
namespace ir {

// ソースコード位置情報
struct SourceLocation {
    std::string file_path;
    uint32_t line;
    uint32_t column;

    SourceLocation() : line(0), column(0) {}
    SourceLocation(const std::string &path, uint32_t l, uint32_t c)
        : file_path(path), line(l), column(c) {}
};

} // namespace ir
} // namespace cb
