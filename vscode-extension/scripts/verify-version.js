#!/usr/bin/env node

/**
 * VSCode拡張機能のバージョンがcb_config.jsonと一致しているか確認するスクリプト
 * CI/CDやビルド前のチェックに使用
 * 使用方法: node scripts/verify-version.js
 */

const fs = require('fs');
const path = require('path');

// パス設定
const rootDir = path.join(__dirname, '../..');
const configFile = path.join(rootDir, 'cb_config.json');
const packageJsonPath = path.join(__dirname, '../package.json');

// cb_config.jsonファイルを読み込み
if (!fs.existsSync(configFile)) {
    console.error('❌ Error: cb_config.json file not found at', configFile);
    process.exit(1);
}

const config = JSON.parse(fs.readFileSync(configFile, 'utf8'));
const expectedVersion = config.version;

if (!expectedVersion) {
    console.error('❌ Error: version field not found in cb_config.json');
    process.exit(1);
}

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
    console.error('   cb_config.json:', expectedVersion);
    console.error('   package.json:', actualVersion);
    console.error('');
    console.error('💡 Fix by running: cd vscode-extension && npm run update-version');
    process.exit(1);
}

console.log('✅ Version check passed:', actualVersion);
console.log('   VSCode extension version matches cb_config.json');

