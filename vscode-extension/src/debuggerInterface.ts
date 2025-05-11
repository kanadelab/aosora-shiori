import { randomUUID } from 'crypto';
import {number, z} from 'zod';
import * as Net from 'net';

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
	stackTrace: z.array(StackFrame)
});
type BreakHitRequest = z.infer<typeof BreakHitRequest>;

const VariableScope = z.object({
	stackId: z.number(),		//スタックフレームのID
	scope: z.string(),
	path: z.array(z.string())
});

export type VariableScope = z.infer<typeof VariableScope>;
export const VARIABLE_SCOPE_GLOBAL = 'globals';
export const VARIABLE_SCOPE_LOCAL = 'locals';
export const VARIABLE_SCOPE_SHIORI_REQUEST = 'shiori_request';

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

export class AosoraDebuggerInterface {

	public onClose:() => void;
	public onBreak:(filename:string, line:number) => void;

	private socketClient:Net.Socket|null;
	private breakInfo:BreakHitRequest|null;

	//待機レスポンスリスト
	private responseMap:Map<string, (body:any, error: any) => void>;

	public constructor(){
		this.onClose = () => {};
		this.onBreak = () => {};
		this.responseMap = new Map<string, ()=>void>();
		this.socketClient = null;
		this.breakInfo = null;
	}

	//ブレーク情報取得
	public GetBreakInfo(){
		return this.breakInfo;
	}

	//接続
	public Connect() {
		this.socketClient = Net.connect(27016, 'localhost', () => {
			console.log("connected to aosora");
		});

		//受信
		this.socketClient.on('data', (data => {
			let requestObj = null;
			try{
				const dataStr = data.toString();
				requestObj = JSON.parse(dataStr);
			}
			catch{
				console.log("json parse error");
			}
			this.Recv(requestObj);
		}));

		//終了
		this.socketClient.on('close', () => {
			console.log('client-> connection is closed');
			this.onClose();
		});
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

		//リクエスト送信
		this.socketClient?.write(JSON.stringify(request));
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
		console.log("break!");
		this.breakInfo = request;
		this.onBreak(this.breakInfo.filename, this.breakInfo.line);
	}

	//-- エディタ向けインターフェース

	//ブレークポイント設定（エディタ側都合でファイルごとに差し替えの形）

	public SetBreakPoints(filename: string, lines: number[]){
		return new Promise<void>((resolve, reject) => {
			const requestBody = {
				filename:  filename.replace("\\\\", "\\"),
				lines
			};
			this.Send('set_breakpoints', requestBody, (_, e) => {!e ? resolve() : reject();} );
		});
	}

	public RequestScope(scope: VariableScope) {
		return new Promise<VariableInformation[]>((resolve, reject) => {
			this.Send('request_variable_scope', scope, (response, error) => {
				if(error){
					reject();
				}

				const parsedVariables = VariableScopeResponse.safeParse(response);
				if(parsedVariables.success){
					resolve(parsedVariables.data.variables);
				}
				else {
					reject();
				}

			});
		});
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
	public Continue(){
		this.Send("continue", {});
	}

	public StepIn(){
		this.Send("stepin", {});
	}

	public StepOut(){
		this.Send("stepout", {});
	}

	public StepOver(){
		this.Send("stepover", {});
	}

	//切断
	public Disconnect(){
		this.socketClient?.end();
	}
}