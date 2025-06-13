import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import * as childProcess from 'child_process';

//ランタイム（SSP）を起動
export function LaunchDebuggerRuntime(extensionPath:string, runtimePath:string, ghostPath:string, projPath:string, onProcessExit?:()=>void){

	let runtimeResolvedPath = path.join(projPath, runtimePath);
	if(!fs.existsSync(runtimeResolvedPath)){
		throw new Error(`debug.debugger.runtime で設定されたパスにファイルが見つかりませんでした: ${runtimeResolvedPath}`);
	}

	if(!fs.existsSync(ghostPath)){
		throw new Error('ゴーストフォルダを検出できませんでした。');
	}

	if(!fs.existsSync(projPath)){
		throw new Error('aosora プロジェクトフォルダを検出できませんでした。');
	}

	//プロセス終了検出？
	const command = `"${extensionPath}\\launch.bat" "${runtimePath}" "${ghostPath}" "${projPath}"`;
	childProcess.exec(command, (error, stdout, stderr) => {
		if(error){
			console.error(error);
		}
		else {
			//プロセス正常終了
		}
		if(onProcessExit){
			onProcessExit();
		}
	});
}