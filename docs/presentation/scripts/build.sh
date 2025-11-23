#!/bin/bash

# Build script for Cb presentation

echo "🔨 Building presentation..."

# Install dependencies if needed
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies..."
    npm install
fi

# Type check
echo "✅ Running type check..."
npm run type-check

# Build the project
echo "🏗️  Building project..."
npm run build

echo "✨ Build complete! Output in dist/"
echo "📝 To preview: npm run preview"