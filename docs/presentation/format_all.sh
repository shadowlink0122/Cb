#!/bin/bash
echo "HTMLファイルをフォーマット中..."
if npx prettier --write "sections/**/*.html" "*.html" --ignore-path .prettierignore 2>&1 | grep -q "\\[error\\]"; then
    echo "Prettierでエラーが発生しました"
    exit 1
else
    echo "完了"
fi
