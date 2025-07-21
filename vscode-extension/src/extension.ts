import * as vscode from 'vscode';
import {DebugSendProvider} from "./debugSendProvider";
import {SendPreviewFunction} from './scriptPreview';
import { DebugAdapterFactory, DebugConfigurationProvider } from './debugger';
import GetMessage from './messages';

export function activate(context: vscode.ExtensionContext) {

	//ゴーストに送信コマンド
	const sendToGhostCommand = vscode.commands.registerCommand('aosora-shiori.sendToGhost', async (scriptBody:string, isError:boolean) => {
		if(!isError){
			if(scriptBody){
				await SendPreviewFunction(scriptBody, context.extensionPath);
			}
		}
		else{
			vscode.window.showErrorMessage(GetMessage().scriptPreview002);
		}
	});

	//スクリプトエラー表示関係
	const createDiagnosticCollection = vscode.languages.createDiagnosticCollection("aosora-diagnostic");

	context.subscriptions.push(sendToGhostCommand);
	context.subscriptions.push(vscode.languages.registerCodeLensProvider('aosora', new DebugSendProvider(context.extensionPath, createDiagnosticCollection)));

	//デバッガ
	context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('aosora', new DebugAdapterFactory(context.extensionPath)));
	context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('aosora', new DebugConfigurationProvider() ));
}

export function deactivate() {}
