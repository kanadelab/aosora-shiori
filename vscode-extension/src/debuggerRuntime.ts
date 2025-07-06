import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import * as childProcess from 'child_process';
import * as os from 'os';
import {IsBinaryExecutablePlatform} from './utility';

const isWindows = (os.type() === 'Windows_NT');

//ランタイム（SSP）を起動
export function LaunchDebuggerRuntime(extensionPath:string, runtimePath:string, ghostPath:string, projPath:string, onProcessExit?:()=>void){

	if(!IsBinaryExecutablePlatform()){
		throw new Error("この機能はwindowsプラットフォームでのみ使用できます");
	}

	let runtimeResolvedPath = runtimePath;
	if(!path.isAbsolute(runtimePath)){
		runtimeResolvedPath = path.join(projPath, runtimePath);
	}
	if(isWindows && !fs.existsSync(runtimeResolvedPath)){
		throw new Error(`debug.debugger.runtime で設定されたパスにファイルが見つかりませんでした: ${runtimeResolvedPath}`);
	}

	if(!fs.existsSync(ghostPath)){
		throw new Error('ゴーストフォルダを検出できませんでした。');
	}

	if(!fs.existsSync(projPath)){
		throw new Error('aosora プロジェクトフォルダを検出できませんでした。');
	}

	//プロセス終了検出？
	const scriptPath = extensionPath + ((isWindows) ? ("\\launch.bat") : ("/launch.sh"));
	const command = `"${scriptPath}" "${runtimePath}" "${ghostPath}" "${projPath}"`;
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