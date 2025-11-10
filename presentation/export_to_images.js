/**
 * reveal.jsスライドを個別の画像ファイルとしてエクスポート
 *
 * 使用方法:
 * 1. Node.jsとPuppeteerをインストール
 *    npm install puppeteer
 *
 * 2. このスクリプトを実行
 *    node export_to_images.js
 *
 * 3. slides/フォルダに各スライドのPNG画像が生成される
 */

const puppeteer = require('puppeteer');
const fs = require('fs');
const path = require('path');

(async () => {
  console.log('Starting slide export...');

  // 出力ディレクトリの作成
  const outputDir = path.join(__dirname, 'slides');
  if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir);
  }

  const browser = await puppeteer.launch({
    headless: 'new'
  });

  const page = await browser.newPage();

  // 高解像度設定
  await page.setViewport({
    width: 1920,
    height: 1080,
    deviceScaleFactor: 2  // Retina対応
  });

  // プレゼンテーションファイルのパス
  const presentationPath = path.join(__dirname, 'cb_interpreter_presentation.html');
  const fileUrl = `file://${presentationPath}`;

  console.log(`Loading presentation from: ${fileUrl}`);
  await page.goto(fileUrl, {
    waitUntil: 'networkidle2',
    timeout: 30000
  });

  // reveal.jsが読み込まれるまで待機
  await page.waitForFunction(() => window.Reveal && window.Reveal.isReady());

  // 総スライド数を取得
  const slideCount = await page.evaluate(() => {
    const indices = Reveal.getIndices();
    Reveal.slide(0, 0);  // 最初のスライドに移動

    // すべてのスライド数をカウント
    let count = 0;
    const slides = Reveal.getSlides();
    return slides.length;
  });

  console.log(`Total slides: ${slideCount}`);

  // 各スライドをスクリーンショット
  for (let i = 0; i < slideCount; i++) {
    console.log(`Capturing slide ${i + 1}/${slideCount}...`);

    await page.evaluate((slideIndex) => {
      Reveal.slide(slideIndex, 0);
    }, i);

    // アニメーションの完了を待つ
    await page.waitForTimeout(500);

    // スクリーンショット
    const filename = path.join(outputDir, `slide_${String(i + 1).padStart(2, '0')}.png`);
    await page.screenshot({
      path: filename,
      fullPage: false,
      type: 'png'
    });

    console.log(`  Saved: ${filename}`);
  }

  await browser.close();

  console.log('\n✅ Export completed!');
  console.log(`📁 Images saved to: ${outputDir}`);
  console.log('\nNext steps:');
  console.log('1. Open Google Slides');
  console.log('2. Create a new presentation');
  console.log('3. For each slide:');
  console.log('   - Insert → Image → Upload from computer');
  console.log('   - Select the corresponding PNG file');
  console.log('   - Resize to fit the slide');
})();
