#include "ffi_manager.h"
#include "../../common/type_helpers.h"
#include <cstring>
#include <dlfcn.h>
#include <iostream>

#ifdef __APPLE__
#define LIB_EXTENSION ".dylib"
#elif defined(__linux__)
#define LIB_EXTENSION ".so"
#else
#define LIB_EXTENSION ".so"
#endif

namespace cb {

FFIManager::FFIManager() {
    initializeSearchPaths();
    // last_result_を初期化
    last_result_.type = TYPE_UNKNOWN;
    last_result_.value = 0;
    last_result_.double_value = 0.0;
}

FFIManager::~FFIManager() {
    // すべてのライブラリをアンロード
    for (auto &[name, lib] : loaded_libraries_) {
        if (lib.handle) {
            dlclose(lib.handle);
        }
    }
}

void FFIManager::initializeSearchPaths() {
    search_paths_.push_back("./stdlib/foreign/");
    search_paths_.push_back(".");
    search_paths_.push_back("/usr/local/lib/");
    search_paths_.push_back("/usr/lib/");

#ifdef __APPLE__
    search_paths_.push_back("/opt/homebrew/lib/");
    search_paths_.push_back(
        "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib/");
    search_paths_.push_back("/System/Library/Frameworks/");
#endif
}

std::string FFIManager::resolveLibraryPath(const std::string &module_name) {
    // モジュール名からライブラリファイル名を生成
    // foreign.m -> libm.dylib (macOS) or libm.so (Linux)
    // foreign.math -> libmath.dylib
    // foreign.c -> libc.dylib

    std::string lib_name = "lib" + module_name + LIB_EXTENSION;

    // 検索パスを探索
    for (const auto &path : search_paths_) {
        std::string full_path = path + lib_name;

        // ファイルが存在するかチェック（dlopenで試す）
        void *test_handle = dlopen(full_path.c_str(), RTLD_LAZY | RTLD_NOLOAD);
        if (test_handle) {
            dlclose(test_handle);
            return full_path;
        }

        // まだロードされていない場合は試しにロード
        test_handle = dlopen(full_path.c_str(), RTLD_LAZY);
        if (test_handle) {
            dlclose(test_handle);
            return full_path;
        }
    }

    // システムライブラリとして試す（絶対パスなし）
    return lib_name;
}

bool FFIManager::loadLibrary(const std::string &module_name,
                             const std::string &library_path) {
    // すでにロード済みの場合
    if (loaded_libraries_.find(module_name) != loaded_libraries_.end()) {
        return true;
    }

    std::string path =
        library_path.empty() ? resolveLibraryPath(module_name) : library_path;

    void *handle = nullptr;
    if (!loadLibraryHandle(path, &handle)) {
        last_error_ =
            "Failed to load library: " + path + " - " + std::string(dlerror());
        return false;
    }

    LoadedLibrary lib;
    lib.handle = handle;
    lib.path = path;

    loaded_libraries_[module_name] = lib;

    return true;
}

bool FFIManager::loadLibraryHandle(const std::string &path, void **handle) {
    *handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!*handle) {
// macOSの場合、システムライブラリとして再試行
#ifdef __APPLE__
        std::string lib_name = path.substr(path.find_last_of("/") + 1);
        if (lib_name.find("lib") == 0) {
            // libm.dylib -> m として試す
            lib_name = lib_name.substr(3); // "lib" を削除
            size_t dot_pos = lib_name.find('.');
            if (dot_pos != std::string::npos) {
                lib_name = lib_name.substr(0, dot_pos);
            }

            *handle = dlopen((lib_name + LIB_EXTENSION).c_str(), RTLD_LAZY);
            if (*handle) {
                return true;
            }
        }
#endif
        return false;
    }
    return true;
}

void *FFIManager::getFunctionPointer(void *lib_handle,
                                     const std::string &func_name) {
    dlerror(); // エラーをクリア
    void *func_ptr = dlsym(lib_handle, func_name.c_str());

    const char *error = dlerror();
    if (error) {
        last_error_ = "Failed to find function: " + func_name + " - " +
                      std::string(error);
        return nullptr;
    }

    return func_ptr;
}

bool FFIManager::registerFunction(const std::string &module_name,
                                  const std::string &function_name,
                                  const FunctionSignature &signature) {
    // ライブラリがロードされているか確認
    if (loaded_libraries_.find(module_name) == loaded_libraries_.end()) {
        if (!loadLibrary(module_name)) {
            return false;
        }
    }

    LoadedLibrary &lib = loaded_libraries_[module_name];

    // 関数ポインタを取得
    void *func_ptr = getFunctionPointer(lib.handle, function_name);
    if (!func_ptr) {
        return false;
    }

    // 関数ポインタを保存
    lib.function_pointers[function_name] = func_ptr;

    // シグネチャを保存
    function_signatures_[module_name][function_name] = signature;

    return true;
}

Variable FFIManager::callFunction(const std::string &module_name,
                                  const std::string &function_name,
                                  const std::vector<Variable> &args) {
    // ライブラリと関数の存在確認
    if (loaded_libraries_.find(module_name) == loaded_libraries_.end()) {
        last_error_ = "Module not loaded: " + module_name;
        return Variable(); // エラー値
    }

    LoadedLibrary &lib = loaded_libraries_[module_name];

    if (lib.function_pointers.find(function_name) ==
        lib.function_pointers.end()) {
        last_error_ = "Function not registered: " + function_name;
        return Variable();
    }

    void *func_ptr = lib.function_pointers[function_name];
    const FunctionSignature &sig =
        function_signatures_[module_name][function_name];

    // 引数の数チェック
    if (args.size() != sig.parameters.size()) {
        last_error_ = "Argument count mismatch for " + function_name;
        return Variable();
    }

    Variable result;

    if (sig.return_type == TYPE_DOUBLE || sig.return_type == TYPE_FLOAT) {
        result.type = TYPE_DOUBLE;

        if (sig.parameters.size() == 1 &&
            sig.parameters[0].first == TYPE_DOUBLE) {
            // double func(double)
            typedef double (*func_type)(double);
            func_type func = reinterpret_cast<func_type>(func_ptr);
            double arg0 = args[0].double_value;
            result.double_value = func(arg0);
            result.value = static_cast<int64_t>(result.double_value);
            return result;
        } else if (sig.parameters.size() == 2 &&
                   sig.parameters[0].first == TYPE_DOUBLE &&
                   sig.parameters[1].first == TYPE_DOUBLE) {
            // double func(double, double)
            typedef double (*func_type)(double, double);
            func_type func = reinterpret_cast<func_type>(func_ptr);
            double arg0 = args[0].double_value;
            double arg1 = args[1].double_value;
            result.double_value = func(arg0, arg1);
            result.value = static_cast<int64_t>(result.double_value);
            return result;
        }
    } else if (sig.return_type == TYPE_INT) {
        result.type = TYPE_INT;

        if (sig.parameters.size() == 0) {
            // int func()
            typedef int (*func_type)();
            func_type func = reinterpret_cast<func_type>(func_ptr);
            result.value = func();
            return result;
        } else if (sig.parameters.size() == 1 &&
                   sig.parameters[0].first == TYPE_INT) {
            // int func(int)
            typedef int (*func_type)(int);
            func_type func = reinterpret_cast<func_type>(func_ptr);
            int arg0 = static_cast<int>(args[0].value);
            result.value = func(arg0);
            return result;
        } else if (sig.parameters.size() == 2 &&
                   sig.parameters[0].first == TYPE_INT &&
                   sig.parameters[1].first == TYPE_INT) {
            // int func(int, int)
            typedef int (*func_type)(int, int);
            func_type func = reinterpret_cast<func_type>(func_ptr);
            int arg0 = static_cast<int>(args[0].value);
            int arg1 = static_cast<int>(args[1].value);
            result.value = func(arg0, arg1);
            return result;
        }
    } else if (sig.return_type == TYPE_VOID) {
        // void func(...)
        if (sig.parameters.size() == 0) {
            typedef void (*func_type)();
            func_type func = reinterpret_cast<func_type>(func_ptr);
            func();
        } else if (sig.parameters.size() == 1 &&
                   sig.parameters[0].first == TYPE_INT) {
            typedef void (*func_type)(int);
            func_type func = reinterpret_cast<func_type>(func_ptr);
            func(static_cast<int>(args[0].value));
        }
        result.type = TYPE_VOID;
        return result;
    }

    last_error_ = "Unsupported function signature for " + function_name +
                  ": return type " + type_info_to_string(sig.return_type) +
                  " with " + std::to_string(sig.parameters.size()) +
                  " parameters";
    result.type = TYPE_UNKNOWN;
    return result;
}

void FFIManager::processForeignModule(const ASTNode *node) {
    if (!node || node->node_type != ASTNodeType::AST_FOREIGN_MODULE_DECL) {
        return;
    }

    if (!node->foreign_module_decl) {
        return;
    }

    const ForeignModuleDecl &module_decl = *(node->foreign_module_decl);
    std::string module_name = module_decl.module_name;

    // ライブラリをロード
    if (!loadLibrary(module_name)) {
        std::cerr << "Error: Failed to load library for module '" << module_name
                  << "': " << last_error_ << std::endl;
        return;
    }

    // 各関数を登録
    for (const auto &func_decl : module_decl.functions) {
        FunctionSignature sig;
        sig.return_type = func_decl.return_type;

        for (const auto &param : func_decl.parameters) {
            sig.parameters.push_back({param.type, param.name});
        }

        if (!registerFunction(module_name, func_decl.function_name, sig)) {
            std::cerr << "Error: Failed to register function '"
                      << func_decl.function_name << "': " << last_error_
                      << std::endl;
        }
    }
}

void *FFIManager::convertToCType(const Variable &var, const std::string &type) {
    // TODO: 将来の実装
    return nullptr;
}

Variable FFIManager::convertFromCType(void *ptr, const std::string &type) {
    // TODO: 将来の実装
    return Variable();
}

bool FFIManager::isForeignFunction(const std::string &function_name) const {
    // すべてのモジュールから関数を検索
    for (const auto &module_pair : function_signatures_) {
        const auto &func_map = module_pair.second;
        if (func_map.find(function_name) != func_map.end()) {
            return true;
        }
    }
    return false;
}

Variable FFIManager::callForeignFunction(const std::string &function_name,
                                         const std::vector<Variable> &args) {
    // すべてのモジュールから関数を検索
    for (const auto &module_pair : function_signatures_) {
        const std::string &module_name = module_pair.first;
        const auto &func_map = module_pair.second;

        auto it = func_map.find(function_name);
        if (it != func_map.end()) {
            // 見つかった場合、callFunctionを呼び出し
            last_result_ = callFunction(module_name, function_name, args);
            return last_result_;
        }
    }

    // 関数が見つからない場合
    last_error_ = "Foreign function not found: " + function_name;
    Variable result;
    result.type = TYPE_UNKNOWN;
    last_result_ = result;
    return result;
}

bool FFIManager::isForeignModuleLoaded(const std::string &module_name) const {
    return loaded_libraries_.find(module_name) != loaded_libraries_.end();
}

} // namespace cb
