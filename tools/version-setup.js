const fs = require('fs');
const version = require('../version.json');

const resourceFile = "../aosora-shiori-dll/aosora-shiori-dll.rc";
const resourceCharset = "utf-16le";

const versionHeaderFile = "../aosora-shiori/Version.h";
const versionHeaderCharset = "utf-8";

const packageFile = "../vscode-extension/package.json";
const packageCharset = "utf-8";

//バージョンデータの読み込み
const aosora_ver = version["aosora-version"];
const vscode_extension_ver = version["vscode-extension-version"];
const debugger_ver = version["debugger-revision"];
let build_num = "0";
if(process.env.GITHUB_RUN_NUMBER){
    build_num = process.env.GITHUB_RUN_NUMBER;
}

const build_ver = `${aosora_ver}.${build_num}`;

//リソースのバージョン書き換え
let resourceFileBody = fs.readFileSync(resourceFile, resourceCharset);
resourceFileBody = resourceFileBody.replaceAll('0.0.0.1', build_ver);
resourceFileBody = resourceFileBody.replaceAll('0,0,0,1', build_ver.replaceAll('.', ','));
fs.writeFileSync(resourceFile, resourceFileBody, resourceCharset);

//Version.hの書き換え
let versionHeader = `#pragma once\r\n` + 
    `#define AOSORA_SHIORI_VERSION\t"${aosora_ver}"\r\n` + 
    `#define AOSORA_SHIORI_BUILD\t"Build#${build_num}"\r\n` +
    `#define AOSORA_DEBUGGER_REVISION\t"${debugger_ver}"\r\n`;
fs.writeFileSync(versionHeaderFile, versionHeader, versionHeaderCharset);

//vscode-extensionのpackage.json書き換え
let packageFileBody = fs.readFileSync(packageFile, packageCharset);
packageFileBody = packageFileBody.replace('0.0.1', vscode_extension_ver);
fs.writeFileSync(packageFile, packageFileBody, packageCharset);
