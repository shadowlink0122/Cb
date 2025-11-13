#!/usr/bin/env node

/**
 * VSCode拡張機能のバージョンをルートのVERSIONファイルから自動更新するスクリプト
 * 使用方法: node scripts/update-version.js
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

const version = fs.readFileSync(versionFile, 'utf8').trim();

// バージョンフォーマットチェック (x.y.z)
if (!/^\d+\.\d+\.\d+$/.test(version)) {
    console.error('❌ Error: Invalid version format in .cbversion file:', version);
    console.error('   Expected format: x.y.z (e.g., 0.13.0)');
    process.exit(1);
}

// package.jsonを読み込み
if (!fs.existsSync(packageJsonPath)) {
    console.error('❌ Error: package.json not found at', packageJsonPath);
    process.exit(1);
}

const packageJson = JSON.parse(fs.readFileSync(packageJsonPath, 'utf8'));
const oldVersion = packageJson.version;

// バージョンが既に同じ場合はスキップ
if (oldVersion === version) {
    console.log('✅ Version is already up to date:', version);
    process.exit(0);
}

// バージョンを更新
packageJson.version = version;

// package.jsonに書き戻し（フォーマット保持）
fs.writeFileSync(
    packageJsonPath,
    JSON.stringify(packageJson, null, 2) + '\n',
    'utf8'
);

console.log('✅ Version updated successfully:');
console.log('   Old version:', oldVersion);
console.log('   New version:', version);
console.log('   File:', packageJsonPath);
