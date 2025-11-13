#!/usr/bin/env node

/**
 * VSCode拡張機能のバージョンがVERSIONファイルと一致しているか確認するスクリプト
 * CI/CDやビルド前のチェックに使用
 * 使用方法: node scripts/verify-version.js
 */

const fs = require('fs');
const path = require('path');

// パス設定
const rootDir = path.join(__dirname, '../..');
const versionFile = path.join(rootDir, '.cbversion');
const packageJsonPath = path.join(__dirname, '../package.json');

// .cbversionファイルを読み込み
if (!fs.existsSync(versionFile)) {
    console.error('❌ Error: .cbversion file not found at', versionFile);
    process.exit(1);
}

const expectedVersion = fs.readFileSync(versionFile, 'utf8').trim();

// package.jsonを読み込み
if (!fs.existsSync(packageJsonPath)) {
    console.error('❌ Error: package.json not found at', packageJsonPath);
    process.exit(1);
}

const packageJson = JSON.parse(fs.readFileSync(packageJsonPath, 'utf8'));
const actualVersion = packageJson.version;

// バージョンチェック
if (actualVersion !== expectedVersion) {
    console.error('❌ Version mismatch detected!');
    console.error('   .cbversion file:', expectedVersion);
    console.error('   package.json:', actualVersion);
    console.error('');
    console.error('💡 Fix by running: cd vscode-extension && npm run update-version');
    process.exit(1);
}

console.log('✅ Version check passed:', actualVersion);
console.log('   VSCode extension version matches .cbversion file');
