#ifndef CB_FFI_MANAGER_H
#define CB_FFI_MANAGER_H

#include "src/backend/interpreter/core/interpreter.h"
#include "src/common/ast.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace cb {

struct FunctionSignature {
    TypeInfo return_type;
    std::vector<std::pair<TypeInfo, std::string>> parameters; // (type, name)
};

struct LoadedLibrary {
    void *handle;
    std::string path;
    std::map<std::string, void *> function_pointers;
};

class FFIManager {
  public:
    FFIManager();
    ~FFIManager();

    // ライブラリのロード
    bool loadLibrary(const std::string &module_name,
                     const std::string &library_path = "");

    // 関数の登録
    bool registerFunction(const std::string &module_name,
                          const std::string &function_name,
                          const FunctionSignature &signature);

    // 関数の呼び出し
    Variable callFunction(const std::string &module_name,
                          const std::string &function_name,
                          const std::vector<Variable> &args);

    // ライブラリパスの解決
    std::string resolveLibraryPath(const std::string &module_name);

    // 外部モジュール宣言の処理
    void processForeignModule(const ASTNode *node);

    // 外部関数かどうかをチェック
    bool isForeignFunction(const std::string &function_name) const;

    // モジュール名なしで外部関数を呼び出し（全モジュールから検索）
    Variable callForeignFunction(const std::string &function_name,
                                 const std::vector<Variable> &args);

    // 外部モジュールが読み込まれているかチェック
    bool isForeignModuleLoaded(const std::string &module_name) const;

    // エラーメッセージ取得
    std::string getLastError() const { return last_error_; }

    // 最後の結果取得（v0.13.0: double返り値対応）
    const Variable &getLastResult() const { return last_result_; }

  private:
    std::map<std::string, LoadedLibrary> loaded_libraries_;
    std::map<std::string, std::map<std::string, FunctionSignature>>
        function_signatures_;
    std::vector<std::string> search_paths_;
    std::string last_error_;
    Variable last_result_; // 最後の関数呼び出し結果

    // ヘルパー関数
    void initializeSearchPaths();
    bool loadLibraryHandle(const std::string &path, void **handle);
    void *getFunctionPointer(void *lib_handle, const std::string &func_name);

    // 型変換（将来の実装）
    void *convertToCType(const Variable &var, const std::string &type);
    Variable convertFromCType(void *ptr, const std::string &type);
};

} // namespace cb

#endif // CB_FFI_MANAGER_H
