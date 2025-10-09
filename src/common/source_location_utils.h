/**
 * @file source_location_utils.h
 * @brief Source location utilities for error reporting
 *
 * Provides utilities for tracking source code locations
 * and generating user-friendly error messages with context.
 *
 * Part of v0.9.2 Phase 1: Error message improvements
 *
 * Note: SourceLocation is defined in ast.h. This file provides
 * additional utilities for error reporting.
 */

#ifndef SOURCE_LOCATION_UTILS_H
#define SOURCE_LOCATION_UTILS_H

#include "ast.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Represents a span (range) in source code
 *
 * Used for marking ranges of code that contain errors,
 * from a start location to an end location.
 */
struct SourceSpan {
    SourceLocation start; ///< Start of the span
    SourceLocation end;   ///< End of the span

    /**
     * @brief Default constructor
     */
    SourceSpan() = default;

    /**
     * @brief Construct span from two locations
     * @param s Start location
     * @param e End location
     */
    SourceSpan(const SourceLocation &s, const SourceLocation &e)
        : start(s), end(e) {}

    /**
     * @brief Check if span is valid
     * @return true if both start and end are valid
     */
    bool isValid() const { return start.isValid() && end.isValid(); }

    /**
     * @brief Check if span is on a single line
     * @return true if start and end are on the same line
     */
    bool isSingleLine() const { return start.line == end.line; }
};

/**
 * @brief Utilities for source location manipulation
 */
namespace SourceLocationUtils {

/**
 * @brief Extract a single line from source code
 * @param source Full source code
 * @param line_number Line number to extract (1-indexed)
 * @return The extracted line, or empty string if not found
 */
inline std::string extractLine(const std::string &source, int line_number) {
    std::istringstream stream(source);
    std::string line;
    int current_line = 1;

    while (std::getline(stream, line)) {
        if (current_line == line_number) {
            return line;
        }
        current_line++;
    }

    return "";
}

/**
 * @brief Extract lines with context around a target line
 * @param source Full source code
 * @param target_line Target line number
 * @param context_lines Number of context lines before/after
 * @return Vector of (line_number, line_text) pairs
 */
inline std::vector<std::pair<int, std::string>>
extractLinesWithContext(const std::string &source, int target_line,
                        int context_lines = 1) {

    std::vector<std::pair<int, std::string>> result;
    std::istringstream stream(source);
    std::string line;
    int current_line = 1;

    int start_line = std::max(1, target_line - context_lines);
    int end_line = target_line + context_lines;

    while (std::getline(stream, line)) {
        if (current_line >= start_line && current_line <= end_line) {
            result.emplace_back(current_line, line);
        }
        if (current_line > end_line) {
            break;
        }
        current_line++;
    }

    return result;
}

/**
 * @brief Create a caret (^) string pointing to an error location
 * @param column Column number (1-indexed)
 * @param length Length of the caret line (default: 1)
 * @return String with spaces followed by caret(s)
 */
inline std::string createCaret(int column, int length = 1) {
    if (column < 1)
        column = 1;
    if (length < 1)
        length = 1;

    std::string result(column - 1, ' ');
    result += std::string(length, '^');
    return result;
}

/**
 * @brief Calculate Levenshtein distance between two strings
 * @param s1 First string
 * @param s2 Second string
 * @return Edit distance between the strings
 */
inline int levenshteinDistance(const std::string &s1, const std::string &s2) {
    const size_t len1 = s1.length();
    const size_t len2 = s2.length();

    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

    for (size_t i = 0; i <= len1; i++) {
        dp[i][0] = i;
    }
    for (size_t j = 0; j <= len2; j++) {
        dp[0][j] = j;
    }

    for (size_t i = 1; i <= len1; i++) {
        for (size_t j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,       // deletion
                dp[i][j - 1] + 1,       // insertion
                dp[i - 1][j - 1] + cost // substitution
            });
        }
    }

    return dp[len1][len2];
}

/**
 * @brief Find similar strings for "Did you mean?" suggestions
 * @param target Target string to match
 * @param candidates List of candidate strings
 * @param max_distance Maximum edit distance to consider (default: 3)
 * @return Vector of similar strings, sorted by similarity
 */
inline std::vector<std::string>
findSimilarStrings(const std::string &target,
                   const std::vector<std::string> &candidates,
                   int max_distance = 3) {

    std::vector<std::pair<int, std::string>> scored;

    for (const auto &candidate : candidates) {
        int distance = levenshteinDistance(target, candidate);
        if (distance <= max_distance) {
            scored.emplace_back(distance, candidate);
        }
    }

    // Sort by distance (ascending)
    std::sort(scored.begin(), scored.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    std::vector<std::string> result;
    for (const auto &pair : scored) {
        result.push_back(pair.second);
    }

    return result;
}

} // namespace SourceLocationUtils

#endif // SOURCE_LOCATION_UTILS_H
