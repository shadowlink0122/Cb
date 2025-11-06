# Allocators Tests

**カテゴリ**: メモリアロケータ  
**対象**: `stdlib/allocators/`

---

## 📖 概要

このディレクトリには、Cbの標準ライブラリのアロケータ（メモリ管理）に関するテストが含まれています。

---

## 📂 テストファイル

| ファイル | 対象 | テスト数 | 状態 |
|---------|------|---------|------|
| `test_system_allocator.cb` | SystemAllocator | 2 | ✅ |
| `test_bump_allocator.cb` | BumpAllocator | 2 | ✅ |

---

## 🧪 テスト対象

### SystemAllocator

- malloc/freeラッパー
- 汎用的なメモリアロケータ
- OS環境向け

**テスト項目**:
- 構造体の初期化
- インターフェース実装の確認

### BumpAllocator

- バンプアロケータ（リニアアロケータ）
- 高速な一時メモリ確保
- ベアメタル環境向け

**テスト項目**:
- 初期化と設定
- リセット機能
- deallocate無視の動作

---

## 🚀 実行方法

### 全てのアロケータテストを実行

```bash
./main tests/cases/stdlib/allocators/test_system_allocator.cb
./main tests/cases/stdlib/allocators/test_bump_allocator.cb
```

### make経由

```bash
make stdlib-test-cb
```

---

## 📝 新しいアロケータテストの追加

1. **stdlibファイル作成**
   ```cb
   // stdlib/allocators/new_allocator.cb
   export struct NewAllocator { };
   ```

2. **テストファイル作成**
   ```cb
   // tests/cases/stdlib/allocators/test_new_allocator.cb
   import stdlib.allocators.new_allocator;
   
   void test_new_allocator_basic() {
       println("✅ Test passed");
   }
   
   void main() {
       test_new_allocator_basic();
   }
   ```

3. **Makefileに追加**
   ```makefile
   @./$(MAIN_TARGET) tests/cases/stdlib/allocators/test_new_allocator.cb
   ```

---

## 🔗 関連ドキュメント

- **stdlib実装**: `stdlib/allocators/`
- **C++テスト**: `tests/stdlib/allocators/`
- **テスト構造**: `docs/testing/stdlib_test_structure.md`

---

**最終更新**: 2025年10月28日
