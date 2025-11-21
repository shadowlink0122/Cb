import Reveal from 'reveal.js';
import RevealHighlight from 'reveal.js/plugin/highlight/highlight';
import RevealNotes from 'reveal.js/plugin/notes/notes';
import RevealMath from 'reveal.js/plugin/math/math';

// Import slides
import { loadSlides } from './slide-loader';
import { highlightCbCode } from './syntax-highlight';

// Initialize Reveal.js
function initPresentation() {
    // Load all slides
    const slidesContainer = document.getElementById('slides-container');
    if (!slidesContainer) {
        console.error('Slides container not found');
        return;
    }

    // Load slides synchronously before initializing Reveal
    loadSlides(slidesContainer);

    // Small delay to ensure DOM is fully rendered
    requestAnimationFrame(() => {
        // Initialize Reveal
        const deck = new Reveal({
            hash: true,
            controls: true,
            progress: true,
            center: false,  // Don't center vertically to allow scrolling
            transition: 'slide',
            transitionSpeed: 'default',
            backgroundTransition: 'fade',

            // Enable keyboard navigation and overview
            keyboard: true,
            overview: true,

            // Enable scrolling
            // scrollActivationWidth: null,  // This option may not exist in the current version
            // scrollProgress: true,  // This option may not exist in the current version

            // Disable features that might cause rendering issues
            preloadIframes: false,
            autoPlayMedia: false,

            // View settings
            width: 1280,
            height: 720,
            margin: 0.1,
            minScale: 0.2,
            maxScale: 2.0,

            // Plugins
            plugins: [RevealHighlight, RevealNotes, RevealMath]
        });

        deck.initialize().then(() => {
            // Presentation is ready
            console.log('Reveal.js initialized successfully');
            console.log('Total slides:', deck.getTotalSlides());
            console.log('Current slide:', deck.getCurrentSlide());

            // Apply Cb syntax highlighting
            highlightCbCode();

            // Debug: Log when overview mode is toggled
            deck.on('overviewshown', () => {
                console.log('Overview mode shown');
            });
            deck.on('overviewhidden', () => {
                console.log('Overview mode hidden');
            });
        });

        // Add keyboard shortcuts after initialization
        document.addEventListener('keydown', (event) => {
            if (event.key === 'f' && (event.metaKey || event.ctrlKey)) {
                event.preventDefault();
                // @ts-ignore - toggleFullscreen might not be in types
                if ((deck as any).toggleFullscreen) {
                    (deck as any).toggleFullscreen();
                }
            }
        });
    });
}

// Start the presentation when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initPresentation);
} else {
    // Use setTimeout to ensure all resources are loaded
    setTimeout(initPresentation, 0);
}