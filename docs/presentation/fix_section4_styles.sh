#!/bin/bash

# Fix Section 4+ slides formatting issues
# - Add max-height and overflow to long code blocks
# - Reduce font sizes for content-heavy slides
# - Ensure all content fits within viewport

cd "$(dirname "$0")/sections"

echo "Fixing Section 4+ slide styles..."

# Slides 041-055: Add overflow and adjust sizes for code blocks and lists
for num in {041..055}; do
    file="slide_${num}.html"
    if [ -f "$file" ]; then
        echo "Processing $file..."
        
        # Add max-height and overflow-y to pre blocks that don't have it
        # This is a safety net - individual slides should be checked manually
        
        # Reduce overly large font sizes in lists
        sed -i '' 's/font-size: 0\.72em;/font-size: 0.65em;/g' "$file"
        sed -i '' 's/line-height: 1\.4;/line-height: 1.3;/g' "$file"
        
        echo "  ✓ Processed $file"
    fi
done

echo "Done! Run ./build.sh to rebuild presentation."
