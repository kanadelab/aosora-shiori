import * as vscode from 'vscode';

// function A,B (a,b) ここまで。次にif expressionが来るのでここは別でチェックが必要そう
// 色々同じように終了コードをみつけないとなのかも…
const PatternFunctionBlockBegin = /\bfunction(\s*|\b)(\w|,|\s)*(\s*|\b)(\((\w|,|\s)*\))*/g;
const PatternFunctionBlockEnd = /\}/g;
const PatternTTalkBlockBegin = /\btalk(\s*|\b)(\w|,|\s)*(\s*|\b)(\((\w|,|\s)*\))*/g;
const PatternTalkBlockEnd = /\}/g;

const PatternStringBegin = /"/g;
const PatternStringEnd = /"/g;		//TODO: エスケープ対応、改行末尾？

const PatternFormatBegin = /\$\{/g;
const PatternFormatEnd = /\}/g;

const PatternFuncScopeBegin = /\%\{/g;
const PatternFuncScopeEnd = /\}/g;

const PatternObjectScopeBegin = /\{/g;
const PatternObjectScopeEnd = /\}/g;


//関数if式の開始パターン
const PatternIfExpressionBegin = /^(\s*|\b)if(\s*|\b)\(/g;
const PatternIfExpressionEnd = /\)/g;

const PatternBlockBegin = /(\s*|\b)\{/g;

export const SCRIPT_BLOCK_TYPE_FUNCTION = 0;
export const SCRIPT_BLOCK_TYPE_TALK = 1;
export const SCRIPT_BLOCK_TYPE_STRING = 2;
export const SCRIPT_BLOCK_TYPE_OBJECT_INITIALIZER = 3;

//スクリプトブロック
export class ScriptBlock {
	public readonly blockType:number;
	public readonly beginIndex:number;
	public readonly children:ScriptBlock[];

	constructor(blockType:number, beginIndex:number, blocks: ScriptBlock[] = []){
		this.blockType = blockType;
		this.beginIndex = beginIndex;
		this.children = blocks;
	}

	//送信スクリプトボディ
	public sendScriptBody:string|null=null;
}

class ScriptAnalyzeContext {

	private patternCache:{[key:string]:RegExpMatchArray|null} = {};
	private document:vscode.TextDocument;
	private currentIndex:number = 0;

	constructor(document:vscode.TextDocument){
		this.document = document;
	}

	//ドキュメント取得
	public GetDocument() {
		return this.document;
	}

	//現在のインデックス取得
	public GetCurrentIndex() {
		return this.currentIndex;
	}

	//マッチ領域ぶんオフセットする
	public SeekIndex(match:RegExpMatchArray){
		this.currentIndex = match.index! + match.length;
	}

	//単純マッチ
	public Match(pattern:RegExp){
		//TODO: キャッシュにつっこめるといいかも？
		pattern.lastIndex = this.currentIndex;
		return pattern.exec(this.document.getText());
	}

	//正規表現を複数検証し、最初に見つかったインデックスを返す
	public Query(patterns:RegExp[]):{itemIndex:number, matchResult:RegExpMatchArray|null}{

		//最小のマッチを探す
		let minMatchTextIndex = this.document.getText().length;
		let minMatchItemIndex = patterns.length;
		let minMatchItem:RegExpMatchArray|null = null;

		for(let i = 0; i < patterns.length; i++){
			const pattern = patterns[i];

			let isNotMatch = false;
			let match:RegExpMatchArray|null = null;

			if(pattern.source in this.patternCache){

				//結果をキャッシュしてる場合はそれを使う
				const cache = this.patternCache[pattern.source];

				if(cache && cache.index !== undefined){
					//開始位置が決まってる場合
					if(this.currentIndex <= cache.index){
						match = cache;
					}
				}
				else {
					//マッチしてないので今後も探しても無駄
					isNotMatch = true;
				}
			}

			//キャッシュが切れてるので検索
			if(match == null && !isNotMatch){
				//検索開始位置を制御
				pattern.lastIndex = this.currentIndex;
				var matchResult = pattern.exec(this.document.getText());

				//検索結果を記録
				match = matchResult;
				this.patternCache[pattern.source] = matchResult;
			}

			//比較
			if(match && match.index !== undefined){
				if(match.index < minMatchTextIndex){
					minMatchItem = match;
					minMatchTextIndex = match.index;
					minMatchItemIndex = i;
				}
			}
		}

		if(minMatchItemIndex < patterns.length){
			//見つかった
			return {itemIndex: minMatchItemIndex, matchResult: minMatchItem};
		}
		else {
			//見つからなかった
			return {itemIndex: -1,matchResult: null};
		}
	}
}

type MatchProcess = [
	RegExp, (match:RegExpMatchArray, index:number)=>boolean
];

export class ScriptAnalyzer {



	public Analyze(document:vscode.TextDocument){

		const analyzeContext = new ScriptAnalyzeContext(document);
		return this.AnalyzeFunctionBlock(analyzeContext);

	}

	//functionかtalkのパターンにマッチした
	public AnalyzeFunctionStatement(analyzeContext: ScriptAnalyzeContext, index:number):ScriptBlock|null{
		//if式の確認
		const beginIndex = index;

		const ifParseResult = analyzeContext.Match(PatternIfExpressionBegin);
		if(ifParseResult){
			analyzeContext.SeekIndex(ifParseResult);
			this.AnalyzeFunctionBlock(analyzeContext, PatternIfExpressionEnd);
		}

		//開カッコ
		const beginBlockResult = analyzeContext.Match(PatternBlockBegin);
		if(beginBlockResult){
			analyzeContext.SeekIndex(beginBlockResult);
			const children = this.AnalyzeFunctionBlock(analyzeContext, PatternFunctionBlockEnd);

			//アイテム追加
			const item = new ScriptBlock(SCRIPT_BLOCK_TYPE_FUNCTION, index, children);

			//ここまで関数ブロックとして本文にとりこむ
			const doc = analyzeContext.GetDocument();
			const endIndex = analyzeContext.GetDocument().getText(new vscode.Range(doc.positionAt(beginIndex), doc.positionAt(analyzeContext.GetCurrentIndex())));
			item.sendScriptBody = endIndex;
			return item;
		}
		else {
			//打ち切るべきか？
		}
		return null;
	}

	public AnalyzeFunctionBlock(analyzeContext:ScriptAnalyzeContext, sequenceEndPattern?:RegExp) :ScriptBlock[] {

		const result:ScriptBlock[] = [];

		let objectNestCount = 0;

		//次にマッチしたものに対しての処理の羅列
		const patterns:MatchProcess[] = [

			//関数ブロック開始
			[
				PatternFunctionBlockBegin,
				(match, index) => {
					analyzeContext.SeekIndex(match);
					const item = this.AnalyzeFunctionStatement(analyzeContext, index);
					if(item){
						result.push(item);
					}
					return false;
				}
			],
			
			//トークブロック開始
			[
				PatternTTalkBlockBegin,
				(match, index) => {
					analyzeContext.SeekIndex(match);
					const item = this.AnalyzeFunctionStatement(analyzeContext, index);
					if(item){
						result.push(item);
					}
					return false;
				}
			],

			//文字列開始
			[
				PatternStringBegin,
				(match, index) => {
					analyzeContext.SeekIndex(match);

					//内部を検索
					const children = this.AnalyzeStringBlock(analyzeContext, PatternStringEnd);
					const item = new ScriptBlock(SCRIPT_BLOCK_TYPE_STRING, index, children);
					result.push(item);
					return false;
				}
			],

			//（汎用）オブジェクトブロック開始
			[
				PatternObjectScopeBegin,
				(match, index) => {
					//ネストの管理にだけ使用するので無視
					analyzeContext.SeekIndex(match);

					objectNestCount ++;
					return false;
				}
			]
		];

		//終端処理
		if(sequenceEndPattern){
			patterns.push([sequenceEndPattern, (match) => {
				analyzeContext.SeekIndex(match);

				//ちょっと微妙だけどオブジェクトネストを検査
				if(sequenceEndPattern.source == PatternObjectScopeEnd.source){
					if(objectNestCount > 0){
						objectNestCount --;
						return false;	//ネストの戻りなので続行
					}
				}

				return true;	//返す
			}]);
		}

		while(true){
			//検索、最初にヒットしたものの処理を行う
			const matchResult = analyzeContext.Query(patterns.map(o => o[0]));
			if(matchResult.matchResult){
				const funcResult = patterns[matchResult.itemIndex][1](matchResult.matchResult, matchResult.matchResult.index!);
				if(funcResult){
					break;
				}
			}
			else {
				//いずれもマッチしなければ終了
				break;
			}
		}

		return result;
	}

	public AnalyzeStringBlock(analyzeContext:ScriptAnalyzeContext, sequenceEndPattern?:RegExp) :ScriptBlock[]{
		const result:ScriptBlock[] = [];

		//ここでは ${}, %{} のマッチのみが確認対象となる
		const patterns:MatchProcess[] = [
			[
				PatternFormatBegin,
				(match, index) => {
					analyzeContext.SeekIndex(match);
					const children = this.AnalyzeFunctionBlock(analyzeContext, PatternFormatEnd);
					const item = new ScriptBlock(SCRIPT_BLOCK_TYPE_FUNCTION, index, children);
					result.push(item);
					return false;
				}
			],
			[
				PatternFuncScopeBegin,
				(match, index) => {
					analyzeContext.SeekIndex(match);
					const children = this.AnalyzeFunctionBlock(analyzeContext, PatternFormatEnd);
					const item = new ScriptBlock(SCRIPT_BLOCK_TYPE_FUNCTION, index, children);
					result.push(item);
					return false;
				}
			]
		];

		if(sequenceEndPattern){
			patterns.push([sequenceEndPattern, (match) => {
				analyzeContext.SeekIndex(match);
				return true;
			}]);
		}

		while(true){
			//検索、最初にヒットしたものの処理を行う
			const matchResult = analyzeContext.Query(patterns.map(o => o[0]));
			if(matchResult.matchResult){
				const funcResult = patterns[matchResult.itemIndex][1](matchResult.matchResult, matchResult.matchResult.index!);
				if(funcResult){
					break ;
				}
			}
			else {
				//いずれもマッチしなくなったら終了
				break;
			}
		}

		return result;
	}

}