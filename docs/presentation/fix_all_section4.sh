#!/bin/bash

cd "$(dirname "$0")/sections"

echo "Applying uniform fixes to Section 4+ slides..."

for num in {041..055}; do
    file="slide_${num}.html"
    if [ -f "$file" ]; then
        echo "Processing $file..."
        
        # Reduce font sizes uniformly
        sed -i '' 's/font-size: 0\.85em/font-size: 0.78em/g' "$file"
        sed -i '' 's/font-size: 0\.75em/font-size: 0.70em/g' "$file"
        sed -i '' 's/font-size: 0\.72em/font-size: 0.65em/g' "$file"
        sed -i '' 's/font-size: 0\.70em/font-size: 0.63em/g' "$file"
        sed -i '' 's/font-size: 0\.68em/font-size: 0.62em/g' "$file"
        
        # Reduce line heights
        sed -i '' 's/line-height: 1\.4/line-height: 1.3/g' "$file"
        sed -i '' 's/line-height: 1\.35/line-height: 1.25/g' "$file"
        
        # Reduce margins
        sed -i '' 's/margin-bottom: 0\.4em/margin-bottom: 0.3em/g' "$file"
        sed -i '' 's/margin-bottom: 0\.3em/margin-bottom: 0.25em/g' "$file"
        sed -i '' 's/margin-top: 0\.6em/margin-top: 0.5em/g' "$file"
        sed -i '' 's/margin-top: 0\.5em/margin-top: 0.4em/g' "$file"
        sed -i '' 's/margin: 0\.5em/margin: 0.4em/g' "$file"
        
        echo "  ✓ $file"
    fi
done

echo "Done! Rebuild with ./build.sh"
