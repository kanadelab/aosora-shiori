import * as util from './utility';

//ja-jp
const MessageJaJp = {
	scriptPreview001: "トーク送信がすでに実行中です。しばらく待ってお試しください。",
	scriptPreview002: "スクリプトの読み込みに失敗しました。エラーを修正してから試してみてください。",
	scriptPreview003: "<< ゴーストに送信",
	scriptPreview004: "この関数を実行してゴーストに送信します。",
	scriptPreview005: "スクリプトが読み込み失敗する状態のため送信できません。",

	debugger001: "debug.debugger.runtime で設定されたパスにファイルが見つかりませんでした",
	debugger002: "ゴーストフォルダを検出できませんでした。",
	debugger003: "aosora プロジェクトフォルダを検出できませんでした。",
	debugger004: "すべてのエラー",
	debugger005: "実行時にエラーが発生したとき、キャッチされるかどうかにかかわらず実行中断します。",
	//debugger006: "asprojファイルで debug 設定が有効化されていません。",
	debugger007: "SSPを起動して接続しています...",
	debugger008: "デバッガはゴーストへの接続に失敗しました。",
	debugger009: "asprojファイルに debug.debugger.runtime 設定が見つかりません。",
	debugger010: "ワークスペースに ghost.asproj が見つからないため起動できませんでした。",
	debugger011: "ワークスペースに複数のghost.asprojがあるため起動に使用する1つを特定できませんでした。",
	debugger012: "起動中のゴーストにアタッチしています...",
	debugger013: "Aosora Debugger は Pause をサポートしていません。"
};
type Messages = typeof MessageJaJp;

//en-us
const MessageEnUs:Messages = {
	...MessageJaJp,     // fallback to japanese

	scriptPreview003: "<< send to Ghost",

	debugger004: "all errors"
};



//----------

export default function GetMessage():Messages{
	if(util.IsJapaneseLanguage()){
		return MessageJaJp;
	}
	else {
		return MessageEnUs;
	}
}

