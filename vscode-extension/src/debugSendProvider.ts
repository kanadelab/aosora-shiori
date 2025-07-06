import * as vscode from 'vscode';
import {Analyze, AnalyzedSourceRange, AnalyzeResult} from './scriptAnalyzer';
import {IsBinaryExecutablePlatform} from './utility';
import GetMessage from './messages';

const functionPattern = /(function|talk)/g;

export class DebugSendProvider implements vscode.CodeLensProvider{

	private extensionPath:string;
	private diag:vscode.DiagnosticCollection;
	private lastSuccessAnalyzeResult:AnalyzeResult|null;

	constructor(extensionPath:string, diag: vscode.DiagnosticCollection){
		this.extensionPath = extensionPath;
		this.diag = diag;
		this.lastSuccessAnalyzeResult = null;
	}

	private MakeSourceRange(range:AnalyzedSourceRange):vscode.Range{
		return new vscode.Range(range.line, range.column, range.endLine, range.endColumn);
	}
	
	async provideCodeLenses(document: vscode.TextDocument, token: vscode.CancellationToken): Promise<vscode.CodeLens[]> {
		
		//aosora-analyzerを起動
		const analyzeResult = await Analyze(document, this.extensionPath);
		if(!analyzeResult.error){
			//エラーなし
			this.diag.set(document.uri, []);
			this.lastSuccessAnalyzeResult = analyzeResult;
		}
		else {

			//認識できるエラーを吐いている場合、構文エラーなのでエディタに指摘内容を反映する
			if(analyzeResult.message && analyzeResult.range){
				const range = this.MakeSourceRange(analyzeResult.range);
				if(range){
				this.diag.set(document.uri, [
						{
							message: analyzeResult.message,
							range: range,
							severity: vscode.DiagnosticSeverity.Error
						}
					]);
				}
			}
			else {
				//認識できないエラー：仕方ないので無視
				this.diag.set(document.uri, []);
			}
		}

		//最後に解析成功した情報を使用してCodeLendsを作る
		//失敗時無効化していると、スクリプト記述中に表示ががたがたしてしまうため
		if(this.lastSuccessAnalyzeResult && this.lastSuccessAnalyzeResult.functions){
			const result:vscode.CodeLens[] = [];
			for(const item of this.lastSuccessAnalyzeResult.functions){
				const scriptRange = this.MakeSourceRange(item.range);
				const script = document.getText(scriptRange);

				//function または talk の部分があれば採用する
				//解析エラー時、関数行のCodelends表示を維持する一方、どうみても関数ではないところに送信表示を出さないようにするため
				const keyWordRange = document.getWordRangeAtPosition(new vscode.Position(item.range.line, item.range.column), functionPattern);;
				if(keyWordRange){
					const codeLens = new vscode.CodeLens(keyWordRange);
					codeLens.command = {
						title: GetMessage().scriptPreview003,
						command: "aosora-shiori.sendToGhost",
						tooltip: !analyzeResult.error ? GetMessage().scriptPreview004 : GetMessage().scriptPreview005,
						arguments: [script, analyzeResult.error]                        
					};
					result.push(codeLens);
				}
			}

			return result;
		}
		else {
			return [];
		}
	}

}