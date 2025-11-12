#!/bin/bash

cd /Users/shadowlink/Documents/git/Cb/docs/presentation/sections

for f in slide_{041..054}.html; do
  if [ -f "$f" ]; then
    # 小さすぎるフォントサイズを修正
    sed -i '' \
      -e 's/font-size: 0\.42em/font-size: 0.48em/g' \
      -e 's/font-size: 0\.40em/font-size: 0.48em/g' \
      -e 's/font-size: 0\.4em/font-size: 0.48em/g' \
      -e 's/font-size: 0\.45em/font-size: 0.50em/g' \
      -e 's/font-size: 0\.63em/font-size: 0.68em/g' \
      -e 's/font-size: 0\.60em; line-height: 1\.3; margin: 0; padding-left: 1\.3em/font-size: 0.65em; line-height: 1.3; margin: 0; padding-left: 1.3em/g' \
      "$f"
    echo "✓ Adjusted $f"
  fi
done

echo "Font size adjustment complete!"
