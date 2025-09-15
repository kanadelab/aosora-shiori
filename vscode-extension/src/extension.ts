import * as vscode from 'vscode';
import {DebugSendProvider} from "./debugSendProvider";
import { DebugAdapterFactory, DebugConfigurationProvider } from './debugger';
import GetMessage from './messages';
import { SendScript } from './scriptPreview';

export function activate(context: vscode.ExtensionContext) {

	const createDiagnosticCollection = vscode.languages.createDiagnosticCollection("aosora-diagnostic");
	const debugSendProvider = new DebugSendProvider(context.extensionPath, createDiagnosticCollection);

	//ゴーストに送信コマンド
	const sendToGhostCommand = vscode.commands.registerCommand('aosora-shiori.sendToGhost', async (sendScript:SendScript, isError:boolean) => {
		await debugSendProvider.SendToGhost(sendScript, isError);
	});

	//スクリプトエラー表示関係
	context.subscriptions.push(sendToGhostCommand);
	context.subscriptions.push(vscode.languages.registerCodeLensProvider('aosora', debugSendProvider));

	//デバッガ
	context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('aosora', new DebugAdapterFactory(context.extensionPath)));
	context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('aosora', new DebugConfigurationProvider() ));

	
	vscode.commands.registerCommand("aosora.sendcaret", async () => {
		await debugSendProvider.SendFromMenu();
	});
}

export function deactivate() {}
