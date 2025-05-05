import * as vscode from 'vscode';
import { Breakpoint, DebugSession, Handles, InitializedEvent, Scope, Source, StackFrame, StoppedEvent, TerminatedEvent, Thread } from '@vscode/debugadapter';
import { DebugProtocol } from '@vscode/debugprotocol';
import { AosoraDebuggerInterface, VARIABLE_SCOPE_LOCAL, VARIABLE_SCOPE_SHIORI_REQUEST, VariableInformation, VariableScope } from './debuggerInterface';
import { basename } from 'path';
export class DebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory {
	createDebugAdapterDescriptor(session: vscode.DebugSession, executable: vscode.DebugAdapterExecutable | undefined): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
		return new vscode.DebugAdapterInlineImplementation(new AosoraDebugSession());
	}
}


/**
 * 蒼空デバッグセッション
 */
class AosoraDebugSession extends DebugSession {

	private debugInterface: AosoraDebuggerInterface;
	private variableHandles = new Handles<VariableScope>();

	public constructor() {
		super();
		this.debugInterface = new AosoraDebuggerInterface();

		//各種コールバックを設定
		this.debugInterface.onClose = () => {
			this.sendEvent(new TerminatedEvent());
		};

		this.debugInterface.onBreak = () => {
			this.sendEvent(new StoppedEvent('Breakpoint', 1));
		};
	}

	protected initializeRequest(response: DebugProtocol.InitializeResponse, args: DebugProtocol.InitializeRequestArguments): void {

		//TODO: この辺でセットアップ

		this.sendResponse(response);

		//準備完了
		this.sendEvent(new InitializedEvent());
	}

	//デバッグ起動のリクエスト（アタッチは別で存在している）
	protected async launchRequest(response: DebugProtocol.LaunchResponse, args: any) {
		console.log("test");
		this.debugInterface.Connect();
		this.sendResponse(response);
	}

	//ブレークポイント
	protected async setBreakPointsRequest(response: DebugProtocol.SetBreakpointsResponse, args: DebugProtocol.SetBreakpointsArguments, request?: DebugProtocol.Request): Promise<void> {

		//ファイル名と行番号を取得
		const filename = args.source.path ?? "";
		const lines = args.lines ?? [];

		//ブレークポイントを単純に全採用状態にする
		const editorBreeakPoints = lines.map(o => {
			const bp = new Breakpoint(true, o);
			bp.setId(o);
			return bp;
		});

		//デバッガに送信する行数に変換
		const debuggerBreakPoints = lines.map(o => {
			return this.convertClientLineToDebugger(o);
		})

		//設定要求
		await this.debugInterface.SetBreakPoints(this.convertClientPathToDebugger(filename).toLowerCase(), debuggerBreakPoints);

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
		if(breakInfo){
			response.body.stackFrames = breakInfo.stackTrace.map(o => {
				return new StackFrame(o.line, o.name, this.createSource(o.filename), this.convertDebuggerLineToClient(o.line));
			});
		}
		this.sendResponse(response);
	}

	protected continueRequest(response: DebugProtocol.ContinueResponse, args: DebugProtocol.ContinueArguments, request?: DebugProtocol.Request): void {
		this.debugInterface.Continue();
		this.sendResponse(response);
	}

	protected disconnectRequest(response: DebugProtocol.DisconnectResponse, args: DebugProtocol.DisconnectArguments, request?: DebugProtocol.Request): void {
		this.debugInterface.Disconnect();
		this.sendResponse(response);
	}

	//変数情報リクエスト
	protected scopesRequest(response: DebugProtocol.ScopesResponse, args: DebugProtocol.ScopesArguments, request?: DebugProtocol.Request): void {
		response.body = {
			scopes: [
				new Scope("SHIORI Request", this.variableHandles.create({path: [], scope: VARIABLE_SCOPE_SHIORI_REQUEST, stackId: 0}), false),
				new Scope("ローカル変数", this.variableHandles.create({path: [], scope: VARIABLE_SCOPE_LOCAL, stackId: args.frameId}), false)
			]
		};
		this.sendResponse(response);
	}

	protected async variablesRequest(response: DebugProtocol.VariablesResponse, args: DebugProtocol.VariablesArguments, request?: DebugProtocol.Request): Promise<void> {
		const v = this.variableHandles.get(args.variablesReference);
		
		let variables:DebugProtocol.Variable[] = [];

		//ルート空間の情報要求
		if(v.path.length == 0){
			variables = (await this.debugInterface.RequestScope(v)).map(o => this.convertVariable(o));
		}
		
		response.body = {
			variables
		};
		this.sendResponse(response);
	}

	//ヘルパ類
	private createSource(filename:string){
		return new Source(basename(filename), this.convertDebuggerPathToClient(filename));
	}

	private convertVariable(v:VariableInformation):DebugProtocol.Variable{
		if(v.primitiveType != 'object'){
			return {
				name: v.key,
				value: v.value ?? null,
				type: v.primitiveType,
				variablesReference: 0	//TODO: 下位リクエスト用のハンドル
			};
		}
		else if(v.value ?? null === null) {
			return {
				name: v.key,
				value: 'null',
				type: 'null',
				variablesReference: 0,
			};
		}
		else{
			return {
				name: v.key,
				value: `(${v.objectType})`,
				type: v.primitiveType,
				variablesReference: 0
			};
		}
	}
}