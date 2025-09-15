import * as vscode from 'vscode';
import {Analyze, AnalyzedSourceRange, AnalyzeResult} from './scriptAnalyzer';
import {IsBinaryExecutablePlatform} from './utility';
import GetMessage from './messages';
import {SendPreviewFunction, SendScript} from './scriptPreview';

const functionPattern = /(function|talk)/g;

export class DebugSendProvider implements vscode.CodeLensProvider{

	private extensionPath:string;
	private diag:vscode.DiagnosticCollection;
	private lastSuccessAnalyzeResult:AnalyzeResult|null;
	private isAnalyzeSuccess:boolean;

	constructor(extensionPath:string, diag: vscode.DiagnosticCollection){
		this.extensionPath = extensionPath;
		this.diag = diag;
		this.lastSuccessAnalyzeResult = null;
		this.isAnalyzeSuccess = false;
	}

	private MakeSourceRange(range:AnalyzedSourceRange):vscode.Range{
		return new vscode.Range(range.line, range.column, range.endLine, range.endColumn);
	}

	public async SendToGhost(scriptBody:SendScript, isError:boolean){
		if(!isError){
			if(scriptBody){
				await SendPreviewFunction(scriptBody, this.extensionPath);
			}
		}
		else{
			vscode.window.showErrorMessage(GetMessage().scriptPreview002);
		}
	}

	public async SendFromMenu() {

		if(!this.isAnalyzeSuccess){
			//送信不可
			return ;
		}

		const editor = vscode.window.activeTextEditor;
		if (!editor) {
			return;
		}
		const position = editor.selection.active;

		if(this.lastSuccessAnalyzeResult && this.lastSuccessAnalyzeResult.functions){

			//関数群の位置を検出しておくる
			for(const func of this.lastSuccessAnalyzeResult.functions){
				//カレットが位置に含むかをチェック
				const r = this.MakeSourceRange(func.range);
				if(r.contains(position)){
					//送信対象
					const script = editor.document.getText(r);
					this.SendToGhost({
						scriptBody: script,
						unit: this.lastSuccessAnalyzeResult.unit!,
						uses: this.lastSuccessAnalyzeResult.uses!
					}, false);
					return;
				}
			}
		}
	}
	
	public async provideCodeLenses(document: vscode.TextDocument, token: vscode.CancellationToken): Promise<vscode.CodeLens[]> {
		
		//aosora-analyzerを起動
		const analyzeResult = await Analyze(document, this.extensionPath);
		if(!analyzeResult.error){
			//エラーなし
			this.diag.set(document.uri, []);
			this.lastSuccessAnalyzeResult = analyzeResult;
			this.isAnalyzeSuccess = true;
		}
		else {
			this.isAnalyzeSuccess = false;

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
					const sendScript:SendScript = {
						scriptBody: script,
						unit: this.lastSuccessAnalyzeResult.unit!,
						uses: this.lastSuccessAnalyzeResult.uses!
					};

					codeLens.command = {
						title: GetMessage().scriptPreview003,
						command: "aosora-shiori.sendToGhost",
						tooltip: !analyzeResult.error ? GetMessage().scriptPreview004 : GetMessage().scriptPreview005,
						arguments: [sendScript, analyzeResult.error]                        
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