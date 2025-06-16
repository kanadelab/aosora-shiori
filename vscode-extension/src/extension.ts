// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import {DebugSendProvider} from "./debugSendProvider";
import {SendPreviewFunction} from './scriptPreview';
import { DebugAdapterFactory, DebugConfigurationProvider } from './debugger';

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

	let disposable = vscode.commands.registerCommand('aosora-shiori.sendToGhost', async (scriptBody:string) => {
		if(scriptBody){
			await SendPreviewFunction(scriptBody, context.extensionPath);
		}
	});

	context.subscriptions.push(disposable);
	context.subscriptions.push(vscode.languages.registerCodeLensProvider('*', new DebugSendProvider()));
	context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('aosora', new DebugAdapterFactory(context.extensionPath)));
	context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('aosora', new DebugConfigurationProvider() ));
}

// This method is called when your extension is deactivated
export function deactivate() {}
