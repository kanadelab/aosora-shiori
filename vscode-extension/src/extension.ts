// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import {DebugSendProvider} from "./debugSendProvider";
import {SendPreviewFunction} from './scriptPreview';
import { DebugAdapterFactory, DebugConfigurationProvider } from './debugger';

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	console.log('Congratulations, your extension "aosora-shiori" is now active!');

	// The command has been defined in the package.json file
	// Now provide the implementation of the command with registerCommand
	// The commandId parameter must match the command field in package.json
	let disposable = vscode.commands.registerCommand('aosora-shiori.helloWorld', async (scriptBody:string) => {
		// The code you place here will be executed every time your command is executed
		// Display a message box to the user
		if(scriptBody){
			//vscode.window.showInformationMessage(scriptBody);
			await SendPreviewFunction(scriptBody, context.extensionPath);
		}
		else {
			vscode.window.showInformationMessage('Hello World from aosora-shiori!');
		}
	});

	context.subscriptions.push(disposable);
	context.subscriptions.push(vscode.languages.registerCodeLensProvider('*', new DebugSendProvider()));
	context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('aosora', new DebugAdapterFactory(context.extensionPath)));
	context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('aosora', new DebugConfigurationProvider() ));

	context.subscriptions.push(
		vscode.commands.registerCommand('extension.aosora-shiori.debugEditorContents', (resource: vscode.Uri) => {
			console.log("OnDebug");
			vscode.debug.startDebugging(undefined, {
				type: 'aosora',
				name: 'Aosora Debug',
				request: 'launch'
			})
		})
	);

}

// This method is called when your extension is deactivated
export function deactivate() {}
