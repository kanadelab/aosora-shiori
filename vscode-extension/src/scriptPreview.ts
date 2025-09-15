import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import * as childProcess from 'child_process';
import * as iconv from 'iconv-lite';
import { MessageOptions } from 'vscode';
import GetMessage from './messages';
import { IsBinaryExecutablePlatform } from './utility';

const ERROR_CODE_INVALID_ARGS = 1;
const ERROR_CODE_GHOST_NOT_FOUND = 2;
const ERROR_CODE_GHOST_SCRIPT_ERROR = 3;
const ERROR_CODE_PREVIEW_SCRIPT_ERROR = 4;

let isExecuting = false;

//unitやuseが追加されたので、ゴーストに送信する場合これらの情報が必要になる。
export type SendScript = {
	scriptBody: string,
	unit: string,
	uses: string[]
};


//スクリプトプレビューイング
export async function SendPreviewFunction(sendScript:SendScript, extensionPath:string){
	if(isExecuting){
		vscode.window.showErrorMessage(GetMessage().scriptPreview001);
	}

	isExecuting = true;
	try{
		const outPath = extensionPath + "/" + '_aosora_send_script_.as';
		const executablePath = ((IsBinaryExecutablePlatform()) ? (extensionPath + "/" + "aosora-sstp.exe") : ("aosora-sstp.sh"));
		let command = `"${executablePath}" "${outPath}"`;

		//ワークスペースがあればパスに足す
		const projFiles = await vscode.workspace.findFiles("**/ghost.asproj", null, 1);
		if(projFiles.length > 0){
			const workspace = path.dirname(projFiles[0].fsPath);
			if (IsBinaryExecutablePlatform()) {
				command += ` ${workspace}\\\\`;
			}
			else {
				command += ` "${workspace}/"`;
			}
		}

		let functionBody = "";
		functionBody += `unit ${sendScript.unit};\r\n`;
		functionBody += sendScript.uses.map(o => `use ${o};`).join("\r\n");
		functionBody += "\r\n";
		functionBody += sendScript.scriptBody;

		//一時ファイルを用意して呼び出す
		await fs.promises.writeFile(outPath, functionBody, 'utf-8');

		//実行待ち
		await new Promise<void>(r  => {
			childProcess.exec(command, (error, stdout, stderr) => {
				if(error){
					vscode.window.showErrorMessage(ExitCodeToString(error.code) + stderr);
				}
				r();
			});
		});

		await fs.promises.rm(outPath);
	}
	catch{}
	finally{
		isExecuting = false;
	}
}

function ExitCodeToString(code?:number){
	if(code == ERROR_CODE_GHOST_NOT_FOUND) {
		return "スクリプト送信先のゴーストが見つかりませんでした。";
	}
	else if(code == ERROR_CODE_GHOST_SCRIPT_ERROR) {
		return "スクリプト読み込みエラーです。ゴーストのスクリプトが正しい状態で保存されているか確認してみてください。";
	}
	else if(code == ERROR_CODE_PREVIEW_SCRIPT_ERROR) {
		return "プレビュースクリプトで読み込みエラーが発生しました。";
	}
	
	return "プレビュー送信でエラーが発生しました。";
}
