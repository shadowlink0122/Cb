// =================================================================
// Cb Language - Improved Array System Design
// =================================================================

#pragma once
#include <memory>
#include <vector>
#include <map>

// メモリブロック管理（ガベージコレクション対応）
class MemoryBlock {
public:
    void* data;
    size_t size;
    size_t element_size;
    TypeInfo element_type;
    std::vector<size_t> dimensions; // 多次元配列の各次元サイズ
    
    MemoryBlock(size_t total_size, size_t elem_size, TypeInfo type, 
                const std::vector<size_t>& dims)
        : size(total_size), element_size(elem_size), element_type(type), 
          dimensions(dims) {
        data = std::malloc(total_size);
    }
    
    ~MemoryBlock() {
        std::free(data);
    }
    
    // 多次元インデックス → 線形インデックス変換
    size_t calculate_linear_index(const std::vector<size_t>& indices) const {
        size_t linear_index = 0;
        size_t multiplier = 1;
        
        // 行優先（row-major）順序でインデックス計算
        for (int i = dimensions.size() - 1; i >= 0; --i) {
            linear_index += indices[i] * multiplier;
            multiplier *= dimensions[i];
        }
        
        return linear_index;
    }
    
    // 要素アクセス（型安全）
    template<typename T>
    T* get_element(const std::vector<size_t>& indices) {
        size_t index = calculate_linear_index(indices);
        return static_cast<T*>(data) + index;
    }
};

// メモリ管理システム
class ArrayMemoryManager {
private:
    std::map<std::string, std::unique_ptr<MemoryBlock>> memory_blocks_;
    
public:
    // 配列メモリ割り当て
    bool allocate_array(const std::string& var_name, 
                       TypeInfo element_type,
                       const std::vector<size_t>& dimensions) {
        
        size_t element_size = get_type_size(element_type);
        size_t total_elements = 1;
        for (auto dim : dimensions) {
            total_elements *= dim;
        }
        
        auto block = std::make_unique<MemoryBlock>(
            total_elements * element_size, element_size, element_type, dimensions);
            
        memory_blocks_[var_name] = std::move(block);
        return true;
    }
    
    // 配列アクセス
    MemoryBlock* get_array(const std::string& var_name) {
        auto it = memory_blocks_.find(var_name);
        return (it != memory_blocks_.end()) ? it->second.get() : nullptr;
    }
    
    // メモリ解放
    void deallocate_array(const std::string& var_name) {
        memory_blocks_.erase(var_name);
    }
    
private:
    size_t get_type_size(TypeInfo type) {
        switch (type) {
            case TYPE_TINY: return sizeof(int8_t);
            case TYPE_SHORT: return sizeof(int16_t);
            case TYPE_INT: return sizeof(int32_t);
            case TYPE_LONG: return sizeof(int64_t);
            case TYPE_BOOL: return sizeof(bool);
            case TYPE_STRING: return sizeof(std::string);
            default: return sizeof(int32_t);
        }
    }
};

// 改善された変数構造
struct ImprovedVariable {
    TypeInfo type;
    bool is_const;
    bool is_assigned;
    
    // スカラー値
    union {
        int64_t int_value;
        double double_value;
        bool bool_value;
    };
    std::string str_value;
    
    // 配列の場合はメモリブロックへの参照
    std::string array_memory_key;  // ArrayMemoryManagerでの識別子
    
    // 配列かどうかの判定
    bool is_array() const {
        return !array_memory_key.empty();
    }
};
