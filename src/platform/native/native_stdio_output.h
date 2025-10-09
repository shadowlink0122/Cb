#pragma once
#include "../../common/io_interface.h"

class NativeStdioOutput : public IOInterface {
  public:
    void write_char(char c) override;
    void write_string(const char *str) override;
};
