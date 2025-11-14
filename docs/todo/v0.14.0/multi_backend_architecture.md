# 複数バックエンド対応アーキテクチャ

**バージョン**: v0.16.0
**作成日**: 2025-11-13
**ステータス**: 設計中

---

## 目次

1. [アーキテクチャ概要](#1-アーキテクチャ概要)
2. [バックエンドインターフェース](#2-バックエンドインターフェース)
3. [バックエンド選択とディスパッチ](#3-バックエンド選択とディスパッチ)
4. [ターゲット情報管理](#4-ターゲット情報管理)
5. [コンパイラパイプライン](#5-コンパイラパイプライン)
6. [コマンドラインインターフェース](#6-コマンドラインインターフェース)
7. [ビルドシステムの統合](#7-ビルドシステムの統合)

---

## 1. アーキテクチャ概要

### 1.1 全体構造

```
┌──────────────────────────────────────────────────┐
│  Frontend (Parser + Preprocessor)                │
│  - Lexer                                         │
│  - Parser                                        │
│  - AST Construction                              │
└──────────────────┬───────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────┐
│  IR Pipeline                                     │
│  ┌────────────────────────────────────────────┐ │
│  │  AST → HIR → MIR → LIR                     │ │
│  │  - Type Resolution                          │ │
│  │  - Generics Monomorphization                │ │
│  │  - SSA Transformation                       │ │
│  │  - Dataflow Analysis                        │ │
│  └────────────────────────────────────────────┘ │
└──────────────────┬───────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────┐
│  Backend Dispatcher                              │
│  - Backend Selection                             │
│  - Target Configuration                          │
└──────────────────┬───────────────────────────────┘
                   │
         ┌─────────┼─────────┬───────────┐
         ▼         ▼         ▼           ▼
    ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
    │Interp. │ │Native  │ │ WASM   │ │TypeSc. │
    │Backend │ │Backend │ │Backend │ │Backend │
    └────────┘ └────────┘ └────────┘ └────────┘
         │         │         │           │
         ▼         ▼         ▼           ▼
    ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
    │ Execute│ │Binary  │ │ .wasm  │ │  .ts   │
    └────────┘ └────────┘ └────────┘ └────────┘
```

### 1.2 サポートするバックエンド

| バックエンド | 出力形式 | 主な用途 |
|-------------|---------|----------|
| Interpreter | - | 即座実行、デバッグ |
| Native | ELF/Mach-O/PE | 高性能アプリケーション |
| WASM | .wasm | Webアプリケーション |
| TypeScript | .ts | フロントエンド開発 |

---

## 2. バックエンドインターフェース

### 2.1 統一バックエンドインターフェース

#### ファイル: `src/backend/codegen/common/codegen_interface.h`

```cpp
#pragma once
#include "backend/ir/lir/lir_node.h"
#include "target_info.h"
#include <string>
#include <memory>

namespace cb {
namespace codegen {

// コード生成インターフェース
class CodegenInterface {
public:
    virtual ~CodegenInterface() = default;

    // ターゲット情報の設定
    virtual void set_target(const TargetInfo& target) = 0;

    // LIRからコード生成
    virtual void generate(const ir::lir::LIRProgram& program) = 0;

    // 出力ファイルの書き込み
    virtual void write_output(const std::string& output_file) = 0;

    // バックエンド名
    virtual std::string name() const = 0;

    // サポートするターゲット
    virtual std::vector<std::string> supported_targets() const = 0;

    // 最適化レベルの設定
    virtual void set_optimization_level(int level) {
        optimization_level = level;
    }

protected:
    int optimization_level = 0;
};

} // namespace codegen
} // namespace cb
```

### 2.2 各バックエンドの実装

#### インタプリタバックエンド

```cpp
// src/backend/codegen/interpreter/interpreter_backend.h
#pragma once
#include "codegen/common/codegen_interface.h"

namespace cb {
namespace codegen {

class InterpreterBackend : public CodegenInterface {
public:
    void set_target(const TargetInfo& target) override;
    void generate(const ir::lir::LIRProgram& program) override;
    void write_output(const std::string& output_file) override;
    std::string name() const override { return "interpreter"; }
    std::vector<std::string> supported_targets() const override;

    // インタプリタ固有の機能
    void execute();

private:
    ir::lir::LIRProgram program;
};

} // namespace codegen
} // namespace cb
```

#### ネイティブバックエンド

```cpp
// src/backend/codegen/native/native_backend.h
#pragma once
#include "codegen/common/codegen_interface.h"

namespace cb {
namespace codegen {

class NativeBackend : public CodegenInterface {
public:
    void set_target(const TargetInfo& target) override;
    void generate(const ir::lir::LIRProgram& program) override;
    void write_output(const std::string& output_file) override;
    std::string name() const override { return "native"; }
    std::vector<std::string> supported_targets() const override;

private:
    TargetInfo target;
    std::string assembly_code;
};

} // namespace codegen
} // namespace cb
```

#### WASMバックエンド

```cpp
// src/backend/codegen/wasm/wasm_backend.h
#pragma once
#include "codegen/common/codegen_interface.h"
#include "wasm_module_builder.h"

namespace cb {
namespace codegen {

class WASMBackend : public CodegenInterface {
public:
    void set_target(const TargetInfo& target) override;
    void generate(const ir::lir::LIRProgram& program) override;
    void write_output(const std::string& output_file) override;
    std::string name() const override { return "wasm"; }
    std::vector<std::string> supported_targets() const override;

private:
    wasm::WASMModuleBuilder module_builder;
};

} // namespace codegen
} // namespace cb
```

#### TypeScriptバックエンド

```cpp
// src/backend/codegen/typescript/typescript_backend.h
#pragma once
#include "codegen/common/codegen_interface.h"
#include "ts_emitter.h"

namespace cb {
namespace codegen {

class TypeScriptBackend : public CodegenInterface {
public:
    void set_target(const TargetInfo& target) override;
    void generate(const ir::lir::LIRProgram& program) override;
    void write_output(const std::string& output_file) override;
    std::string name() const override { return "typescript"; }
    std::vector<std::string> supported_targets() const override;

private:
    typescript::TSEmitter emitter;
};

} // namespace codegen
} // namespace cb
```

---

## 3. バックエンド選択とディスパッチ

### 3.1 バックエンドファクトリ

#### ファイル: `src/backend/codegen/common/backend_factory.h`

```cpp
#pragma once
#include "codegen_interface.h"
#include "target_info.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace cb {
namespace codegen {

// バックエンドファクトリ
class BackendFactory {
public:
    // シングルトンインスタンス
    static BackendFactory& instance();

    // バックエンドの登録
    void register_backend(const std::string& name, std::function<std::unique_ptr<CodegenInterface>()> creator);

    // バックエンドの作成
    std::unique_ptr<CodegenInterface> create_backend(const std::string& name);

    // 登録されているバックエンドの一覧
    std::vector<std::string> available_backends() const;

    // バックエンドのサポート確認
    bool is_backend_available(const std::string& name) const;

private:
    BackendFactory();
    std::unordered_map<std::string, std::function<std::unique_ptr<CodegenInterface>()>> backends;
};

// バックエンド登録マクロ
#define REGISTER_BACKEND(name, class_name) \
    namespace { \
        struct class_name##Registrar { \
            class_name##Registrar() { \
                BackendFactory::instance().register_backend(name, []() { \
                    return std::make_unique<class_name>(); \
                }); \
            } \
        }; \
        static class_name##Registrar class_name##_registrar; \
    }

} // namespace codegen
} // namespace cb
```

#### ファイル: `src/backend/codegen/common/backend_factory.cpp`

```cpp
#include "backend_factory.h"

namespace cb {
namespace codegen {

BackendFactory& BackendFactory::instance() {
    static BackendFactory instance;
    return instance;
}

BackendFactory::BackendFactory() {}

void BackendFactory::register_backend(const std::string& name, std::function<std::unique_ptr<CodegenInterface>()> creator) {
    backends[name] = creator;
}

std::unique_ptr<CodegenInterface> BackendFactory::create_backend(const std::string& name) {
    auto it = backends.find(name);
    if (it == backends.end()) {
        throw std::runtime_error("Backend not found: " + name);
    }
    return it->second();
}

std::vector<std::string> BackendFactory::available_backends() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : backends) {
        result.push_back(name);
    }
    return result;
}

bool BackendFactory::is_backend_available(const std::string& name) const {
    return backends.find(name) != backends.end();
}

} // namespace codegen
} // namespace cb
```

### 3.2 各バックエンドの登録

```cpp
// src/backend/codegen/interpreter/interpreter_backend.cpp
#include "interpreter_backend.h"
#include "codegen/common/backend_factory.h"

namespace cb {
namespace codegen {

// 自動登録
REGISTER_BACKEND("interpreter", InterpreterBackend);

// 実装...

} // namespace codegen
} // namespace cb
```

```cpp
// src/backend/codegen/native/native_backend.cpp
#include "native_backend.h"
#include "codegen/common/backend_factory.h"

namespace cb {
namespace codegen {

REGISTER_BACKEND("native", NativeBackend);

// 実装...

} // namespace codegen
} // namespace cb
```

```cpp
// src/backend/codegen/wasm/wasm_backend.cpp
#include "wasm_backend.h"
#include "codegen/common/backend_factory.h"

namespace cb {
namespace codegen {

REGISTER_BACKEND("wasm", WASMBackend);

// 実装...

} // namespace codegen
} // namespace cb
```

```cpp
// src/backend/codegen/typescript/typescript_backend.cpp
#include "typescript_backend.h"
#include "codegen/common/backend_factory.h"

namespace cb {
namespace codegen {

REGISTER_BACKEND("typescript", TypeScriptBackend);

// 実装...

} // namespace codegen
} // namespace cb
```

---

## 4. ターゲット情報管理

### 4.1 ターゲット情報クラス

#### ファイル: `src/backend/codegen/common/target_info.h`

```cpp
#pragma once
#include <string>
#include <cstdint>

namespace cb {
namespace codegen {

// ターゲットアーキテクチャ
enum class TargetArch {
    X86_64,           // x86-64 (AMD64)
    ARM64,            // ARM64 (AArch64)
    WASM32,           // WebAssembly 32-bit
    WASM64,           // WebAssembly 64-bit
    Unknown,
};

// ターゲットOS
enum class TargetOS {
    Linux,
    MacOS,
    Windows,
    Web,              // ブラウザ
    Unknown,
};

// エンディアン
enum class Endian {
    Little,
    Big,
};

// 呼び出し規約
enum class CallingConvention {
    SystemV,          // System V AMD64 ABI (Linux, macOS)
    MicrosoftX64,     // Microsoft x64 calling convention (Windows)
    AAPCS64,          // ARM Architecture Procedure Call Standard 64-bit
    WASM,             // WebAssembly calling convention
};

// ターゲット情報
struct TargetInfo {
    TargetArch arch;
    TargetOS os;
    Endian endian;
    CallingConvention calling_convention;
    size_t pointer_size;       // ポインタサイズ（バイト）
    size_t pointer_alignment;  // ポインタアライメント（バイト）
    std::string triple;        // ターゲットトリプル（例: "x86_64-unknown-linux-gnu"）

    // デフォルトコンストラクタ
    TargetInfo();

    // ターゲットトリプルからの構築
    static TargetInfo from_triple(const std::string& triple);

    // 現在のホストのターゲット情報
    static TargetInfo host();

    // 文字列表現
    std::string to_string() const;

    // アーキテクチャ名
    std::string arch_name() const;

    // OS名
    std::string os_name() const;
};

} // namespace codegen
} // namespace cb
```

#### ファイル: `src/backend/codegen/common/target_info.cpp`

```cpp
#include "target_info.h"
#include <stdexcept>

namespace cb {
namespace codegen {

TargetInfo::TargetInfo()
    : arch(TargetArch::Unknown),
      os(TargetOS::Unknown),
      endian(Endian::Little),
      calling_convention(CallingConvention::SystemV),
      pointer_size(8),
      pointer_alignment(8) {}

TargetInfo TargetInfo::from_triple(const std::string& triple) {
    TargetInfo target;

    // ターゲットトリプルのパース
    // 例: "x86_64-unknown-linux-gnu"
    //     "arm64-apple-darwin"
    //     "wasm32-unknown-unknown"

    if (triple.find("x86_64") != std::string::npos || triple.find("amd64") != std::string::npos) {
        target.arch = TargetArch::X86_64;
        target.pointer_size = 8;
        target.pointer_alignment = 8;
    } else if (triple.find("arm64") != std::string::npos || triple.find("aarch64") != std::string::npos) {
        target.arch = TargetArch::ARM64;
        target.pointer_size = 8;
        target.pointer_alignment = 8;
    } else if (triple.find("wasm32") != std::string::npos) {
        target.arch = TargetArch::WASM32;
        target.pointer_size = 4;
        target.pointer_alignment = 4;
    } else if (triple.find("wasm64") != std::string::npos) {
        target.arch = TargetArch::WASM64;
        target.pointer_size = 8;
        target.pointer_alignment = 8;
    }

    if (triple.find("linux") != std::string::npos) {
        target.os = TargetOS::Linux;
        target.calling_convention = CallingConvention::SystemV;
    } else if (triple.find("darwin") != std::string::npos || triple.find("apple") != std::string::npos) {
        target.os = TargetOS::MacOS;
        target.calling_convention = CallingConvention::SystemV;
    } else if (triple.find("windows") != std::string::npos) {
        target.os = TargetOS::Windows;
        target.calling_convention = CallingConvention::MicrosoftX64;
    } else if (triple.find("wasm") != std::string::npos) {
        target.os = TargetOS::Web;
        target.calling_convention = CallingConvention::WASM;
    }

    target.triple = triple;
    return target;
}

TargetInfo TargetInfo::host() {
#if defined(__x86_64__) || defined(_M_X64)
    #if defined(__linux__)
        return from_triple("x86_64-unknown-linux-gnu");
    #elif defined(__APPLE__)
        return from_triple("x86_64-apple-darwin");
    #elif defined(_WIN32)
        return from_triple("x86_64-pc-windows-msvc");
    #endif
#elif defined(__aarch64__) || defined(_M_ARM64)
    #if defined(__linux__)
        return from_triple("arm64-unknown-linux-gnu");
    #elif defined(__APPLE__)
        return from_triple("arm64-apple-darwin");
    #endif
#endif
    return TargetInfo();
}

std::string TargetInfo::to_string() const {
    return triple;
}

std::string TargetInfo::arch_name() const {
    switch (arch) {
    case TargetArch::X86_64: return "x86_64";
    case TargetArch::ARM64: return "arm64";
    case TargetArch::WASM32: return "wasm32";
    case TargetArch::WASM64: return "wasm64";
    default: return "unknown";
    }
}

std::string TargetInfo::os_name() const {
    switch (os) {
    case TargetOS::Linux: return "linux";
    case TargetOS::MacOS: return "macos";
    case TargetOS::Windows: return "windows";
    case TargetOS::Web: return "web";
    default: return "unknown";
    }
}

} // namespace codegen
} // namespace cb
```

---

## 5. コンパイラパイプライン

### 5.1 コンパイラドライバー

#### ファイル: `src/frontend/compiler_driver.h`

```cpp
#pragma once
#include "common/ast.h"
#include "backend/ir/hir/hir_node.h"
#include "backend/ir/mir/mir_node.h"
#include "backend/ir/lir/lir_node.h"
#include "backend/codegen/common/codegen_interface.h"
#include "backend/codegen/common/target_info.h"
#include <memory>
#include <string>

namespace cb {

// コンパイラオプション
struct CompilerOptions {
    std::string backend = "interpreter";     // バックエンド選択
    std::string target_triple = "";          // ターゲット指定
    std::string output_file = "";            // 出力ファイル
    int optimization_level = 0;              // 最適化レベル (0-3)

    // デバッグオプション
    bool dump_ast = false;
    bool dump_hir = false;
    bool dump_mir = false;
    bool dump_lir = false;
    bool dump_cfg = false;
    bool emit_cfg_dot = false;

    // 停止ポイント
    enum class StopAt {
        None,
        AST,
        HIR,
        MIR,
        LIR,
        Assembly,
    };
    StopAt stop_at = StopAt::None;
};

// コンパイラドライバー
class CompilerDriver {
public:
    CompilerDriver(const CompilerOptions& options);

    // コンパイル実行
    int compile(const std::string& input_file);

private:
    // パイプラインのステージ
    std::unique_ptr<ASTNode> parse(const std::string& input_file);
    ir::hir::HIRProgram generate_hir(ASTNode* ast);
    ir::mir::MIRProgram generate_mir(const ir::hir::HIRProgram& hir);
    ir::lir::LIRProgram generate_lir(const ir::mir::MIRProgram& mir);
    void codegen(const ir::lir::LIRProgram& lir);

    // デバッグ出力
    void dump_ast(const ASTNode* ast);
    void dump_hir(const ir::hir::HIRProgram& hir);
    void dump_mir(const ir::mir::MIRProgram& mir);
    void dump_lir(const ir::lir::LIRProgram& lir);

    CompilerOptions options;
    codegen::TargetInfo target;
    std::unique_ptr<codegen::CodegenInterface> backend;
};

} // namespace cb
```

#### ファイル: `src/frontend/compiler_driver.cpp`

```cpp
#include "compiler_driver.h"
#include "backend/codegen/common/backend_factory.h"
#include "backend/ir/hir/hir_generator.h"
#include "backend/ir/hir/hir_dumper.h"
#include "backend/ir/mir/mir_generator.h"
#include "backend/ir/mir/mir_dumper.h"
#include "backend/ir/lir/lir_generator.h"
#include "backend/ir/lir/lir_dumper.h"
#include <iostream>
#include <fstream>

namespace cb {

CompilerDriver::CompilerDriver(const CompilerOptions& options)
    : options(options) {
    // ターゲット情報の設定
    if (options.target_triple.empty()) {
        target = codegen::TargetInfo::host();
    } else {
        target = codegen::TargetInfo::from_triple(options.target_triple);
    }

    // バックエンドの作成
    backend = codegen::BackendFactory::instance().create_backend(options.backend);
    backend->set_target(target);
    backend->set_optimization_level(options.optimization_level);
}

int CompilerDriver::compile(const std::string& input_file) {
    try {
        // 1. パース
        std::cout << "Parsing..." << std::endl;
        auto ast = parse(input_file);
        if (!ast) {
            std::cerr << "Parse failed" << std::endl;
            return 1;
        }

        if (options.dump_ast) {
            dump_ast(ast.get());
        }

        if (options.stop_at == CompilerOptions::StopAt::AST) {
            return 0;
        }

        // 2. HIR生成
        std::cout << "Generating HIR..." << std::endl;
        auto hir = generate_hir(ast.get());

        if (options.dump_hir) {
            dump_hir(hir);
        }

        if (options.stop_at == CompilerOptions::StopAt::HIR) {
            return 0;
        }

        // 3. MIR生成
        std::cout << "Generating MIR..." << std::endl;
        auto mir = generate_mir(hir);

        if (options.dump_mir) {
            dump_mir(mir);
        }

        if (options.dump_cfg || options.emit_cfg_dot) {
            // CFG可視化
        }

        if (options.stop_at == CompilerOptions::StopAt::MIR) {
            return 0;
        }

        // 4. LIR生成
        std::cout << "Generating LIR..." << std::endl;
        auto lir = generate_lir(mir);

        if (options.dump_lir) {
            dump_lir(lir);
        }

        if (options.stop_at == CompilerOptions::StopAt::LIR) {
            return 0;
        }

        // 5. コード生成
        std::cout << "Generating code..." << std::endl;
        codegen(lir);

        std::cout << "Compilation successful!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
        return 1;
    }
}

std::unique_ptr<ASTNode> CompilerDriver::parse(const std::string& input_file) {
    // パーサーを使用してASTを構築
    // 実装は既存のパーサーを使用
    return nullptr;  // TODO: 実装
}

ir::hir::HIRProgram CompilerDriver::generate_hir(ASTNode* ast) {
    ir::hir::HIRGenerator generator;
    return generator.generate(ast);
}

ir::mir::MIRProgram CompilerDriver::generate_mir(const ir::hir::HIRProgram& hir) {
    ir::mir::MIRGenerator generator;
    return generator.generate(hir);
}

ir::lir::LIRProgram CompilerDriver::generate_lir(const ir::mir::MIRProgram& mir) {
    ir::lir::LIRGenerator generator;
    return generator.generate(mir);
}

void CompilerDriver::codegen(const ir::lir::LIRProgram& lir) {
    backend->generate(lir);

    if (!options.output_file.empty()) {
        backend->write_output(options.output_file);
    }
}

void CompilerDriver::dump_hir(const ir::hir::HIRProgram& hir) {
    ir::hir::HIRDumper dumper;
    std::cout << "=== HIR ===" << std::endl;
    std::cout << dumper.dump_program(const_cast<ir::hir::HIRProgram*>(&hir)) << std::endl;
}

void CompilerDriver::dump_mir(const ir::mir::MIRProgram& mir) {
    ir::mir::MIRDumper dumper;
    std::cout << "=== MIR ===" << std::endl;
    std::cout << dumper.dump_program(const_cast<ir::mir::MIRProgram*>(&mir)) << std::endl;
}

void CompilerDriver::dump_lir(const ir::lir::LIRProgram& lir) {
    ir::lir::LIRDumper dumper;
    std::cout << "=== LIR ===" << std::endl;
    std::cout << dumper.dump_program(const_cast<ir::lir::LIRProgram*>(&lir)) << std::endl;
}

} // namespace cb
```

---

## 6. コマンドラインインターフェース

### 6.1 main.cppの更新

#### ファイル: `src/frontend/main.cpp`

```cpp
#include "compiler_driver.h"
#include "backend/codegen/common/backend_factory.h"
#include <iostream>
#include <string>
#include <vector>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] <input_file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  --backend=<name>         Select backend (interpreter, native, wasm, typescript)\n";
    std::cout << "  --target=<triple>        Target triple (e.g., x86_64-unknown-linux-gnu)\n";
    std::cout << "  --output=<file>          Output file\n";
    std::cout << "  -O<level>                Optimization level (0-3)\n";
    std::cout << "\n";
    std::cout << "Debug options:\n";
    std::cout << "  --dump-ast               Dump AST\n";
    std::cout << "  --dump-hir               Dump HIR\n";
    std::cout << "  --dump-mir               Dump MIR\n";
    std::cout << "  --dump-lir               Dump LIR\n";
    std::cout << "  --dump-cfg               Dump control flow graph\n";
    std::cout << "  --emit-cfg-dot           Emit CFG in GraphViz DOT format\n";
    std::cout << "  --dump-all-ir            Dump all IR levels\n";
    std::cout << "\n";
    std::cout << "Stop options:\n";
    std::cout << "  --stop-at=<stage>        Stop at stage (ast, hir, mir, lir, asm)\n";
    std::cout << "\n";
    std::cout << "Available backends:\n";
    for (const auto& backend : cb::codegen::BackendFactory::instance().available_backends()) {
        std::cout << "  - " << backend << "\n";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    cb::CompilerOptions options;
    std::string input_file;

    // コマンドライン引数のパース
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.find("--backend=") == 0) {
            options.backend = arg.substr(10);
        } else if (arg.find("--target=") == 0) {
            options.target_triple = arg.substr(9);
        } else if (arg.find("--output=") == 0) {
            options.output_file = arg.substr(9);
        } else if (arg.find("-O") == 0) {
            options.optimization_level = std::stoi(arg.substr(2));
        } else if (arg == "--dump-ast") {
            options.dump_ast = true;
        } else if (arg == "--dump-hir") {
            options.dump_hir = true;
        } else if (arg == "--dump-mir") {
            options.dump_mir = true;
        } else if (arg == "--dump-lir") {
            options.dump_lir = true;
        } else if (arg == "--dump-cfg") {
            options.dump_cfg = true;
        } else if (arg == "--emit-cfg-dot") {
            options.emit_cfg_dot = true;
        } else if (arg == "--dump-all-ir") {
            options.dump_hir = true;
            options.dump_mir = true;
            options.dump_lir = true;
        } else if (arg.find("--stop-at=") == 0) {
            std::string stage = arg.substr(10);
            if (stage == "ast") options.stop_at = cb::CompilerOptions::StopAt::AST;
            else if (stage == "hir") options.stop_at = cb::CompilerOptions::StopAt::HIR;
            else if (stage == "mir") options.stop_at = cb::CompilerOptions::StopAt::MIR;
            else if (stage == "lir") options.stop_at = cb::CompilerOptions::StopAt::LIR;
            else if (stage == "asm") options.stop_at = cb::CompilerOptions::StopAt::Assembly;
        } else if (arg[0] != '-') {
            input_file = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return 1;
        }
    }

    if (input_file.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    // コンパイラドライバーの実行
    cb::CompilerDriver driver(options);
    return driver.compile(input_file);
}
```

---

## 7. ビルドシステムの統合

### 7.1 Makefileの更新

```makefile
# バックエンドディレクトリ
CODEGEN_COMMON_DIR=$(BACKEND_DIR)/codegen/common
CODEGEN_INTERPRETER_DIR=$(BACKEND_DIR)/codegen/interpreter
CODEGEN_NATIVE_DIR=$(BACKEND_DIR)/codegen/native
CODEGEN_WASM_DIR=$(BACKEND_DIR)/codegen/wasm
CODEGEN_TYPESCRIPT_DIR=$(BACKEND_DIR)/codegen/typescript

# コード生成オブジェクトファイル
CODEGEN_COMMON_OBJS = \
    $(CODEGEN_COMMON_DIR)/target_info.o \
    $(CODEGEN_COMMON_DIR)/backend_factory.o

CODEGEN_INTERPRETER_OBJS = \
    $(CODEGEN_INTERPRETER_DIR)/interpreter_backend.o

CODEGEN_NATIVE_OBJS = \
    $(CODEGEN_NATIVE_DIR)/native_backend.o \
    $(CODEGEN_NATIVE_DIR)/x86_64_codegen.o \
    $(CODEGEN_NATIVE_DIR)/arm64_codegen.o \
    $(CODEGEN_NATIVE_DIR)/register_allocator.o

CODEGEN_WASM_OBJS = \
    $(CODEGEN_WASM_DIR)/wasm_backend.o \
    $(CODEGEN_WASM_DIR)/wasm_codegen.o \
    $(CODEGEN_WASM_DIR)/wasm_module_builder.o

CODEGEN_TYPESCRIPT_OBJS = \
    $(CODEGEN_TYPESCRIPT_DIR)/typescript_backend.o \
    $(CODEGEN_TYPESCRIPT_DIR)/typescript_codegen.o \
    $(CODEGEN_TYPESCRIPT_DIR)/ts_emitter.o

CODEGEN_OBJS = \
    $(CODEGEN_COMMON_OBJS) \
    $(CODEGEN_INTERPRETER_OBJS) \
    $(CODEGEN_NATIVE_OBJS) \
    $(CODEGEN_WASM_OBJS) \
    $(CODEGEN_TYPESCRIPT_OBJS)

# コンパイラドライバー
COMPILER_DRIVER_OBJS = \
    $(FRONTEND_DIR)/compiler_driver.o

# 全オブジェクトファイル
ALL_OBJS = $(FRONTEND_OBJS) $(IR_OBJS) $(CODEGEN_OBJS) $(BACKEND_OBJS) $(COMMON_OBJS) $(COMPILER_DRIVER_OBJS)

# メイン実行ファイル
$(MAIN_TARGET): $(ALL_OBJS)
    $(CC) $(CFLAGS) -o $(MAIN_TARGET) $(ALL_OBJS)

# ディレクトリ作成
setup-dirs:
    @mkdir -p $(CODEGEN_COMMON_DIR) $(CODEGEN_INTERPRETER_DIR)
    @mkdir -p $(CODEGEN_NATIVE_DIR) $(CODEGEN_WASM_DIR) $(CODEGEN_TYPESCRIPT_DIR)
```

---

## 8. まとめ

この複数バックエンド対応アーキテクチャにより：

1. **統一インターフェース**: 全バックエンドが共通のインターフェースを実装
2. **動的バックエンド選択**: 実行時にバックエンドを切り替え可能
3. **ターゲット情報管理**: クロスコンパイルのサポート
4. **拡張性**: 新しいバックエンドの追加が容易
5. **柔軟なコンパイラパイプライン**: IR各段階で停止・ダンプ可能

v0.16.0完了後、Cb言語は以下の全てのターゲットをサポートします：
- インタプリタ実行
- ネイティブバイナリ（Linux/macOS/Windows）
- WebAssembly（ブラウザ/Node.js）
- TypeScript（フロントエンド開発）
