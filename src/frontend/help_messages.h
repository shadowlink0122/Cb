#pragma once

namespace HelpMessages {
// Version information
extern const char *CB_VERSION;

// Help message functions
void print_version();
void print_usage(const char *program_name);
void print_run_help(const char *program_name);
void print_compile_help(const char *program_name);
} // namespace HelpMessages
