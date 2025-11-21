export default function hirArchitecture(): string {
    return `<section class="hir-architecture-slide">
        <h2>HIRベースのマルチターゲット戦略</h2>

        <div class="hir-container">
            <div class="hir-diagram">
                <div class="source-level">
                    <div class="cb-source">Cbソースコード</div>
                    <div class="arrow-down">→</div>
                    <div class="ast-box">AST</div>
                </div>
                <div class="arrow-down-large">
                    <div class="arrow-down">↓ 変換</div>
                </div>
                <div class="hir-level">
                    <div class="hir-hub">
                        <strong>HIR</strong>
                        <span class="hub-subtitle">High-level Intermediate Representation</span>
                        <span class="hub-description">中間表現</span>
                    </div>
                </div>

                <div class="target-arrows">
                    <span>↙</span>
                    <span>↙</span>
                    <span>↓</span>
                    <span>↘</span>
                    <span>↘</span>
                </div>

                <div class="target-level">
                    <div class="target-box cpp">
                        <strong>C++</strong>
                        <span class="status active">実装中</span>
                        <small>ネイティブ実行</small>
                    </div>
                    <div class="target-box wasm">
                        <strong>WASM</strong>
                        <span class="status planned">計画中</span>
                        <small>ブラウザ実行</small>
                    </div>
                    <div class="target-box typescript">
                        <strong>TypeScript</strong>
                        <span class="status planned">計画中</span>
                        <small>Node.js/Deno</small>
                    </div>
                    <div class="target-box llvm">
                        <strong>LLVM IR</strong>
                        <span class="status future">構想</span>
                        <small>最適化</small>
                    </div>
                    <div class="target-box gpu">
                        <strong>GPU</strong>
                        <span class="status future">構想</span>
                        <small>CUDA/Metal</small>
                    </div>
                </div>
            </div>

            <div class="hir-features">
                <h3>🎯 HIRの役割</h3>
                <div class="feature-grid">
                    <div class="hir-feature">
                        <strong>統一された中間表現</strong>
                        <p>すべてのターゲットが共通のHIRから生成</p>
                    </div>
                    <div class="hir-feature">
                        <strong>最適化の一元化</strong>
                        <p>HIRレベルで共通の最適化を適用</p>
                    </div>
                    <div class="hir-feature">
                        <strong>新ターゲット追加が容易</strong>
                        <p>HIR→新言語の変換器を追加するだけ</p>
                    </div>
                    <div class="hir-feature">
                        <strong>クロスプラットフォーム</strong>
                        <p>1つのコードから複数環境向けにビルド</p>
                    </div>
                </div>
            </div>
        </div>
    </section>`;
}