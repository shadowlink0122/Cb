/**
 * reveal.jsスライドを高品質PDFとしてエクスポート
 *
 * 使用方法:
 * 1. Node.jsとPuppeteerをインストール
 *    npm install
 *
 * 2. このスクリプトを実行
 *    npm run export:pdf
 *    または
 *    node export_to_pdf_hq.js
 *
 * 3. cb_presentation.pdfが生成される
 */

const puppeteer = require('puppeteer');
const path = require('path');
const fs = require('fs');

(async () => {
  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
  console.log('  Cb Presentation → PDF Exporter');
  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
  console.log('');

  // プレゼンテーションファイルの存在確認
  const presentationPath = path.join(__dirname, 'cb_interpreter_presentation.html');

  if (!fs.existsSync(presentationPath)) {
    console.error('❌ Error: cb_interpreter_presentation.html not found');
    console.error(`   Expected at: ${presentationPath}`);
    process.exit(1);
  }

  console.log('✓ Presentation file found');

  try {
    console.log('📦 Launching browser...');
    const browser = await puppeteer.launch({
      headless: 'new',
      args: ['--no-sandbox', '--disable-setuid-sandbox']
    });

    const page = await browser.newPage();

    // プレゼンテーションファイルのパス
    const fileUrl = `file://${presentationPath}?print-pdf`;

    console.log('📄 Loading presentation...');
    console.log(`   URL: ${fileUrl}`);

    await page.goto(fileUrl, {
      waitUntil: 'networkidle2',
      timeout: 60000  // 60秒タイムアウト
    });

    console.log('✓ Presentation loaded');

    // reveal.jsの印刷モードが準備されるまで待機
    console.log('⏳ Waiting for reveal.js print mode...');
    await page.waitForTimeout(3000);

    const outputPath = path.join(__dirname, 'cb_presentation.pdf');

    console.log('🖨️  Generating PDF...');
    await page.pdf({
      path: outputPath,
      format: 'A4',
      landscape: true,
      printBackground: true,
      preferCSSPageSize: true,
      margin: {
        top: 0,
        right: 0,
        bottom: 0,
        left: 0
      }
    });

    await browser.close();

    console.log('');
    console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
    console.log('✅ PDF export completed successfully!');
    console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
    console.log('');
    console.log(`📄 PDF saved to: ${outputPath}`);
    console.log('');
    console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
    console.log('Next steps:');
    console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
    console.log('');
    console.log('🍎 For Mac users (Keynote):');
    console.log('  1. Open Keynote');
    console.log('  2. File → Open → Select cb_presentation.pdf');
    console.log('  3. File → Export → Keynote (.key file)');
    console.log('  4. Upload .key file to Google Drive');
    console.log('  5. Open with Google Slides');
    console.log('');
    console.log('🌐 Direct PDF import to Google Slides:');
    console.log('  1. Open Google Slides (slides.google.com)');
    console.log('  2. File → Import slides');
    console.log('  3. Upload → Select cb_presentation.pdf');
    console.log('  4. Click "Import all"');
    console.log('');
    console.log('💡 For detailed instructions:');
    console.log('   - See QUICK_START.md');
    console.log('   - See KEYNOTE_GUIDE.md (Mac)');
    console.log('');
    console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');

  } catch (error) {
    console.error('');
    console.error('❌ Error during PDF generation:');
    console.error(error.message);
    console.error('');
    console.error('Troubleshooting:');
    console.error('  - Make sure Puppeteer is installed: npm install');
    console.error('  - Check internet connection (CDN resources)');
    console.error('  - Try increasing timeout in the script');
    console.error('  - See QUICK_START.md for more help');
    process.exit(1);
  }
})();
