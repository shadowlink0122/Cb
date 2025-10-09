/**
 * @file test_error_reporter.cpp
 * @brief Unit tests for ErrorReporter and SourceLocation utilities
 * 
 * Tests the enhanced error reporting system added in v0.9.2.
 */

#include "../../src/common/source_location.h"
#include "../../src/common/error_reporter.h"
#include <iostream>
#include <cassert>
#include <sstream>

// Test source code for error reporting
const std::string TEST_SOURCE = R"(int main() {
    int x = 10;
    int y = unknown_var;
    return 0;
}
)";

void testSourceLocation() {
    std::cout << "Testing SourceLocation..." << std::endl;
    
    // Test default constructor
    SourceLocation loc1;
    assert(!loc1.isValid());
    assert(loc1.line == 0);
    assert(loc1.column == 0);
    
    // Test constructor with values
    SourceLocation loc2("test.cb", 5, 10);
    assert(loc2.isValid());
    assert(loc2.filename == "test.cb");
    assert(loc2.line == 5);
    assert(loc2.column == 10);
    assert(loc2.toString() == "test.cb:5:10");
    
    // Test toString without filename
    SourceLocation loc3("", 3, 7);
    assert(loc3.toString() == "3:7");
    
    std::cout << "  SourceLocation tests passed!" << std::endl;
}

void testSourceSpan() {
    std::cout << "Testing SourceSpan..." << std::endl;
    
    SourceLocation start("test.cb", 5, 10);
    SourceLocation end("test.cb", 5, 20);
    
    SourceSpan span(start, end);
    assert(span.isValid());
    assert(span.isSingleLine());
    
    // Multi-line span
    SourceLocation end2("test.cb", 7, 5);
    SourceSpan span2(start, end2);
    assert(span2.isValid());
    assert(!span2.isSingleLine());
    
    std::cout << "  SourceSpan tests passed!" << std::endl;
}

void testExtractLine() {
    std::cout << "Testing extractLine..." << std::endl;
    
    std::string line1 = SourceLocationUtils::extractLine(TEST_SOURCE, 1);
    assert(line1 == "int main() {");
    
    std::string line2 = SourceLocationUtils::extractLine(TEST_SOURCE, 2);
    assert(line2 == "    int x = 10;");
    
    std::string line3 = SourceLocationUtils::extractLine(TEST_SOURCE, 3);
    assert(line3 == "    int y = unknown_var;");
    
    // Invalid line
    std::string line_invalid = SourceLocationUtils::extractLine(TEST_SOURCE, 100);
    assert(line_invalid == "");
    
    std::cout << "  extractLine tests passed!" << std::endl;
}

void testExtractLinesWithContext() {
    std::cout << "Testing extractLinesWithContext..." << std::endl;
    
    auto lines = SourceLocationUtils::extractLinesWithContext(TEST_SOURCE, 3, 1);
    
    // Should get lines 2, 3, 4 (3±1)
    assert(lines.size() == 3);
    assert(lines[0].first == 2);
    assert(lines[0].second == "    int x = 10;");
    assert(lines[1].first == 3);
    assert(lines[1].second == "    int y = unknown_var;");
    assert(lines[2].first == 4);
    assert(lines[2].second == "    return 0;");
    
    std::cout << "  extractLinesWithContext tests passed!" << std::endl;
}

void testCreateCaret() {
    std::cout << "Testing createCaret..." << std::endl;
    
    std::string caret1 = SourceLocationUtils::createCaret(1);
    assert(caret1 == "^");
    
    std::string caret5 = SourceLocationUtils::createCaret(5);
    assert(caret5 == "    ^");
    
    std::string caret_multi = SourceLocationUtils::createCaret(3, 5);
    assert(caret_multi == "  ^^^^^");
    
    std::cout << "  createCaret tests passed!" << std::endl;
}

void testLevenshteinDistance() {
    std::cout << "Testing levenshteinDistance..." << std::endl;
    
    // Identical strings
    assert(SourceLocationUtils::levenshteinDistance("test", "test") == 0);
    
    // One character difference
    assert(SourceLocationUtils::levenshteinDistance("test", "text") == 1);
    
    // Insertion
    assert(SourceLocationUtils::levenshteinDistance("test", "tests") == 1);
    
    // Deletion
    assert(SourceLocationUtils::levenshteinDistance("tests", "test") == 1);
    
    // Multiple operations
    assert(SourceLocationUtils::levenshteinDistance("kitten", "sitting") == 3);
    
    std::cout << "  levenshteinDistance tests passed!" << std::endl;
}

void testFindSimilarStrings() {
    std::cout << "Testing findSimilarStrings..." << std::endl;
    
    std::vector<std::string> candidates = {
        "unknown_var", "unknown_func", "known_var", "var", "variable"
    };
    
    // Find similar to "unknwn_var" (typo)
    auto similar = SourceLocationUtils::findSimilarStrings("unknwn_var", candidates, 2);
    assert(!similar.empty());
    assert(similar[0] == "unknown_var"); // Closest match
    
    // Find similar to "var"
    auto similar2 = SourceLocationUtils::findSimilarStrings("vars", candidates, 1);
    assert(!similar2.empty());
    assert(similar2[0] == "var");
    
    std::cout << "  findSimilarStrings tests passed!" << std::endl;
}

void testErrorReporter() {
    std::cout << "Testing ErrorReporter..." << std::endl;
    
    ErrorReporter reporter(TEST_SOURCE, "test.cb");
    
    // Test simple error
    std::cout << "\n--- Test 1: Simple error ---" << std::endl;
    reporter.reportSimple(ErrorSeverity::ERROR, "Test error message");
    
    // Test error with location
    std::cout << "\n--- Test 2: Error with location ---" << std::endl;
    SourceLocation loc("test.cb", 3, 13);
    reporter.report(ErrorSeverity::ERROR, loc, "Undefined variable");
    
    // Test error with suggestions
    std::cout << "\n--- Test 3: Error with suggestions ---" << std::endl;
    std::vector<std::string> suggestions = {"known_var", "x", "y"};
    reporter.report(ErrorSeverity::ERROR, loc, "Undefined variable 'unknown_var'", suggestions);
    
    // Test span error
    std::cout << "\n--- Test 4: Span error ---" << std::endl;
    SourceLocation start("test.cb", 3, 13);
    SourceLocation end("test.cb", 3, 24);
    SourceSpan span(start, end);
    reporter.reportSpan(ErrorSeverity::ERROR, span, "Invalid expression");
    
    std::cout << "  ErrorReporter tests passed!" << std::endl;
}

void testFindSuggestions() {
    std::cout << "Testing ErrorReporter::findSuggestions..." << std::endl;
    
    std::vector<std::string> known_names = {
        "variable", "value", "vector", "void", "volatile"
    };
    
    // Find suggestions for typo
    auto suggestions = ErrorReporter::findSuggestions("variabl", known_names, 3);
    assert(!suggestions.empty());
    assert(suggestions[0] == "variable");
    
    // Find suggestions for different typo
    auto suggestions2 = ErrorReporter::findSuggestions("valu", known_names, 2);
    assert(!suggestions2.empty());
    assert(suggestions2[0] == "value");
    
    std::cout << "  findSuggestions tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Error Reporter Unit Tests ===" << std::endl;
    std::cout << std::endl;
    
    try {
        testSourceLocation();
        testSourceSpan();
        testExtractLine();
        testExtractLinesWithContext();
        testCreateCaret();
        testLevenshteinDistance();
        testFindSimilarStrings();
        testErrorReporter();
        testFindSuggestions();
        
        std::cout << "\n=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
