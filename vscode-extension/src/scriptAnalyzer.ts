import * as vscode from 'vscode';
import { exec } from 'child_process';
import {z} from 'zod';
import {IsBinaryExecutablePlatform, IsJapaneseLanguage} from './utility';

const AnalyzedSourceRange = z.object({
	line: z.number(),
	column: z.number(),
	endLine: z.number(),
	endColumn: z.number()
});

const AnalyzeResult = z.object({
	error: z.boolean(),
	message: z.optional(z.string()),
	range: z.optional(AnalyzedSourceRange),
	functions: z.optional(z.array(z.object({
		range: AnalyzedSourceRange
	})))
});

export type AnalyzedSourceRange = z.infer<typeof AnalyzedSourceRange>;
export type AnalyzeResult = z.infer<typeof AnalyzeResult>;


function AnalyzeScript(script:string, extensionPath:string):Promise<AnalyzeResult>{

	const executablePath = ((IsBinaryExecutablePlatform()) ? (extensionPath + "/" + "aosora-analyzer.exe") : ("aosora-analyzer"));
	let command = executablePath;

	//日本語以外だったら英語で起動する
	if(IsJapaneseLanguage()){
		command += " --language ja-jp";
	}
	else {
		command += " --language en-us";
	}

	return new Promise<string>((resolve, reject) => {
		const child = exec(command, (error, stdout) => {
			if (error) {
				reject(error);
				return;
			}
			resolve(JSON.parse(stdout));
		});
		if (child.stdin) {
			child.stdin.write(script);
			child.stdin.end();
		}
	})
		.then(o => AnalyzeResult.parseAsync(o))
		.catch( () => ({error: true, message: "exec error"}));
}

//ランタイムベースのスクリプトアナライザ
export async function Analyze(document:vscode.TextDocument, extensionPath:string){
	const str = document.getText();
	return await AnalyzeScript(str,extensionPath);
}