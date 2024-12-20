import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import * as childProcess from 'child_process';
import * as iconv from 'iconv-lite';

//スクリプトプレビューイング
export async function SendPreviewFunction(functionBody:string, extensionPath:string){

    //TODO: TypeScriptがわ: 一時的なスクリプトを保存し、引数をつけて実行ファイルを呼出、その後保存。リザルトはstdoutから読む。
    //TODO: 実行ファイル側: コマンド引数で入力された一時スクリプトとゴーストパスを読み、FMOをみてターゲットのゴーストに送る

    const outPath = extensionPath + "/" + '_aosora_send_script_.as';
    const executablePath = extensionPath + "/" + "aosora-sstp.exe";
    let command = `"${executablePath}" "${outPath}"`;

    //ワークスペースがあればパスに足す
    const projFiles = await vscode.workspace.findFiles("**/ghost.asproj", null, 1);
    if(projFiles.length > 0){
        const workspace = path.dirname(projFiles[0].fsPath);
        command += ` ${workspace}`;
    }

    //一時ファイルを用意して呼び出す
    fs.writeFileSync(outPath, functionBody, 'utf-8');
    
    childProcess.exec(command, () => {
        fs.rmSync(outPath);
    });
    
}