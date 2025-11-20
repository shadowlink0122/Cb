export default function currentFocus(): string {
    return `<section class="current-focus-slide">
        <h2>FFI - 多言語連携機能</h2>

        <div class="ffi-container">
            <div class="ffi-main">
                <h3>🔗 Foreign Function Interface</h3>
                <p class="ffi-description">
                    既存の資産を活用しながら、段階的にCbへ移行可能
                </p>

                <div class="ffi-flow-large">
                    <div class="source-langs">
                        <div class="lang-card">
                            <span class="lang-icon">C/C++</span>
                            <small>システム<br/>ライブラリ</small>
                        </div>
                        <div class="lang-card">
                            <span class="lang-icon rust">Rust</span>
                            <small>高速処理<br/>モジュール</small>
                        </div>
                        <div class="lang-card">
                            <span class="lang-icon go">Go</span>
                            <small>ネットワーク<br/>ライブラリ</small>
                        </div>
                    </div>

                    <div class="compile-arrow">→</div>

                    <div class="object-files">
                        <div class="obj-type">.o</div>
                        <div class="obj-type">.a</div>
                        <div class="obj-type">.so</div>
                        <small>オブジェクトファイル</small>
                    </div>

                    <div class="import-arrow">→</div>

                    <div class="cb-usage">
                        <div class="cb-code">
                            <code>
// Cb側での利用<br/>
import "libmath.a";<br/>
import "network.so";<br/>
<br/>
void main() {<br/>
&nbsp;// 既存関数を呼び出し<br/>
&nbsp;let result=calc(10,20);<br/>
}
                            </code>
                        </div>
                    </div>
                </div>
            </div>

            <div class="ffi-benefits">
                <h3>📊 FFIのメリット</h3>
                <div class="benefit-list">
                    <div class="benefit-item">
                        <strong>既存資産の活用</strong>
                        <p>長年開発されたライブラリをそのまま利用</p>
                    </div>
                    <div class="benefit-item">
                        <strong>段階的な移行</strong>
                        <p>必要な部分から徐々にCbで書き換え</p>
                    </div>
                    <div class="benefit-item">
                        <strong>言語の良いとこ取り</strong>
                        <p>各言語の得意分野を組み合わせて利用</p>
                    </div>
                    <div class="benefit-item">
                        <strong>リスクの最小化</strong>
                        <p>実績あるコードを維持しつつ新機能追加</p>
                    </div>
                </div>
            </div>
        </div>
    </section>`;
}