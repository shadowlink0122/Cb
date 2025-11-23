export default function includeModule(): string {
    return `<section class="include-module-slide">
        <h2>モジュールシステム - import</h2>

        <div class="code-section-large">
            <h3>📁 モジュールのインポート</h3>
            <pre><code class="language-cb">// モジュールのインポート
import math;
import utils;
import io;

// 特定の関数・型のみインポート
import { sin, cos, sqrt } from math;
import { HashMap, Vector } from collections;

// 相対パスでのインポート
import ./local_module;
import ../parent_module;

// 条件付きコンパイル
#ifdef DEBUG
    #define LOG(msg) println("[DEBUG] ", msg)
#else
    #define LOG(msg)  // 空のマクロ
#endif</code></pre>
        </div>

        <div class="feature-note">
            <p>📦 <strong>モダンなモジュールシステム</strong></p>
        </div>
    </section>`;
}