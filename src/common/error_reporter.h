/**
 * @file error_reporter.h
 * @brief Enhanced error reporting with source location and suggestions
 * 
 * Provides utilities for generating detailed error messages with:
 * - Source code context (line numbers, code snippets)
 * - Visual indicators (caret ^ pointing to error location)
 * - "Did you mean?" suggestions
 * - Color-coded output (when supported)
 * 
 * Part of v0.9.2 Phase 1: Error message improvements
 */

#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include "source_location_utils.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

/**
 * @brief Error severity levels
 */
enum class ErrorSeverity {
    NOTE,       ///< Informational note
    WARNING,    ///< Warning (non-fatal)
    ERROR,      ///< Error (compilation fails)
    FATAL       ///< Fatal error (immediate termination)
};

/**
 * @brief Enhanced error reporter with source context
 * 
 * Generates detailed error messages with source code snippets,
 * location information, and helpful suggestions.
 */
class ErrorReporter {
public:
    /**
     * @brief Constructor
     * @param src Source code content (for extracting snippets)
     * @param filename Source file name
     * @param use_colors Enable color output (default: false)
     */
    ErrorReporter(const std::string& src, const std::string& filename, bool use_colors = false)
        : source_(src), filename_(filename), use_colors_(use_colors) {}

    /**
     * @brief Report an error with source location
     * @param severity Error severity level
     * @param location Source location where error occurred
     * @param message Error message
     * @param suggestions Optional list of suggestions ("Did you mean?")
     */
    void report(ErrorSeverity severity,
                const SourceLocation& location,
                const std::string& message,
                const std::vector<std::string>& suggestions = {}) const {
        
        std::ostringstream oss;
        
        // Format: filename:line:column: severity: message
        oss << location.toString() << ": ";
        oss << severityToString(severity) << ": ";
        oss << message << "\n";
        
        // Show source code context
        if (location.isValid() && !source_.empty()) {
            auto context = SourceLocationUtils::extractLinesWithContext(source_, location.line, 1);
            
            for (const auto& [line_num, line_content] : context) {
                oss << formatLineNumber(line_num) << " | " << line_content << "\n";
                
                // Add caret pointing to error column
                if (line_num == location.line) {
                    oss << std::string(formatLineNumber(line_num).length(), ' ') << " | ";
                    oss << SourceLocationUtils::createCaret(location.column) << "\n";
                }
            }
        }
        
        // Add suggestions if available
        if (!suggestions.empty()) {
            oss << "\n";
            if (suggestions.size() == 1) {
                oss << "Did you mean '" << suggestions[0] << "'?\n";
            } else {
                oss << "Did you mean one of these?\n";
                for (const auto& suggestion : suggestions) {
                    oss << "  - " << suggestion << "\n";
                }
            }
        }
        
        std::cerr << oss.str();
    }

    /**
     * @brief Report an error with source span
     * @param severity Error severity level
     * @param span Source span covering the error
     * @param message Error message
     * @param suggestions Optional list of suggestions
     */
    void reportSpan(ErrorSeverity severity,
                    const SourceSpan& span,
                    const std::string& message,
                    const std::vector<std::string>& suggestions = {}) const {
        
        std::ostringstream oss;
        
        oss << span.start.toString() << ": ";
        oss << severityToString(severity) << ": ";
        oss << message << "\n";
        
        if (span.isValid() && !source_.empty()) {
            if (span.isSingleLine()) {
                // Single line span
                std::string line = SourceLocationUtils::extractLine(source_, span.start.line);
                oss << formatLineNumber(span.start.line) << " | " << line << "\n";
                
                int start_col = span.start.column;
                int end_col = span.end.column;
                int length = std::max(1, end_col - start_col + 1);
                
                oss << std::string(formatLineNumber(span.start.line).length(), ' ') << " | ";
                oss << SourceLocationUtils::createCaret(start_col, length) << "\n";
            } else {
                // Multi-line span
                auto context = SourceLocationUtils::extractLinesWithContext(
                    source_, span.start.line, 0);
                
                for (const auto& [line_num, line_content] : context) {
                    if (line_num >= span.start.line && line_num <= span.end.line) {
                        oss << formatLineNumber(line_num) << " | " << line_content << "\n";
                    }
                }
            }
        }
        
        // Add suggestions
        if (!suggestions.empty()) {
            oss << "\n";
            if (suggestions.size() == 1) {
                oss << "Did you mean '" << suggestions[0] << "'?\n";
            } else {
                oss << "Did you mean one of these?\n";
                for (const auto& suggestion : suggestions) {
                    oss << "  - " << suggestion << "\n";
                }
            }
        }
        
        std::cerr << oss.str();
    }

    /**
     * @brief Report a simple error without source context
     * @param severity Error severity level
     * @param message Error message
     */
    void reportSimple(ErrorSeverity severity, const std::string& message) const {
        std::cerr << filename_ << ": "
                  << severityToString(severity) << ": "
                  << message << "\n";
    }

    /**
     * @brief Find suggestions for an unknown identifier
     * @param unknown_name The unknown identifier
     * @param known_names List of known identifiers
     * @param max_suggestions Maximum number of suggestions to return
     * @return List of suggested names
     */
    static std::vector<std::string> findSuggestions(
        const std::string& unknown_name,
        const std::vector<std::string>& known_names,
        size_t max_suggestions = 3) {
        
        auto similar = SourceLocationUtils::findSimilarStrings(unknown_name, known_names);
        
        if (similar.size() > max_suggestions) {
            similar.resize(max_suggestions);
        }
        
        return similar;
    }

private:
    std::string source_;         ///< Source code content
    std::string filename_;       ///< Source file name
    [[maybe_unused]] bool use_colors_;  ///< Enable color output (reserved for future use)

    /**
     * @brief Convert severity to string
     * @param severity Error severity level
     * @return String representation
     */
    std::string severityToString(ErrorSeverity severity) const {
        switch (severity) {
            case ErrorSeverity::NOTE:    return "note";
            case ErrorSeverity::WARNING: return "warning";
            case ErrorSeverity::ERROR:   return "error";
            case ErrorSeverity::FATAL:   return "fatal error";
            default:                     return "unknown";
        }
    }

    /**
     * @brief Format line number with padding
     * @param line_num Line number
     * @return Formatted string (e.g., "  42")
     */
    std::string formatLineNumber(int line_num) const {
        std::string num = std::to_string(line_num);
        // Pad to at least 4 characters for alignment
        while (num.length() < 4) {
            num = " " + num;
        }
        return num;
    }
};

#endif // ERROR_REPORTER_H
