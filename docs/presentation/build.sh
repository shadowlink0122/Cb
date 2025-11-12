#!/bin/bash
# プレゼンテーションをビルドするスクリプト
# 個別のスライドファイルから1つのHTMLファイルを生成

SECTIONS_DIR="sections"
CSS_FILE="css/styles.css"
OUTPUT_FILE="presentation.html"

echo "Building presentation..."

# HTMLヘッダー
cat > "$OUTPUT_FILE" << 'EOF'
<!doctype html>
<html lang="ja">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>バイブコーディングで作る自作言語Cbインタプリタ！</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/reveal.js@4.5.0/dist/reset.css">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/reveal.js@4.5.0/dist/reveal.css">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/reveal.js@4.5.0/dist/theme/white.css">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/reveal.js@4.5.0/plugin/highlight/monokai.css">
    <style>
EOF

# CSSを追加
cat "$CSS_FILE" >> "$OUTPUT_FILE"

# スタイルタグを閉じてbodyを開始
cat >> "$OUTPUT_FILE" << 'EOF'
    </style>
</head>
<body>
    <div class="reveal">
        <div class="slides">
EOF

# 全スライドを追加
for slide in $(ls -1 "$SECTIONS_DIR"/slide_*.html | sort); do
    cat "$slide" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
done

# HTMLフッター
cat >> "$OUTPUT_FILE" << 'EOF'
        </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/reveal.js@4.5.0/dist/reveal.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/reveal.js@4.5.0/plugin/highlight/highlight.js"></script>
    <script>
        Reveal.initialize({
            hash: true,
            slideNumber: true,
            transition: 'slide',
            transitionSpeed: 'default',
            controls: true,
            progress: true,
            center: true,
            plugins: [ RevealHighlight ]
        });
    </script>
</body>
</html>
EOF

echo "✓ Presentation built: $OUTPUT_FILE"
echo "  Total slides: $(ls -1 "$SECTIONS_DIR"/slide_*.html | wc -l)"
