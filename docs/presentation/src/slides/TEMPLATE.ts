/**
 * Template for creating new slides
 *
 * Usage:
 * 1. Copy this file with a new name following the naming convention:
 *    - section{N}_{content}.ts for section content
 *    - {number}_{description}.ts for general slides
 *
 * 2. Modify the function name and content
 *
 * 3. Import and add to slide-loader.ts
 */

export default function templateSlide(): string {
    return `
        <section class="custom-slide-class">
            <h2>Slide Title</h2>

            <!-- Simple content -->
            <p>This is a paragraph</p>

            <!-- List -->
            <ul>
                <li>Item 1</li>
                <li>Item 2</li>
                <li>Item 3</li>
            </ul>

            <!-- Code block -->
            <pre><code class="language-typescript">
// TypeScript code example
interface Example {
    name: string;
    value: number;
}
            </code></pre>

            <!-- Two column layout -->
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 2em;">
                <div>
                    <h3>Left Column</h3>
                    <p>Content for left side</p>
                </div>
                <div>
                    <h3>Right Column</h3>
                    <p>Content for right side</p>
                </div>
            </div>

            <!-- Fragment (appears on click) -->
            <div class="fragment">
                <p>This appears on next click</p>
            </div>
        </section>
    `;
}

/**
 * Alternative: Multi-section slide (vertical slides)
 */
export function multiSectionSlide(): string {
    return `
        <section>
            <section>
                <h2>Main Slide</h2>
                <p>Press down for more details</p>
            </section>
            <section>
                <h3>Detail 1</h3>
                <p>First detail slide</p>
            </section>
            <section>
                <h3>Detail 2</h3>
                <p>Second detail slide</p>
            </section>
        </section>
    `;
}