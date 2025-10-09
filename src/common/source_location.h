/**
 * @file source_location.h
 * @brief Source code location tracking for error reporting
 * 
 * This file defines structures and utilities for tracking source code locations,
 * used for generating detailed error messages with file name, line, and column information.
 * 
 * Part of v0.9.2 Phase 1: Error message improvements
 */

#ifndef SOURCE_LOCATION_H
#define SOURCE_LOCATION_H

#include <string>
#include <vector>

/**
 * @brief Represents a location in source code
 * 
 * Used for error reporting to provide detailed location information
 * including filename, line number, and column number.
 */
struct SourceLocation {
    std::string filename;  ///< Source file name
    int line;              ///< Line number (1-indexed)
    int column;            ///< Column number (1-indexed)

    /**
     * @brief Default constructor
     */
    SourceLocation() : filename(""), line(0), column(0) {}

    /**
     * @brief Construct with all location information
     * @param file Source file name
     * @param ln Line number
     * @param col Column number
     */
    SourceLocation(const std::string& file, int ln, int col)
        : filename(file), line(ln), column(col) {}

    /**
     * @brief Check if location is valid
     * @return true if line and column are set (> 0)
     */
    bool isValid() const {
        return line > 0 && column > 0;
    }

    /**
     * @brief Format location as string "file:line:col"
     * @return Formatted location string
     */
    std::string toString() const {
        if (filename.empty()) {
            return std::to_string(line) + ":" + std::to_string(column);
        }
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

/**
 * @brief Represents a span of source code
 * 
 * Used for highlighting ranges of code in error messages.
 */
struct SourceSpan {
    SourceLocation start;  ///< Start location
    SourceLocation end;    ///< End location

    /**
     * @brief Default constructor
     */
    SourceSpan() = default;

    /**
     * @brief Construct span from start and end locations
     * @param s Start location
     * @param e End location
     */
    SourceSpan(const SourceLocation& s, const SourceLocation& e)
        : start(s), end(e) {}

    /**
     * @brief Construct single-point span
     * @param loc Single location
     */
    explicit SourceSpan(const SourceLocation& loc)
        : start(loc), end(loc) {}

    /**
     * @brief Check if span is valid
     * @return true if both start and end are valid
     */
    bool isValid() const {
        return start.isValid() && end.isValid();
    }

    /**
     * @brief Check if span is on a single line
     * @return true if start and end are on same line
     */
    bool isSingleLine() const {
        return start.line == end.line;
    }
};

/**
 * @brief Utility functions for source location handling
 */
namespace SourceLocationUtils {

/**
 * @brief Extract a line from source code
 * @param source Full source code
 * @param line_number Line number to extract (1-indexed)
 * @return The extracted line, or empty string if line doesn't exist
 */
inline std::string extractLine(const std::string& source, int line_number) {
    if (line_number <= 0) return "";
    
    int current_line = 1;
    size_t line_start = 0;
    
    for (size_t i = 0; i < source.length(); ++i) {
        if (source[i] == '\n') {
            if (current_line == line_number) {
                return source.substr(line_start, i - line_start);
            }
            current_line++;
            line_start = i + 1;
        }
    }
    
    // Last line (no trailing newline)
    if (current_line == line_number) {
        return source.substr(line_start);
    }
    
    return "";
}

/**
 * @brief Get lines around a specific line (for context)
 * @param source Full source code
 * @param line_number Target line number (1-indexed)
 * @param context_lines Number of context lines before and after
 * @return Vector of lines with their line numbers
 */
inline std::vector<std::pair<int, std::string>> 
extractLinesWithContext(const std::string& source, int line_number, int context_lines = 2) {
    std::vector<std::pair<int, std::string>> result;
    
    int start_line = std::max(1, line_number - context_lines);
    int end_line = line_number + context_lines;
    
    int current_line = 1;
    size_t line_start = 0;
    
    for (size_t i = 0; i <= source.length(); ++i) {
        if (i == source.length() || source[i] == '\n') {
            if (current_line >= start_line && current_line <= end_line) {
                std::string line = source.substr(line_start, i - line_start);
                result.push_back({current_line, line});
            }
            
            if (current_line > end_line) break;
            
            current_line++;
            line_start = i + 1;
        }
    }
    
    return result;
}

/**
 * @brief Create a caret string (^) pointing to a specific column
 * @param column Column number (1-indexed)
 * @param length Length of the underline (default: 1)
 * @return String with spaces and carets
 */
inline std::string createCaret(int column, int length = 1) {
    if (column <= 0) return "";
    
    std::string result;
    // Add spaces before caret
    for (int i = 1; i < column; ++i) {
        result += ' ';
    }
    // Add carets
    for (int i = 0; i < length; ++i) {
        result += '^';
    }
    
    return result;
}

/**
 * @brief Calculate Levenshtein distance between two strings
 * @param s1 First string
 * @param s2 Second string
 * @return Edit distance between strings
 */
inline int levenshteinDistance(const std::string& s1, const std::string& s2) {
    const size_t len1 = s1.size();
    const size_t len2 = s2.size();
    std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

    for (size_t i = 0; i <= len1; ++i) d[i][0] = static_cast<int>(i);
    for (size_t j = 0; j <= len2; ++j) d[0][j] = static_cast<int>(j);

    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = std::min({
                d[i - 1][j] + 1,      // deletion
                d[i][j - 1] + 1,      // insertion
                d[i - 1][j - 1] + cost // substitution
            });
        }
    }

    return d[len1][len2];
}

/**
 * @brief Find similar strings (for "Did you mean?" suggestions)
 * @param target Target string
 * @param candidates List of candidate strings
 * @param max_distance Maximum edit distance to consider
 * @return List of similar strings sorted by distance
 */
inline std::vector<std::string> findSimilarStrings(
    const std::string& target,
    const std::vector<std::string>& candidates,
    int max_distance = 3) {
    
    std::vector<std::pair<int, std::string>> distances;
    
    for (const auto& candidate : candidates) {
        int distance = levenshteinDistance(target, candidate);
        if (distance <= max_distance) {
            distances.push_back({distance, candidate});
        }
    }
    
    // Sort by distance
    std::sort(distances.begin(), distances.end());
    
    // Extract strings
    std::vector<std::string> result;
    for (const auto& pair : distances) {
        result.push_back(pair.second);
    }
    
    return result;
}

} // namespace SourceLocationUtils

#endif // SOURCE_LOCATION_H
