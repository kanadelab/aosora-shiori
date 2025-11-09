import * as vscode from 'vscode';
import { Breakpoint, DebugSession, InitializedEvent,  LoadedSourceEvent,  OutputEvent,  Scope, Source, StackFrame, StoppedEvent, TerminatedEvent, Thread } from '@vscode/debugadapter';
import { DebugProtocol } from '@vscode/debugprotocol';
import { AosoraDebuggerInterface, LoadedSource, VariableInformation } from './debuggerInterface';
import { basename } from 'path';
import { LaunchDebuggerRuntime } from './debuggerRuntime';
import { LogLevel, LogOutputEvent } from '@vscode/debugadapter/lib/logger';
import { ProjectParser } from './projectParser';
import * as crypto from 'crypto';
import path = require('path');
import GetMessage from './messages';

const DEFAULT_PORT_NUMBER = 27016;

//aosoraデバッガむけの設定情報
interface AosoraDebugConfiguration extends vscode.DebugConfiguration {
	port: number
};

//設定情報の補完、解決を行う
export class DebugConfigurationProvider implements vscode.DebugConfigurationProvider {
	resolveDebugConfiguration(folder: vscode.WorkspaceFolder | undefined, config: {}, token?: vscode.CancellationToken): vscode.ProviderResult<AosoraDebugConfiguration> {
		return {
			name: 'debug aosora',
			type: 'aosora',
			request: 'launch',
			port: DEFAULT_PORT_NUMBER,
			...config
		};
	}
}

export class DebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory {

	private extensionPath:string;
	public constructor(extensionPath:string){
		this.extensionPath = extensionPath;
	}

	createDebugAdapterDescriptor(session: vscode.DebugSession, executable: vscode.DebugAdapterExecutable | undefined): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
		return new vscode.DebugAdapterInlineImplementation(new AosoraDebugSession(this.extensionPath));
	}
}


/**
 * 蒼空デバッグセッション
 */
class AosoraDebugSession extends DebugSession {

	private debugInterface: AosoraDebuggerInterface;
	private extensionPath: string;

	public constructor(extensionPath:string) {
		super();
		this.debugInterface = new AosoraDebuggerInterface();
		this.extensionPath = extensionPath;

		//各種コールバックを設定
		this.debugInterface.onClose = () => {
			this.sendEvent(new TerminatedEvent());
		};

		this.debugInterface.onConnect = (editorDebuggerRevision:string, runtimeDebuggerRevision:string) => {
			if(editorDebuggerRevision !== runtimeDebuggerRevision){
				this.sendEvent(new OutputEvent("ゴーストとVSCode拡張のデバッグ機能バージョンが異なるため、正常に通信できない可能性があります。\n"));
			}
		}

		this.debugInterface.onNetworkError = () => {
			this.sendEvent(new OutputEvent("ゴーストとの通信でエラーが発生しました。", "stderr"));
		};

		this.debugInterface.onBreak = (errorMessage:string|null) => {
			const ev = new StoppedEvent('Breakpoint', 1, errorMessage ?? undefined);
			if(!errorMessage){
				ev.body.reason = 'breakpoint';
			}
			else {
				ev.body.reason = 'exception';
			}
			
			this.sendEvent(ev);
		};

		this.debugInterface.onMessage = (message:string, isError:boolean, filepath: string|null, line:number|null) => {

			const ev = new OutputEvent(message + "\n", isError ? 'stderr' : 'stdout') as DebugProtocol.OutputEvent ;
			if(line && filepath){
				ev.body.source = {
					path: filepath,
				};
				ev.body.line = line;
			}
			this.sendEvent(ev);
		}
	}

	protected initializeRequest(response: DebugProtocol.InitializeResponse, args: DebugProtocol.InitializeRequestArguments): void {


		response.body = {
			supportsExceptionInfoRequest: true,
			supportsLoadedSourcesRequest: true,
			supportsEvaluateForHovers: true,
			//supportsBreakpointLocationsRequest: true,
			supportedChecksumAlgorithms: ['MD5'],
			exceptionBreakpointFilters: [
				{
					label: GetMessage().debugger004,
					description: GetMessage().debugger005,
					filter: "all",
					default: false
				},
				{
					label: "キャッチされなかったエラー",
					description: "実行時にエラーが発生したとき、キャッチされなかった場合に実行中断します。",
					filter: "uncaught",
					default: true
				}
			]
		};

		this.sendResponse(response);

		//準備完了
		this.sendEvent(new InitializedEvent());
	}

	protected terminateRequest(response: DebugProtocol.TerminateResponse, args: DebugProtocol.TerminateArguments, request?: DebugProtocol.Request): void {
		this.debugInterface.Disconnect();
		this.sendResponse(response);
	}

	//デバッグ起動のリクエスト（アタッチは別で存在している）
	protected async launchRequest(response: DebugProtocol.LaunchResponse, args: any) :Promise<void>{

		//プロジェクトファイルをパース
		const ghostProj = await vscode.workspace.findFiles("**/ghost.asproj");
		if(ghostProj.length == 1){
			//プロジェクトファイルを読む
			const project = new ProjectParser();
			const projPath = ghostProj[0].fsPath;
			const debugPath = path.join(path.dirname(projPath), "debug.asproj");
			await project.Parse(projPath);
			await project.Parse(debugPath);

			//起動する
			if(project.runtimePath){
				const aosoraDir = path.dirname(projPath);
				const ghostPath = path.dirname(path.dirname(aosoraDir));	//プロジェクトの２階層上
				try{
					this.sendEvent(new OutputEvent(`${GetMessage().debugger007}\n`, "stdout"));
					LaunchDebuggerRuntime(this.extensionPath, project.runtimePath, ghostPath, aosoraDir, () => {
						this.sendEvent(new TerminatedEvent());
					});
				}
				catch(e){
					const err = e as Error
					vscode.window.showErrorMessage(err.message);
					this.sendEvent(new TerminatedEvent());	
					return;
				}

				try{
					await this.debugInterface.Connect();
				}
				catch{
					vscode.window.showErrorMessage(GetMessage().debugger008);
					this.sendEvent(new TerminatedEvent());	
					return;
				}
			}
			else {
				vscode.window.showErrorMessage(GetMessage().debugger009);
				this.sendEvent(new TerminatedEvent());
			}
		}
		else if(ghostProj.length == 0) {
			//terminateイベントを出す
			vscode.window.showErrorMessage(GetMessage().debugger010);
			this.sendEvent(new TerminatedEvent());
			return;
		}
		else {
			//terminateイベントを出す
			vscode.window.showErrorMessage(GetMessage().debugger011);
			this.sendEvent(new TerminatedEvent());
			return;
		}

		this.sendResponse(response);
	}

	//アタッチ
	protected async attachRequest(response: DebugProtocol.AttachResponse, args: DebugProtocol.AttachRequestArguments, request?: DebugProtocol.Request) {

		this.sendEvent(new OutputEvent(`${GetMessage().debugger012}\n`, "stdout"));

		//純粋に接続する形
		try{
			await this.debugInterface.Connect();
		}
		catch{
			vscode.window.showErrorMessage(GetMessage().debugger008);
			this.sendEvent(new TerminatedEvent());	
			return;
		}
		this.sendResponse(response);
	}

	//例外ブレークポイント
	protected async setExceptionBreakPointsRequest(response: DebugProtocol.SetExceptionBreakpointsResponse, args: DebugProtocol.SetExceptionBreakpointsArguments, request?: DebugProtocol.Request): Promise<void> {
		await this.debugInterface.WaitForConnect();
		await this.debugInterface.SetExceptionBreakPoints(args.filters);
		this.sendResponse(response);
	}

	protected exceptionInfoRequest(response: DebugProtocol.ExceptionInfoResponse, args: DebugProtocol.ExceptionInfoArguments, request?: DebugProtocol.Request): void {
		const breakInfo = this.debugInterface.GetBreakInfo();
		response.body = {
			breakMode: 'unhandled',
			exceptionId: breakInfo?.errorType ?? '',
			description: breakInfo?.errorMessage ?? ''
		};
		this.sendResponse(response);
	}

	//ブレークポイント
	protected async setBreakPointsRequest(response: DebugProtocol.SetBreakpointsResponse, args: DebugProtocol.SetBreakpointsArguments, request?: DebugProtocol.Request): Promise<void> {
		await this.debugInterface.WaitForConnect();

		//ファイル名と行番号を取得
		const filename = args.source.path ?? "";
		const lines = args.lines ?? [];

		//デバッガに送信する行数に変換
		const debuggerBreakPoints = lines.map(o => {
			return this.convertClientLineToDebugger(o);
		})

		//設定要求
		const enabledLines = await this.debugInterface.SetBreakPoints(this.convertClientPathToDebugger(filename), debuggerBreakPoints);
		const enabledLinesSet = new Set<number>(enabledLines);

		//ブレークポイントの状態を取得
		const editorBreeakPoints = lines.map(o => {
			const bp = new Breakpoint(enabledLinesSet.has(o-1), o);
			bp.setId(o);
			return bp;
		});

		//完了
		response.body = {
			breakpoints: editorBreeakPoints
		};
		this.sendResponse(response);
	}

	protected threadsRequest(response: DebugProtocol.ThreadsResponse, request?: DebugProtocol.Request): void {
		//Aosoraはシングルスレッドなので固定で返してしまう
		response.body = {
			threads: [
				new Thread(1, "Aosora MainThread")
			]
		};
		this.sendResponse(response);
	}

	protected stackTraceRequest(response: DebugProtocol.StackTraceResponse, args: DebugProtocol.StackTraceArguments, request?: DebugProtocol.Request): void {
		//トレース情報を返す
		response.body = {
			stackFrames: []
		};

		const breakInfo = this.debugInterface.GetBreakInfo();
		if (breakInfo) {
			response.body.stackFrames = breakInfo.stackTrace.map(o => {
				return new StackFrame(o.id, o.name, this.createSource(o.filename), this.convertDebuggerLineToClient(o.line));
			});
		}
		this.sendResponse(response);
	}

	//実行
	protected async continueRequest(response: DebugProtocol.ContinueResponse, args: DebugProtocol.ContinueArguments, request?: DebugProtocol.Request): Promise<void>{
		await this.debugInterface.Continue();
		this.sendResponse(response);
	}

	//ステップオーバー
	protected async nextRequest(response: DebugProtocol.NextResponse, args: DebugProtocol.NextArguments, request?: DebugProtocol.Request): Promise<void> {
		await this.debugInterface.StepOver();
		this.sendResponse(response);
	}

	//ステップイン
	protected async stepInRequest(response: DebugProtocol.StepInResponse, args: DebugProtocol.StepInArguments, request?: DebugProtocol.Request): Promise<void> {
		await this.debugInterface.StepIn();
		this.sendResponse(response);
	}

	//ステップアウト
	protected async stepOutRequest(response: DebugProtocol.StepOutResponse, args: DebugProtocol.StepOutArguments, request?: DebugProtocol.Request): Promise<void> {
		await this.debugInterface.StepOut();
		this.sendResponse(response);
	}

	//切断
	protected disconnectRequest(response: DebugProtocol.DisconnectResponse, args: DebugProtocol.DisconnectArguments, request?: DebugProtocol.Request): void {
		this.debugInterface.Disconnect();
		this.sendResponse(response);
	}

	protected pauseRequest(response: DebugProtocol.PauseResponse, args: DebugProtocol.PauseArguments, request?: DebugProtocol.Request): void {
		vscode.window.showErrorMessage(GetMessage().debugger013);
		this.sendResponse(response);
	}

	//変数情報リクエスト
	protected async scopesRequest(response: DebugProtocol.ScopesResponse, args: DebugProtocol.ScopesArguments, request?: DebugProtocol.Request): Promise<void> {

		const scopes = await this.debugInterface.RequestEnumScopes(args.frameId);
		response.body = {
			scopes: scopes.map(o => new Scope(o.name, o.handle))
		};
		this.sendResponse(response);
	}

	protected async variablesRequest(response: DebugProtocol.VariablesResponse, args: DebugProtocol.VariablesArguments, request?: DebugProtocol.Request): Promise<void> {

		//変数
		let variables: DebugProtocol.Variable[] = (await this.debugInterface.RequestObject(args.variablesReference)).map(o => this.convertVariable(o));

		response.body = {
			variables
		};
		this.sendResponse(response);
	}

	protected async evaluateRequest(response: DebugProtocol.EvaluateResponse, args: DebugProtocol.EvaluateArguments, request?: DebugProtocol.Request): Promise<void> {

		if(args.frameId !== undefined){
			const expression = args.expression;
			const frameId = args.frameId;

			switch (args.context) {
				case 'watch':
				case 'hover':
					const result = await this.debugInterface.RequestEvaluateExpression(expression, frameId);

					if(result.value){
						const variable = this.convertVariable(result.value);
						response.body = {
							result: variable.value ?? "",
							variablesReference: variable.variablesReference,
							type: variable.type
						};
						this.sendResponse(response);
						return;
					}
					else if(result.exception && args.context !== 'hover') {
						const variable = this.convertVariable(result.exception);
						response.body = {
							result: (result.errorType ?? "Error") + ": " + (variable.value ?? ""),
							variablesReference: variable.variablesReference,
							type: variable.type
						};
						this.sendResponse(response);
						return;
					}
					
			}
		}

		if(args.context === 'hover'){
			//ホバー時情報が正しくなくてもはなにもしない
			this.sendResponse(response);
		}

		response.body = {
			result: 'not supported',
			variablesReference: 0
		};
		this.sendResponse(response);
	}

	//読み込みソースリクエスト
	protected async loadedSourcesRequest(response: DebugProtocol.LoadedSourcesResponse, args: DebugProtocol.LoadedSourcesArguments, request?: DebugProtocol.Request): Promise<void> {
		await this.debugInterface.WaitForConnect();
		const sources = await this.debugInterface.RequestLoadedSource();
		response.body = {
			sources: sources.map(o => this.createLoadedsource(o))
		};
		this.sendResponse(response);
	}

	//ブレーク位置リクエスト
	//NOTE: 動作が期待と違う（自動的に無効状態にするとかでない）ので無効にしておく
	//		行内ブレークポイントの設定可能位置収集に近い
	/*
	protected async breakpointLocationsRequest(response: DebugProtocol.BreakpointLocationsResponse, args: DebugProtocol.BreakpointLocationsArguments, request?: DebugProtocol.Request): Promise<void> {
		if(!args.source.path){
			response.body = {
				breakpoints: []
			};
			this.sendResponse(response);
		}
		else {
			const filename = this.convertDebuggerPathToClient(args.source.path);
			const lines = await this.debugInterface.RequestBreakpointLocations(filename);
			response.body = {
				breakpoints: lines.map(o => ({line: o+1}))
			};
			this.sendResponse(response);
		}
	}
	*/

	//ヘルパ類
	private createSource(filename: string) {
		return new Source(basename(filename), this.convertDebuggerPathToClient(filename));
	}

	private createLoadedsource(source: LoadedSource):DebugProtocol.Source{
		return {
			path: source.path, 
			checksums: source.md5 ? [{algorithm: 'MD5', checksum: source.md5}] : undefined
		};
	}

	private convertVariable(v: VariableInformation): DebugProtocol.Variable {
		if(v.primitiveType == 'null'){
			return {
				name: v.key,
				value: 'null',
				type: 'null',
				variablesReference: -1,
			};
		}
		else if(v.primitiveType == 'string'){
			return {
				name: v.key,
				value: `"${v.value}"`,
				type: v.primitiveType,
				variablesReference: -1
			}
		}
		else if (v.primitiveType != 'object') {
			return {
				name: v.key,
				value: v.value?.toString() ?? null,
				type: v.primitiveType,
				variablesReference: -1
			};
		}
		else {
			return {
				name: v.key,
				value: `(${v.objectType})`,
				type: v.primitiveType,
				variablesReference: v.objectHandle
			};
		}
	}
}