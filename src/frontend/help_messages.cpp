#include "help_messages.h"
#include <iostream>

namespace HelpMessages {

// Version information
const char *CB_VERSION = "0.14.0";

void print_version() {
    std::cout << "Cb programming language version " << CB_VERSION << std::endl;
    std::cout << "Copyright (c) 2025 Cb Project" << std::endl;
}

void print_usage(const char *program_name) {
    std::cout << "Cb Programming Language - Version " << CB_VERSION << "\n\n";
    std::cout << "Usage: " << program_name << " <command> [options] <file>\n\n";
    std::cout << "Commands:\n";
    std::cout
        << "  run, -r <file>          Run file with interpreter (default)\n";
    std::cout << "  compile, -c <file>      Compile file to native binary\n";
    std::cout << "  --help, -h              Show this help message\n";
    std::cout << "  --version, -v           Show version information\n";
    std::cout << "\nGlobal Options:\n";
    std::cout << "  -d, --debug             Enable debug mode\n";
    std::cout << "  --debug-ja              Enable Japanese debug mode\n";
    std::cout << "  --no-preprocess         Disable preprocessor\n";
    std::cout << "  -D<macro>[=val]         Define preprocessor macro\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " run program.cb\n";
    std::cout << "  " << program_name << " -r program.cb\n";
    std::cout << "  " << program_name << " compile program.cb -o myapp\n";
    std::cout << "  " << program_name << " -c program.cb -o myapp\n";
    std::cout << "\nFor command-specific help:\n";
    std::cout << "  " << program_name << " run --help\n";
    std::cout << "  " << program_name << " compile --help\n";
}

void print_run_help(const char *program_name) {
    std::cout << "Cb Run Command - Execute Cb programs with interpreter\n\n";
    std::cout << "Usage: " << program_name << " run [options] <file>\n";
    std::cout << "   or: " << program_name << " -r [options] <file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -d, --debug             Enable debug mode\n";
    std::cout << "  --debug-ja              Enable Japanese debug mode\n";
    std::cout << "  --no-preprocess         Disable preprocessor\n";
    std::cout << "  -D<macro>[=val]         Define preprocessor macro\n";
    std::cout << "  --help                  Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " run program.cb\n";
    std::cout << "  " << program_name << " run program.cb -d\n";
    std::cout << "  " << program_name << " -r program.cb -DDEBUG\n";
    std::cout << "\nDescription:\n";
    std::cout
        << "  The run command executes Cb programs using the interpreter.\n";
    std::cout << "  This provides fast startup time and is ideal for:\n";
    std::cout << "  - Development and testing\n";
    std::cout << "  - Running scripts\n";
    std::cout << "  - Quick prototyping\n";
}

void print_compile_help(const char *program_name) {
    std::cout
        << "Cb Compile Command - Compile Cb programs to native binaries\n\n";
    std::cout << "Usage: " << program_name << " compile [options] <file>\n";
    std::cout << "   or: " << program_name << " -c [options] <file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <output>             Specify output file name\n";
    std::cout
        << "  -d, --debug             Enable debug mode (keep generated C++)\n";
    std::cout << "  --debug-ja              Enable Japanese debug mode\n";
    std::cout << "  --no-preprocess         Disable preprocessor\n";
    std::cout << "  -D<macro>[=val]         Define preprocessor macro\n";
    std::cout << "  --help                  Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " compile program.cb\n";
    std::cout << "  " << program_name << " compile program.cb -o myapp\n";
    std::cout << "  " << program_name << " -c program.cb -o myapp -d\n";
    std::cout << "\nOutput:\n";
    std::cout
        << "  Without -o: Creates executable with same name as input file\n";
    std::cout << "  With -o:    Creates executable with specified name\n";
    std::cout << "  Debug mode: Keeps generated C++ code in ./tmp/ directory\n";
    std::cout << "\nDescription:\n";
    std::cout
        << "  The compile command generates optimized native binaries via:\n";
    std::cout << "  1. Parse Cb code to AST\n";
    std::cout << "  2. Generate High-level IR (HIR)\n";
    std::cout << "  3. Transpile to C++\n";
    std::cout << "  4. Compile with g++/clang\n";
    std::cout << "\n  Compiled binaries provide:\n";
    std::cout << "  - Maximum performance\n";
    std::cout << "  - Standalone deployment\n";
    std::cout << "  - No runtime dependencies\n";
}

} // namespace HelpMessages
