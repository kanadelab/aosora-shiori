
//.exeバイナリを実行可能なプラットフォームかどうかを確認
export function IsBinaryExecutablePlatform() :boolean{
    return process.platform === 'win32';
}

//言語設定が日本語かどうかを検出
//vscodeは設定しづらいのでプライマリを英語にして国外ユーザには英語が常に表示されるようにしておく
export function IsJapaneseLanguage():boolean{
    const lang = (process.env.VSCODE_NLS_CONFIG && JSON.parse(process.env.VSCODE_NLS_CONFIG).locale) || '';
    return lang.startsWith('ja');
}