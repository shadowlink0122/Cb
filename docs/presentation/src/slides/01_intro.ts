export default function introSlide(): string {
    return `<section class="intro-slide">
        <h1><br></h1>
        <h2>自己紹介</h2>
        <div class="profile-container">
            <div class="profile-image">
                <img src="/assets/profile.jpg" alt="Profile" />
            </div>
            <div class="profile-details">
                <h3>miyajima</h3>
                <ul class="social-links">
                    <li><strong>Job:</strong> Software Developer</li>
                    <li><strong>Twitter:</strong> @sl_0122</li>
                    <li><strong>GitHub:</strong> @shadowlink0122</li>
                </ul>
            </div>
            <div class="profile-details">
                <h3>趣味</h3>
                <ul class="social-links">
                    <li>知識０からコンパイラ開発</li>
                    <li>ゲーム アニメ</li>
                    <li>家系ラーメン</li>
                </ul>
            </div>
        </div>

        <div style="text-align: center; margin-top: 2em; padding: 1em; background: #f8f9fa; border-radius: 8px;">
            <p style="font-size: 0.9em; margin: 0;">
                💬 <strong>開発状況を垂れ流し中：</strong>
                <a href="https://osdev.jp/" target="_blank" style="color: #3498db; text-decoration: none; font-weight: bold;">osdev-jp</a>
            </p>
        </div>
    </section>`;
}