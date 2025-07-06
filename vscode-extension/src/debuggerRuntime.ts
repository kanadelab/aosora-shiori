import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import * as childProcess from 'child_process';
import * as os from 'os';
import GetMessage from './messages';

const isWindows = (os.type() === 'Windows_NT');

//ランタイム（SSP）を起動
export function LaunchDebuggerRuntime(extensionPath:string, runtimePath:string, ghostPath:string, projPath:string, onProcessExit?:()=>void){

	let runtimeResolvedPath = runtimePath;
	if(!path.isAbsolute(runtimePath)){
		runtimeResolvedPath = path.join(projPath, runtimePath);
	}
	if(isWindows && !fs.existsSync(runtimeResolvedPath)){
		throw new Error(`${GetMessage().debugger001}: ${runtimeResolvedPath}`);
	}

	if(!fs.existsSync(ghostPath)){
		throw new Error(GetMessage().debugger002);
	}

	if(!fs.existsSync(projPath)){
		throw new Error(GetMessage().debugger003);
	}

	//プロセス起動
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