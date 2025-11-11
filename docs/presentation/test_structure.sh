#!/bin/bash
# プレゼンテーション構造の整合性チェック

echo "🔍 プレゼンテーション構造チェック"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# スライド数チェック
SLIDE_COUNT=$(ls -1 sections/slide_*.html 2>/dev/null | wc -l)
echo "✓ スライド数: $SLIDE_COUNT ファイル"

# presentation.html内のスライド数チェック
HTML_SECTION_COUNT=$(grep -o "</section>" presentation.html | wc -l)
echo "✓ presentation.html内のセクション数: $HTML_SECTION_COUNT"

# CSSファイルチェック
if [ -f "css/styles.css" ]; then
    CSS_LINES=$(wc -l < css/styles.css)
    echo "✓ CSS: $CSS_LINES 行"
else
    echo "✗ CSSファイルが見つかりません"
fi

# ビルドスクリプトチェック
if [ -x "build.sh" ]; then
    echo "✓ build.sh: 実行可能"
else
    echo "✗ build.shが実行可能ではありません"
fi

# プロフィール画像チェック
if [ -f "profile.jpg" ]; then
    if command -v sips &> /dev/null; then
        IMG_SIZE=$(sips -g pixelWidth -g pixelHeight profile.jpg 2>/dev/null | tail -2 | awk '{print $2}' | tr '\n' 'x' | sed 's/x$//')
        echo "✓ profile.jpg: ${IMG_SIZE}"
    else
        echo "✓ profile.jpg: 存在"
    fi
else
    echo "✗ profile.jpgが見つかりません"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ "$SLIDE_COUNT" -eq "$HTML_SECTION_COUNT" ]; then
    echo "✅ すべてのチェックが正常です！"
else
    echo "⚠️  スライド数が一致しません"
fi
