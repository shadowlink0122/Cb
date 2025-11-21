// Slide imports - organized by section
import titleSlide from './slides/00_title';
import introSlide from './slides/01_intro';
import cbOverviewSlide from './slides/02_cb_overview';
import cbVision from './slides/02b_cb_vision';
import currentFocus from './slides/02c_current_focus';
import hirArchitecture from './slides/02d_hir_architecture';
import techStack from './slides/03_tech_stack';
import architectureDiagram from './slides/03b_architecture';
import parserAndTest from './slides/04_parser_test';
import roadmap from './slides/05_roadmap';
import docsAndRelease from './slides/06_docs_release';
import section1Cover from './slides/section1_cover';
import languageFeatures from './slides/07_language_features';

// Basic syntax split slides
import basicTypes from './slides/10a_basic_types';
import constPointers from './slides/10c_const_pointers';
import controlFlow from './slides/10b_control_flow';
import switchStatement from './slides/10d_switch_statement';

// Type system split slides
import unionTypes from './slides/11a_union_types';
import literalTypes from './slides/11b_literal_types';
import structLiterals from './slides/11c_struct_literals';

// Interface & Impl split slides
import interfaceDefinition from './slides/12a_interface_definition';
import implBlocks from './slides/12b_impl_blocks';
import polymorphism from './slides/12c_polymorphism';

// Async/Await split slides
import asyncBasics from './slides/13a_async_basics';
import eventLoop from './slides/13b_event_loop';
import errorPropagation from './slides/13c_error_propagation';

// Preprocessor split slides
import includeModule from './slides/14a_include_module';
import basicMacros from './slides/14b_basic_macros';

// Advanced features split slides
import enumType from './slides/15a_enum';
import functionPointer from './slides/15b_function_pointer';
import memoryManagement from './slides/15c_memory_management';
import patternMatching from './slides/15d_pattern_matching';
import generics from './slides/15e_generics';
import ffi from './slides/15f_ffi';

// Error handling split slides
import optionType from './slides/16a_option_type';
import resultType from './slides/16b_result_type';

// Section 2 - AI Development
import aiDevelopmentIntro from './slides/20_ai_development_intro';
import aiBenefits from './slides/21_ai_benefits';
import humanRole from './slides/22_human_role';
import aiChallenges from './slides/23_ai_challenges';
import documentationStrategy from './slides/24_documentation_strategy';
import documentationPractice from './slides/24a_documentation_practice';
import refactoringNecessity from './slides/25_refactoring_necessity';
import toolsOverview from './slides/26_tools_overview';
import testingIntro from './slides/27a_testing_intro';
import testingExamples from './slides/27b_testing_examples';
import debugModeIntro from './slides/28a_debug_mode_intro';
import debugModeExamples from './slides/28b_debug_mode_examples';
import sanitizerIntro from './slides/29a_sanitizer_intro';
import sanitizerExamples from './slides/29b_sanitizer_examples';
import refactoringStory from './slides/30_refactoring_story';
import section2Summary from './slides/31_section2_summary';

import section2Cover from './slides/section2_cover';
import section3Cover from './slides/section3_cover';
import section4Cover from './slides/section4_cover';

// Define slide structure - these are synchronous functions
const slideModules = [
    titleSlide,
    introSlide,
    cbOverviewSlide,
    languageFeatures,
    cbVision,
    techStack,
    architectureDiagram,      // 実行アーキテクチャ
    hirArchitecture,          // HIR（実行アーキテクチャの次）
    currentFocus,             // FFI（HIRの次）
    parserAndTest,
    roadmap,
    docsAndRelease,
    section1Cover,            // Section 1: 実装した機能
    // 基本構文 (分割版)
    basicTypes,               // 基本型
    constPointers,            // const修飾子とポインタ
    controlFlow,              // 制御構文
    switchStatement,          // switch文の詳細
    // 型システム (分割版)
    unionTypes,               // ユニオン型
    literalTypes,             // リテラル型
    structLiterals,           // 構造体リテラル
    // Interface & Impl (分割版)
    interfaceDefinition,      // Interface定義
    implBlocks,               // Implブロック
    polymorphism,             // ポリモーフィズム
    // Async/Await (分割版)
    asyncBasics,              // 非同期の基本
    eventLoop,                // イベントループ
    errorPropagation,         // エラー伝播演算子
    // プリプロセッサ (分割版)
    includeModule,            // モジュールシステム
    basicMacros,              // 基本的なマクロ
    // 高度な機能 (分割版)
    enumType,                 // Enum型
    functionPointer,          // 関数ポインタ
    memoryManagement,         // メモリ管理
    patternMatching,          // パターンマッチング
    generics,                 // ジェネリクス
    ffi,                      // FFI
    // エラーハンドリング (分割版)
    optionType,               // Option型
    resultType,               // Result型
    // Section 2: AIによるバイブコーディング
    section2Cover,            // Section 2 表紙
    aiDevelopmentIntro,       // AI開発の実態
    aiBenefits,               // AIのメリット
    humanRole,                // 人間の役割
    aiChallenges,             // AI開発の課題
    documentationStrategy,    // ドキュメント戦略
    documentationPractice,    // 実際のドキュメント管理
    refactoringNecessity,     // リファクタリングの必要性
    toolsOverview,            // 3つの武器
    testingIntro,             // テスト入門
    testingExamples,          // テスト実例
    debugModeIntro,           // デバッグモード入門
    debugModeExamples,        // デバッグモード実例
    sanitizerIntro,           // サニタイザー入門
    sanitizerExamples,        // サニタイザー実例
    refactoringStory,         // リファクタリング実例
    section2Summary,          // セクション2まとめ
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