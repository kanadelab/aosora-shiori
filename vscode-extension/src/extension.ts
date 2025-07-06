import * as vscode from 'vscode';
import {DebugSendProvider} from "./debugSendProvider";
import {SendPreviewFunction} from './scriptPreview';
import { DebugAdapterFactory, DebugConfigurationProvider } from './debugger';

export function activate(context: vscode.ExtensionContext) {

	//ゴーストに送信コマンド
	const sendToGhostCommand = vscode.commands.registerCommand('aosora-shiori.sendToGhost', async (scriptBody:string, isError:boolean) => {
		if(!isError){
			if(scriptBody){
				await SendPreviewFunction(scriptBody, context.extensionPath);
			}
		}
		else{
			vscode.window.showErrorMessage("スクリプトの読み込みに失敗しました。エラーを修正してから試してみてください。");
		}
	});

	//スクリプトエラー表示関係
	const createDiagnosticCollection = vscode.languages.createDiagnosticCollection("aosora-diagnostic");

	context.subscriptions.push(sendToGhostCommand);
	context.subscriptions.push(vscode.languages.registerCodeLensProvider('*', new DebugSendProvider(context.extensionPath, createDiagnosticCollection)));

	//デバッガ
	context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('aosora', new DebugAdapterFactory(context.extensionPath)));
	context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('aosora', new DebugConfigurationProvider() ));
}

export function deactivate() {}
