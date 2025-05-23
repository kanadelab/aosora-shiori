import * as vscode from 'vscode';
import { Breakpoint, DebugSession, InitializedEvent, Scope, Source, StackFrame, StoppedEvent, TerminatedEvent, Thread } from '@vscode/debugadapter';
import { DebugProtocol } from '@vscode/debugprotocol';
import { AosoraDebuggerInterface, VariableInformation } from './debuggerInterface';
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

	public constructor() {
		super();
		this.debugInterface = new AosoraDebuggerInterface();

		//各種コールバックを設定
		this.debugInterface.onClose = () => {
			this.sendEvent(new TerminatedEvent());
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
	}

	protected initializeRequest(response: DebugProtocol.InitializeResponse, args: DebugProtocol.InitializeRequestArguments): void {

		//TODO: この辺でセットアップ
		response.body = {
			//supportsExceptionOptions: true,
			//supportsExceptionFilterOptions: true,
			supportsExceptionInfoRequest: true,
			exceptionBreakpointFilters: [
				{
					label: "すべてのエラー",
					description: "実行時にエラーが発生したとき、キャッチされるかどうかにかかわらず実行中断します。",
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

	//デバッグ起動のリクエスト（アタッチは別で存在している）
	protected async launchRequest(response: DebugProtocol.LaunchResponse, args: any) {
		console.log("test");
		this.debugInterface.Connect();
		this.sendResponse(response);
	}

	//例外ブレークポイント
	protected async setExceptionBreakPointsRequest(response: DebugProtocol.SetExceptionBreakpointsResponse, args: DebugProtocol.SetExceptionBreakpointsArguments, request?: DebugProtocol.Request): Promise<void> {
		this.debugInterface.SetExceptionBreakPoints(args.filters);
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
		if (breakInfo) {
			response.body.stackFrames = breakInfo.stackTrace.map(o => {
				return new StackFrame(o.id, o.name, this.createSource(o.filename), this.convertDebuggerLineToClient(o.line));
			});
		}
		this.sendResponse(response);
	}

	protected continueRequest(response: DebugProtocol.ContinueResponse, args: DebugProtocol.ContinueArguments, request?: DebugProtocol.Request): void {
		this.debugInterface.Continue();
		this.sendResponse(response);
	}

	protected nextRequest(response: DebugProtocol.NextResponse, args: DebugProtocol.NextArguments, request?: DebugProtocol.Request): void {
		this.debugInterface.StepOver();
		this.sendResponse(response);
	}

	protected stepInRequest(response: DebugProtocol.StepInResponse, args: DebugProtocol.StepInArguments, request?: DebugProtocol.Request): void {
		this.debugInterface.StepIn();
		this.sendResponse(response);
	}

	protected stepOutRequest(response: DebugProtocol.StepOutResponse, args: DebugProtocol.StepOutArguments, request?: DebugProtocol.Request): void {
		this.debugInterface.StepOut();
		this.sendResponse(response);
	}


	protected disconnectRequest(response: DebugProtocol.DisconnectResponse, args: DebugProtocol.DisconnectArguments, request?: DebugProtocol.Request): void {
		this.debugInterface.Disconnect();
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
		let variables: DebugProtocol.Variable[] = (await this.debugInterface.RequestObject(args.variablesReference)).map(o => this.convertVariable(o));;

		response.body = {
			variables
		};
		this.sendResponse(response);
	}

	//ヘルパ類
	private createSource(filename: string) {
		return new Source(basename(filename), this.convertDebuggerPathToClient(filename));
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
				type: 'null',
				variablesReference: -1
			}
		}
		else if (v.primitiveType != 'object') {
			return {
				name: v.key,
				value: v.value ?? null,
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