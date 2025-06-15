import { randomUUID } from 'crypto';
import {number, z} from 'zod';
import * as Net from 'net';

export const DEBUGGER_REVISION = "0";

//Aosora デバッガインターフェース
type BreakPointRequest = {
	filename: string,
	lines: number[]
};

const DebuggerReceiveFormat = z.object({
	type: z.string(),
	responseId: z.string(),
	body: z.any()
})
type DebuggerReceiveFormat = z.infer<typeof DebuggerReceiveFormat>;

const StackFrame = z.object({
	id: z.number(),
	index: z.number(),
	name: z.string(),
	filename: z.string(),
	line: z.number()
});
type StackFrame = z.infer<typeof StackFrame>;

const BreakHitRequest = z.object({
	filename: z.string(),
	line: z.number(),
	stackTrace: z.array(StackFrame),
	errorMessage: z.nullable(z.string()),
	errorType: z.nullable(z.string())
});
type BreakHitRequest = z.infer<typeof BreakHitRequest>;

const MessageRequest = z.object({
	message: z.string(),
	isError: z.boolean(),
	filepath: z.optional(z.string()),
	line: z.optional(z.number())
});
type MessageRequest = z.infer<typeof MessageRequest>;

const VariableInformation = z.object({
	key: z.string(),
	primitiveType: z.string(),
	objectType: z.string(),
	objectHandle: z.number(),
	value: z.any()
});
export type VariableInformation = z.infer<typeof VariableInformation>;

const ScopeInformation = z.object({
	name: z.string(),
	handle: z.number()
});
export type ScopeInformation = z.infer<typeof ScopeInformation>;

const VariableScopeResponse = z.object({
	variables: z.array(VariableInformation)
});
export type VariableScopeResponse = z.infer<typeof VariableScopeResponse>;

const EnumScopeResponse = z.object({
	scopes: z.array(ScopeInformation)
});
export type EnumScopeResponse = z.infer<typeof EnumScopeResponse>;

const LoadedSource = z.object({
	path: z.string(), 
	md5: z.string()
});
export type LoadedSource = z.infer<typeof LoadedSource>;

const LoadedSourcesResponse = z.object({
	files: z.array(LoadedSource)
});
export type LoadedSourcesResponse = z.infer<typeof LoadedSourcesResponse>;

const BreakpointLocationsResponse = z.object({
	lines: z.array(z.number())
});
export type BreakpointLocationsResponse = z.infer<typeof BreakpointLocationsResponse>;

const VersionResponse = z.object({
	version: z.string(),
	debuggerRevision: z.string()
});
export type VersionResponse = z.infer<typeof VersionResponse>;

export class AosoraDebuggerInterface {

	public onConnect:(editorDebuggerRevision:string, runtimeDebuggerRevision:string) => void;
	public onClose:() => void;
	public onBreak:(errorMessage: string|null) => void;
	public onMessage:(message:string, isError:boolean, filepath:string|null, line:number|null) => void;
	public onNetworkError:() => void;

	private socketClient:Net.Socket|null;
	private breakInfo:BreakHitRequest|null;
	private connectWaitList:(()=>void)[];
	private isConnected = false;
	private isConnectCancel = false;

	//待機レスポンスリスト
	private responseMap:Map<string, (body:any, error: any) => void>;

	public constructor(){
		this.onConnect = () => {};
		this.onClose = () => {};
		this.onBreak = () => {};
		this.onMessage = () => {};
		this.onNetworkError = () => {};
		this.responseMap = new Map<string, ()=>void>();
		this.socketClient = null;
		this.breakInfo = null;
		this.connectWaitList = [];
	}

	private NotifyError():never{
		this.onNetworkError();
		throw new Error();
	}

	//ブレーク情報取得
	public GetBreakInfo(){
		return this.breakInfo;
	}

	private async Wait(ms:number){
		return new Promise<void>(resolve => {
			setTimeout(() => resolve(), ms);
		});
	}

	//接続：内部
	private async ConnectInternal(){
		return new Promise<void>((resolve, reject) => {
				this.socketClient = Net.connect(27016, 'localhost', () => {
				resolve();
				for(const callback of this.connectWaitList){
					callback();
				}
				this.isConnected = true;
			});

			//受信
			this.socketClient.on('data', (data => {
				let offset = 0;
				while(true){
					const index = data.indexOf(0, offset);
					if(index < 0){
						break;
					}					

					//リクエスト処理
					let requestObj = null;
					try{
						const dataStr = data.toString('utf8', offset, index);
						requestObj = JSON.parse(dataStr);
					}
					catch{
						console.log("json parse error");
					}
					this.Recv(requestObj);
					offset = index + 1;
				}
				
			}));

			//終了
			this.socketClient.on('close', () => {
				console.log('client-> connection is closed');
				if(!this.isConnected){
					//接続待ち
					reject();
				}
				else{
					this.onClose();
				}
			});
		});
	}

	//接続
	public async Connect() {

		//30秒待つ形
		for(let i = 0; i < 30; i++){
			if(this.isConnectCancel){
				return;
			}
			try{
				await this.ConnectInternal();

				//バージョン要求
				const versionInfo = await this.RequestVersion();	//接続成功
				this.onConnect(DEBUGGER_REVISION, versionInfo.debuggerRevision);
				return;
			}
			catch{
				//接続待機
				await this.Wait(1000);
			}
		}

		//接続失敗
		throw new Error();
	}

	//接続中の場合、それを待つ
	public async WaitForConnect(){
		return new Promise<void>((resolve) => {
			if(!this.isConnected){
				this.connectWaitList.push(resolve);
			}
			else{
				resolve();
			}
		});
	}

	public IsConnectCancel(){
		return this.isConnectCancel;
	}

	//リクエストの送信
	public Send(requestType:string, requestBody: {}, callback?: (response:any, error:any) => void ){
		const request = {
			type: requestType,
			body: requestBody,
			id: ""
		};

		if(callback){
			//コールバック要求の場合コールバック用の一意IDを作成し待機列にいれる
			request.id = randomUUID();
			this.responseMap.set(request.id, callback);
		}

		//リクエスト送信、末尾に0をつけて終端にする
		const buff = (new TextEncoder).encode(JSON.stringify(request));
		const sendBuff = new Uint8Array(buff.length + 1);
		sendBuff.set(buff);
		sendBuff.set([0], buff.length);		//0終端で送る
		this.socketClient?.write(sendBuff);
	}

	//Promise版のSend
	public async SendPromise(requestType: string, requestBody: {}):Promise<any>{
		return new Promise((resolve, reject) => {
			this.Send(requestType, requestBody, (r, e) => {
				if(e){
					reject(e);
					return;
				}
				resolve(r);
			})
		});
	}

	private Recv(requestObj:any) {

		//受信時、ブレークヒットのようなクライアントからのリクエストか、ウォッチのようなレスポンスかを判断し、レスポンスならコールバックする必要がある
		const parsedRequest = DebuggerReceiveFormat.safeParse(requestObj);
		if(!parsedRequest.success){
			return;
		}

		const req = parsedRequest.data;
		const body = parsedRequest.data.body;

		if(req.type == 'break'){
			const parsedBody = BreakHitRequest.safeParse(body);
			if(parsedBody.success){
				this.RecvBreak(parsedBody.data);
			}
		}
		else if(req.type == 'message'){
			const parsedMessage = MessageRequest.safeParse(body);
			if(parsedMessage.success){
				this.onMessage(parsedMessage.data.message, parsedMessage.data.isError,
					parsedMessage.data.filepath ?? null, parsedMessage.data.line ?? null
				);
			}
		}
		else if(req.type == "response"){
			const callback = this.responseMap.get(req.responseId);
			if(callback){
				this.responseMap.delete(req.responseId);
				callback(body, false);
			}
		}
		else if(req.type == "error_response"){
			const callback = this.responseMap.get(req.responseId);
			if(callback){
				this.responseMap.delete(req.responseId);
				callback(null, true);
			}
		}
	}

	//ブレークリクエスト
	private RecvBreak(request: BreakHitRequest){
		this.breakInfo = request;
		this.onBreak(this.breakInfo.errorMessage);
	}

	//-- エディタ向けインターフェース

	//ブレークポイント設定（エディタ側都合でファイルごとに差し替えの形）
	public async SetBreakPoints(filename: string, lines: number[]):Promise<number[]>{
		const requestBody = {
			filename:  filename.replace("\\\\", "\\"),
			lines
		};
		const response = await this.SendPromise('set_breakpoints', requestBody);
		const parsedResponse = BreakpointLocationsResponse.safeParse(response);
		if(parsedResponse.success){
			return parsedResponse.data.lines;
		}
		this.NotifyError();
	}

	//例外ブレークポイント設定
	public async SetExceptionBreakPoints(exceptions: string[]){
		const requestBody = {
			filters: exceptions
		};
		this.SendPromise('set_exception_breakpoint', requestBody);
	}

	public RequestEnumScopes(stackIndex:number){
		return new Promise<ScopeInformation[]>((resolve, reject) => {
			this.Send('scopes',{stackIndex: stackIndex}, (response, error) => {
				if(error){
					reject();
				}

				const parsedScopes = EnumScopeResponse.safeParse(response);
				if(parsedScopes.success){
					resolve(parsedScopes.data.scopes);
				}
				else {
					reject();
				}
			});
		});
	}

	public RequestObject(handle:number){
		return new Promise<VariableInformation[]>((resolve, reject) => {
			this.Send('members', {handle: handle}, (response, error) => {
				if(error){
					reject();
				}

				const parsedVariables = VariableScopeResponse.safeParse(response);
				if(parsedVariables.success){
					resolve(parsedVariables.data.variables);
				}
				else {
					resolve([]);
				}

			});
		});
	}

	//デバッグ続行
	public async Continue(){
		await this.SendPromise("continue", {});
	}

	public async StepIn(){
		await this.SendPromise("stepin", {});
	}

	public async StepOut(){
		await this.SendPromise("stepout", {});
	}

	public async StepOver(){
		await this.SendPromise("stepover", {});
	}

	public async RequestLoadedSource(){
		const response = await this.SendPromise("loaded_sources", {});
		const parsedResponse = LoadedSourcesResponse.safeParse(response);
		if(parsedResponse.success){
			return parsedResponse.data.files;
		}
		this.NotifyError();
	}

	public async RequestBreakpointLocations(filename:string){
		const response = await this.SendPromise("breakpoint_locations", {
			filename:  filename.replace("\\\\", "\\"),
		});
		const parsedResponse = BreakpointLocationsResponse.safeParse(response);
		if(parsedResponse.success){
			return parsedResponse.data.lines;
		}
		this.NotifyError();
	}

	public async RequestVersion() {
		const response = await this.SendPromise("version", {});
		const parsedResponse = VersionResponse.safeParse(response);
		if(parsedResponse.success){
			return parsedResponse.data;
		}
		this.NotifyError();
	}

	//切断
	public Disconnect(){
		this.isConnectCancel = true;
		this.socketClient?.end();
	}
}