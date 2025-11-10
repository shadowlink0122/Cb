#!/bin/bash

# Cb Presentation Export Script
# PDFおよび画像形式でスライドをエクスポート

set -e  # エラーが発生したら停止

# 色付き出力
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}==================================================${NC}"
echo -e "${BLUE}  Cb Presentation Exporter${NC}"
echo -e "${BLUE}==================================================${NC}"
echo ""

# スクリプトのディレクトリに移動
cd "$(dirname "$0")"

# Node.jsがインストールされているか確認
if ! command -v node &> /dev/null; then
    echo -e "${RED}❌ Error: Node.js is not installed${NC}"
    echo -e "${YELLOW}Please install Node.js from https://nodejs.org/${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Node.js found: $(node --version)${NC}"

# package.jsonが存在するか確認
if [ ! -f "package.json" ]; then
    echo -e "${RED}❌ Error: package.json not found${NC}"
    exit 1
fi

# node_modulesが存在しない場合、依存関係をインストール
if [ ! -d "node_modules" ]; then
    echo -e "${YELLOW}📦 Installing dependencies...${NC}"
    npm install
    echo -e "${GREEN}✓ Dependencies installed${NC}"
    echo ""
fi

# エクスポートタイプを選択
echo "Select export format:"
echo "  1) PDF (高品質PDF - Keynote/Google Slides用)"
echo "  2) Images (各スライドをPNG画像として保存)"
echo "  3) Both (PDFと画像の両方)"
echo ""
read -p "Enter your choice (1-3): " choice

case $choice in
    1)
        echo ""
        echo -e "${BLUE}Exporting to PDF...${NC}"
        npm run export:pdf
        echo ""
        echo -e "${GREEN}✅ PDF export completed!${NC}"
        echo -e "${YELLOW}📄 File: cb_presentation.pdf${NC}"
        ;;
    2)
        echo ""
        echo -e "${BLUE}Exporting to images...${NC}"
        npm run export:images
        echo ""
        echo -e "${GREEN}✅ Image export completed!${NC}"
        echo -e "${YELLOW}📁 Folder: slides/${NC}"
        ;;
    3)
        echo ""
        echo -e "${BLUE}Exporting to PDF and images...${NC}"
        npm run export:all
        echo ""
        echo -e "${GREEN}✅ All exports completed!${NC}"
        echo -e "${YELLOW}📄 PDF: cb_presentation.pdf${NC}"
        echo -e "${YELLOW}📁 Images: slides/${NC}"
        ;;
    *)
        echo -e "${RED}Invalid choice. Exiting.${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${BLUE}==================================================${NC}"
echo -e "${GREEN}Next steps:${NC}"
echo ""

if [ "$choice" == "1" ] || [ "$choice" == "3" ]; then
    echo -e "${YELLOW}For Keynote (Mac):${NC}"
    echo "  1. Open Keynote"
    echo "  2. File → Open → Select cb_presentation.pdf"
    echo "  3. File → Export → Keynote (.key file)"
    echo "  4. Upload .key file to Google Drive"
    echo "  5. Open with Google Slides"
    echo ""
fi

if [ "$choice" == "2" ] || [ "$choice" == "3" ]; then
    echo -e "${YELLOW}For Google Slides (Images):${NC}"
    echo "  1. Create new Google Slides presentation"
    echo "  2. For each slide:"
    echo "     - Insert → Image → Upload from computer"
    echo "     - Select slide_XX.png from slides/ folder"
    echo ""
fi

echo -e "${BLUE}For detailed instructions, see:${NC}"
echo "  - README.md (overview)"
echo "  - KEYNOTE_GUIDE.md (Keynote specific)"
echo ""
echo -e "${BLUE}==================================================${NC}"
