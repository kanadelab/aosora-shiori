import * as vscode from 'vscode';
import { SCRIPT_BLOCK_TYPE_FUNCTION, ScriptAnalyzer } from './scriptAnalyzer';

const functionPattern = /(function|talk)/g;

export class DebugSendProvider implements vscode.CodeLensProvider {
    
    provideCodeLenses(document: vscode.TextDocument, token: vscode.CancellationToken): vscode.ProviderResult<vscode.CodeLens[]> {
        const result:vscode.CodeLens[] = [];

        /*
        const regex = new RegExp(functionPattern);
        const text = document.getText();
        let match;
        while ((match = regex.exec(text)) !== null) {
            const line = document.lineAt(document.positionAt(match.index).line);
            const indexOf = line.text.indexOf(match[0]);
            const position = new vscode.Position(line.lineNumber, indexOf);
            const range = document.getWordRangeAtPosition(position, functionPattern);
            if (range) {
                result.push(new vscode.CodeLens(range));
            }
        }
        */

       

        //データを解析
        const analyzer = new ScriptAnalyzer();
        const blocks = analyzer.Analyze(document);

        for(const item of blocks){
            if(item.blockType == SCRIPT_BLOCK_TYPE_FUNCTION){

                const line = document.lineAt(document.positionAt(item.beginIndex).line);
                const position = new vscode.Position(line.lineNumber, 0);
                const range = document.getWordRangeAtPosition(position, functionPattern);

                //関数ブロックとして
                if(range){
                    const codeLens = new vscode.CodeLens(range);
                    codeLens.command = {
                        title: "<< ゴーストに送信",
                        command: "aosora-shiori.sendToGhost",
                        tooltip: "この関数を実行してゴーストに送信します。",
                        arguments: [item.sendScriptBody]
                    };
                    result.push(codeLens);
                }

            }
        }

        return result;
    }
}