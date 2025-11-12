// スライドを動的に読み込む
const slideFiles = [
    'sections/slide_001.html',
    'sections/slide_002.html',
    'sections/slide_003.html',
    'sections/slide_004.html',
    'sections/slide_005.html',
    'sections/slide_006.html',
    'sections/slide_007.html',
    'sections/slide_008.html',
    'sections/slide_009.html',
    'sections/slide_010.html',
    'sections/slide_011.html',
    'sections/slide_012.html',
    'sections/slide_013.html',
    'sections/slide_014.html',
    'sections/slide_015.html',
    'sections/slide_016.html',
    'sections/slide_017.html',
    'sections/slide_018.html',
    'sections/slide_019.html',
    'sections/slide_020.html',
    'sections/slide_021.html',
    'sections/slide_022.html',
    'sections/slide_023.html',
    'sections/slide_024.html',
    'sections/slide_025.html',
    'sections/slide_026.html',
    'sections/slide_027.html',
    'sections/slide_028.html',
    'sections/slide_029.html',
    'sections/slide_030.html',
    'sections/slide_031.html',
    'sections/slide_032.html',
    'sections/slide_033.html',
    'sections/slide_034.html',
    'sections/slide_035.html',
    'sections/slide_036.html',
    'sections/slide_037.html',
    'sections/slide_038.html',
    'sections/slide_039.html',
    'sections/slide_040.html',
    'sections/slide_041.html',
    'sections/slide_042.html',
    'sections/slide_043.html',
    'sections/slide_044.html',
    'sections/slide_045.html',
    'sections/slide_046.html',
    'sections/slide_047.html',
    'sections/slide_048.html',
    'sections/slide_049.html',
    'sections/slide_050.html',
    'sections/slide_051.html',
    'sections/slide_052.html',
    'sections/slide_053.html',
    'sections/slide_054.html',
    'sections/slide_055.html',
];

async function loadSlides() {
    const container = document.getElementById('slides-container');
    
    for (const file of slideFiles) {
        try {
            const response = await fetch(file);
            const html = await response.text();
            container.innerHTML += html;
        } catch (error) {
            console.error(`Failed to load ${file}:`, error);
        }
    }
    
    // スライド読み込み完了後にReveal.jsを初期化
    Reveal.sync();
}

// ページ読み込み時にスライドを読み込む
document.addEventListener('DOMContentLoaded', loadSlides);
