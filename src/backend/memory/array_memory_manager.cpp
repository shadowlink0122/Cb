#include "array_memory_manager.h"
#include "../../common/debug_messages.h"
#include <cstdlib>
#include <cstring>
#include <cassert>

#ifdef DEBUG
#include <iostream>
#endif

// グローバル配列メモリマネージャーのインスタンス
static ArrayMemoryManager global_array_manager;

ArrayMemoryManager& get_global_array_manager() noexcept {
    return global_array_manager;
}

// 要素サイズの静的計算（コンパイル時最適化）
constexpr size_t get_element_size(TypeInfo type) noexcept {
    switch (type) {
        case TYPE_TINY: return sizeof(int8_t);
        case TYPE_SHORT: return sizeof(int16_t);
        case TYPE_INT: return sizeof(int32_t);
        case TYPE_LONG: return sizeof(int64_t);
        case TYPE_BOOL: return sizeof(bool);
        case TYPE_STRING: return sizeof(void*); // 文字列ポインタ
        default: return sizeof(int32_t);
    }
}

// ArrayMemoryBlock実装
ArrayMemoryBlock::ArrayMemoryBlock(TypeInfo type, const ArrayDimensionInfo& dimensions)
    : element_type(type), 
      element_size(get_element_size(type)),
      dims(dimensions),
      total_size([&]() -> size_t {
          size_t total_elements = 1;
          for (uint8_t i = 0; i < dimensions.dimension_count; ++i) {
              total_elements *= dimensions.dimensions[i];
          }
          return total_elements * element_size;
      }()),
      data([&]() -> void* {
          void* ptr = std::aligned_alloc(alignof(std::max_align_t), total_size);
          if (!ptr) {
              throw std::bad_alloc();
          }
          // ゼロ初期化（高速なmemset）
          std::memset(ptr, 0, total_size);
          return ptr;
      }()) {
    
    #ifdef DEBUG
    debug_msg(DebugMsgId::ARRAY_ALLOC, 
              std::to_string(total_size / element_size).c_str(),
              std::to_string(total_size).c_str());
    #endif
}

ArrayMemoryBlock::~ArrayMemoryBlock() {
    if (data) {
        std::free(const_cast<void*>(data));
    }
}

// 可変長引数テンプレート版のオフセット計算
template<typename... Args>
inline size_t ArrayMemoryBlock::get_linear_offset(Args... indices) const noexcept {
    constexpr size_t num_indices = sizeof...(indices);
    static_assert(num_indices <= MAX_ARRAY_DIMENSIONS, "Too many array dimensions");
    
    size_t index_array[] = {static_cast<size_t>(indices)...};
    size_t offset = 0;
    size_t multiplier = 1;
    
    // 行優先順序でオフセット計算（最適化されたループ展開）
    for (int i = num_indices - 1; i >= 0; --i) {
        offset += index_array[i] * multiplier;
        multiplier *= dims.dimensions[i];
    }
    
    return offset;
}

#ifdef DEBUG
template<typename T>
T* ArrayMemoryBlock::get_element_ptr_checked(size_t linear_offset) const {
    assert(linear_offset * element_size < total_size && "Array access out of bounds");
    return get_element_ptr_unchecked<T>(linear_offset);
}
#endif

// 明示的インスタンス化（よく使われる型のみ）
template size_t ArrayMemoryBlock::get_linear_offset<size_t>(size_t) const noexcept;
template size_t ArrayMemoryBlock::get_linear_offset<size_t, size_t>(size_t, size_t) const noexcept;
template size_t ArrayMemoryBlock::get_linear_offset<size_t, size_t, size_t>(size_t, size_t, size_t) const noexcept;

template int32_t* ArrayMemoryBlock::get_element_ptr_unchecked<int32_t>(size_t) const noexcept;
template int64_t* ArrayMemoryBlock::get_element_ptr_unchecked<int64_t>(size_t) const noexcept;

// ArrayMemoryManager実装
ArrayMemoryManager::~ArrayMemoryManager() {
    // 全配列を削除
    for (size_t i = 0; i < MAX_MANAGED_ARRAYS; ++i) {
        if (handles_[i].in_use && handles_[i].block) {
            delete handles_[i].block;
        }
    }
}

// 線形探索によるハンドル検索（小さな配列数では効率的）
ArrayHandle* ArrayMemoryManager::find_handle(const char* name) noexcept {
    if (!name) return nullptr;
    
    for (size_t i = 0; i < MAX_MANAGED_ARRAYS; ++i) {
        if (handles_[i].in_use && handles_[i].name && 
            std::strcmp(handles_[i].name, name) == 0) {
            return &handles_[i];
        }
    }
    return nullptr;
}

// const版のfind_handle
const ArrayHandle* ArrayMemoryManager::find_handle(const char* name) const noexcept {
    if (!name) return nullptr;
    
    for (size_t i = 0; i < MAX_MANAGED_ARRAYS; ++i) {
        if (handles_[i].in_use && handles_[i].name && 
            std::strcmp(handles_[i].name, name) == 0) {
            return &handles_[i];
        }
    }
    return nullptr;
}

ArrayHandle* ArrayMemoryManager::find_free_handle() noexcept {
    for (size_t i = 0; i < MAX_MANAGED_ARRAYS; ++i) {
        if (!handles_[i].in_use) {
            return &handles_[i];
        }
    }
    return nullptr;
}

ArrayMemoryBlock* ArrayMemoryManager::create_array_1d(const char* name, TypeInfo element_type, size_t size) {
    if (!name || find_handle(name)) {
        return nullptr; // 名前が無効または既に存在
    }
    
    ArrayHandle* handle = find_free_handle();
    if (!handle) {
        return nullptr; // 管理配列数上限
    }
    
    ArrayDimensionInfo dims;
    dims.dimension_count = 1;
    dims.dimensions[0] = size;
    
    try {
        ArrayMemoryBlock* block = new ArrayMemoryBlock(element_type, dims);
        
        // 名前文字列をコピー（簡易実装）
        size_t name_len = std::strlen(name) + 1;
        char* name_copy = static_cast<char*>(std::malloc(name_len));
        std::strcpy(name_copy, name);
        
        handle->name = name_copy;
        handle->block = block;
        handle->in_use = true;
        ++array_count_;
        
        #ifdef DEBUG
        debug_msg(DebugMsgId::ARRAY_CREATE, name);
        #endif
        
        return block;
        
    } catch (const std::bad_alloc&) {
        return nullptr;
    }
}

ArrayMemoryBlock* ArrayMemoryManager::create_array_2d(const char* name, TypeInfo element_type,
                                                     size_t rows, size_t cols) {
    if (!name || find_handle(name)) {
        return nullptr;
    }
    
    ArrayHandle* handle = find_free_handle();
    if (!handle) {
        return nullptr;
    }
    
    ArrayDimensionInfo dims;
    dims.dimension_count = 2;
    dims.dimensions[0] = rows;
    dims.dimensions[1] = cols;
    
    try {
        ArrayMemoryBlock* block = new ArrayMemoryBlock(element_type, dims);
        
        size_t name_len = std::strlen(name) + 1;
        char* name_copy = static_cast<char*>(std::malloc(name_len));
        std::strcpy(name_copy, name);
        
        handle->name = name_copy;
        handle->block = block;
        handle->in_use = true;
        ++array_count_;
        
        #ifdef DEBUG
        debug_msg(DebugMsgId::ARRAY_CREATE, name);
        #endif
        
        return block;
        
    } catch (const std::bad_alloc&) {
        return nullptr;
    }
}

ArrayMemoryBlock* ArrayMemoryManager::create_array_3d(const char* name, TypeInfo element_type,
                                                     size_t d1, size_t d2, size_t d3) {
    if (!name || find_handle(name)) {
        return nullptr;
    }
    
    ArrayHandle* handle = find_free_handle();
    if (!handle) {
        return nullptr;
    }
    
    ArrayDimensionInfo dims;
    dims.dimension_count = 3;
    dims.dimensions[0] = d1;
    dims.dimensions[1] = d2;
    dims.dimensions[2] = d3;
    
    try {
        ArrayMemoryBlock* block = new ArrayMemoryBlock(element_type, dims);
        
        size_t name_len = std::strlen(name) + 1;
        char* name_copy = static_cast<char*>(std::malloc(name_len));
        std::strcpy(name_copy, name);
        
        handle->name = name_copy;
        handle->block = block;
        handle->in_use = true;
        ++array_count_;
        
        #ifdef DEBUG
        debug_msg(DebugMsgId::ARRAY_CREATE, name);
        #endif
        
        return block;
        
    } catch (const std::bad_alloc&) {
        return nullptr;
    }
}

void ArrayMemoryManager::destroy_array(const char* name) noexcept {
    ArrayHandle* handle = find_handle(name);
    if (!handle) return;
    
    delete handle->block;
    std::free(const_cast<char*>(handle->name));
    
    handle->name = nullptr;
    handle->block = nullptr;
    handle->in_use = false;
    --array_count_;
    
    #ifdef DEBUG
    debug_msg(DebugMsgId::ARRAY_DESTROY, name);
    #endif
}

size_t ArrayMemoryManager::get_total_memory_usage() const noexcept {
    size_t total = 0;
    for (size_t i = 0; i < MAX_MANAGED_ARRAYS; ++i) {
        if (handles_[i].in_use && handles_[i].block) {
            total += handles_[i].block->total_size;
        }
    }
    return total;
}

#ifdef DEBUG
void ArrayMemoryManager::dump_array_info(const char* name) const {
    const ArrayHandle* handle = find_handle(name);
    if (!handle || !handle->block) {
        printf("[DEBUG] Array '%s' not found\n", name);
        return;
    }
    
    const ArrayMemoryBlock* block = handle->block;
    printf("[DEBUG] Array '%s':\n", name);
    printf("  - Type: %d\n", (int)block->element_type);
    printf("  - Element size: %zu bytes\n", block->element_size);
    printf("  - Total size: %zu bytes\n", block->total_size);
    printf("  - Dimensions: ");
    for (uint8_t i = 0; i < block->dims.dimension_count; ++i) {
        if (i > 0) printf(" x ");
        printf("%zu", block->dims.dimensions[i]);
    }
    printf("\n");
    printf("  - Memory address: %p\n", block->data);
}

void ArrayMemoryManager::dump_all_arrays() const {
    printf("[DEBUG] Array Memory Manager Status:\n");
    printf("  - Active arrays: %zu / %d\n", array_count_, MAX_MANAGED_ARRAYS);
    printf("  - Total memory usage: %zu bytes\n", get_total_memory_usage());
    
    for (size_t i = 0; i < MAX_MANAGED_ARRAYS; ++i) {
        if (handles_[i].in_use) {
            printf("  - [%zu] %s (%zu bytes)\n", i, handles_[i].name, 
                   handles_[i].block ? handles_[i].block->total_size : 0);
        }
    }
}
#else
void ArrayMemoryManager::dump_array_info(const char* name) const { /* no-op in release */ }
void ArrayMemoryManager::dump_all_arrays() const { /* no-op in release */ }
#endif
