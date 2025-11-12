#!/bin/bash
# スライド再編成スクリプト

cd "$(dirname "$0")"

echo "=== スライド再編成開始 ==="

# 1. 重複スライドを削除
echo "重複スライドを削除中..."
rm -f sections/slide_019.html  # カプセル化重複(014を残す)
rm -f sections/slide_020.html  # import重複(015を残す) 
rm -f sections/slide_036.html  # async/await重複(018を残す)

# 2. 2024->2025の一括修正
echo "年号を2024->2025に修正中..."
find sections -name "*.html" -exec sed -i '' 's/2024年/2025年/g' {} \;
find sections -name "*.html" -exec sed -i '' 's/2024/2025/g' {} \;

# 3. CB->Cbの修正
echo "CB->Cbに修正中..."
find sections -name "*.html" -exec sed -i '' 's/\bCB\b/Cb/g' {} \;

echo "=== 基本修正完了 ==="
echo "次のステップ: 新規スライド追加"
