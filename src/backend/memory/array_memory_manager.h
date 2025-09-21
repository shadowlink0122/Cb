#pragma once
#include "../../common/ast.h"
#include <cstddef>
#include <cstdint>

// 固定長配列の次元情報（最大4次元まで対応）
#define MAX_ARRAY_DIMENSIONS 4

struct ArrayDimensionInfo {
    size_t dimensions[MAX_ARRAY_DIMENSIONS];
    uint8_t dimension_count;
    
    ArrayDimensionInfo() : dimension_count(0) {
        for (int i = 0; i < MAX_ARRAY_DIMENSIONS; ++i) {
            dimensions[i] = 0;
        }
    }
};

// ゼロオーバーヘッド配列メモリブロック
// Rustのsliceやmodern C++のspan的なアプローチ
class ArrayMemoryBlock {
public:
    void* const data;              // 不変ポインタ（メモリ位置固定）
    const size_t total_size;       // 総バイト数
    const size_t element_size;     // 要素サイズ
    const TypeInfo element_type;   // 要素型
    const ArrayDimensionInfo dims; // 次元情報（固定配列）
    
    // コンストラクタ：メモリ確保と初期化を一度に実行
    ArrayMemoryBlock(TypeInfo type, const ArrayDimensionInfo& dimensions);
    ~ArrayMemoryBlock();
    
    // 削除されたコピー/移動操作（メモリ安全性）
    ArrayMemoryBlock(const ArrayMemoryBlock&) = delete;
    ArrayMemoryBlock& operator=(const ArrayMemoryBlock&) = delete;
    ArrayMemoryBlock(ArrayMemoryBlock&&) = delete;
    ArrayMemoryBlock& operator=(ArrayMemoryBlock&&) = delete;
    
    // インライン化された高速アクセス関数
    inline size_t get_linear_offset_1d(size_t index) const noexcept {
        return index;
    }
    
    inline size_t get_linear_offset_2d(size_t i, size_t j) const noexcept {
        return i * dims.dimensions[1] + j;
    }
    
    inline size_t get_linear_offset_3d(size_t i, size_t j, size_t k) const noexcept {
        return i * dims.dimensions[1] * dims.dimensions[2] + 
               j * dims.dimensions[2] + k;
    }
    
    // 汎用多次元オフセット計算（可変長引数版）
    template<typename... Args>
    inline size_t get_linear_offset(Args... indices) const noexcept;
    
    // ゼロコスト抽象化：型安全な要素アクセス
    template<typename T>
    inline T* get_element_ptr_unchecked(size_t linear_offset) const noexcept {
        return static_cast<T*>(data) + linear_offset;
    }
    
    template<typename T>
    inline T& get_element_ref(size_t linear_offset) const noexcept {
        return *static_cast<T*>(data) + linear_offset;
    }
    
    // 境界チェック付きアクセス（デバッグ時のみ）
    #ifdef DEBUG
    template<typename T>
    T* get_element_ptr_checked(size_t linear_offset) const;
    #else
    template<typename T>
    inline T* get_element_ptr_checked(size_t linear_offset) const noexcept {
        return get_element_ptr_unchecked<T>(linear_offset);
    }
    #endif
    
    // スタック配列風の直接アクセス
    inline void* operator[](size_t index) const noexcept {
        return static_cast<char*>(data) + (index * element_size);
    }
};

// 軽量配列管理システム（ハッシュマップの代わりに線形探索）
#define MAX_MANAGED_ARRAYS 64

struct ArrayHandle {
    const char* name;
    ArrayMemoryBlock* block;
    bool in_use;
    
    ArrayHandle() : name(nullptr), block(nullptr), in_use(false) {}
};

class ArrayMemoryManager {
private:
    ArrayHandle handles_[MAX_MANAGED_ARRAYS];
    size_t array_count_;
    
    // 名前でハンドル検索（線形探索：キャッシュフレンドリー）
    ArrayHandle* find_handle(const char* name) noexcept;
    const ArrayHandle* find_handle(const char* name) const noexcept;
    ArrayHandle* find_free_handle() noexcept;
    
public:
    ArrayMemoryManager() : array_count_(0) {}
    ~ArrayMemoryManager();
    
    // 配列作成（固定長、実行時サイズ決定）
    ArrayMemoryBlock* create_array_1d(const char* name, TypeInfo element_type, size_t size);
    ArrayMemoryBlock* create_array_2d(const char* name, TypeInfo element_type, 
                                     size_t rows, size_t cols);
    ArrayMemoryBlock* create_array_3d(const char* name, TypeInfo element_type,
                                     size_t d1, size_t d2, size_t d3);
    
    // 配列取得（O(1)アクセスを目指した設計）
    inline ArrayMemoryBlock* get_array(const char* name) noexcept {
        ArrayHandle* handle = find_handle(name);
        return handle ? handle->block : nullptr;
    }
    
    // 配列削除
    void destroy_array(const char* name) noexcept;
    
    // 統計情報
    inline size_t get_array_count() const noexcept { return array_count_; }
    size_t get_total_memory_usage() const noexcept;
    
    // デバッグ用
    void dump_array_info(const char* name) const;
    void dump_all_arrays() const;
};

// グローバル配列メモリマネージャー（シングルトン）
ArrayMemoryManager& get_global_array_manager() noexcept;
