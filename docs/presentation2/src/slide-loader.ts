// Slide imports - organized by section
import titleSlide from './slides/00_title';
import introSlide from './slides/01_intro';
import cbOverviewSlide from './slides/02_cb_overview';
import section1Cover from './slides/section1_cover';
import section2Cover from './slides/section2_cover';
import section3Cover from './slides/section3_cover';
import section4Cover from './slides/section4_cover';

// Define slide structure - these are synchronous functions
const slideModules = [
    titleSlide,
    introSlide,
    cbOverviewSlide,
    section1Cover,
    section2Cover,
    section3Cover,
    section4Cover
];

export function loadSlides(container: HTMLElement): void {
    // Build all HTML at once
    const allSlidesHtml = slideModules
        .map(slideModule => slideModule())
        .join('\n');

    // Set all slides at once to avoid reflow
    container.innerHTML = allSlidesHtml;
}

// Utility function for dynamic slide loading (for future use)
export async function loadSlideByName(slideName: string): Promise<string> {
    try {
        const module = await import(`./slides/${slideName}`);
        return module.default();
    } catch (error) {
        console.error(`Failed to load slide: ${slideName}`, error);
        return '<section><h2>Slide not found</h2></section>';
    }
}