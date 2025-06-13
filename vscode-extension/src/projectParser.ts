import * as fs from "fs/promises";

export class ProjectParser {

	public runtimePath:string|null;	//ランタイム(SSP)パス
	public enableDebug:boolean;

	public constructor(){
		this.runtimePath = null;
		this.enableDebug = false;
	}

	public async Parse(filename:string){

		try{
			//Aosoraプロジェクトデータを解析
			const projectFile = await fs.readFile(filename, {encoding:"utf-8"});
			const lines = projectFile.split("\n");
			for(const line of lines){
				//改行と空白を除去してカンマで分離
				const items = line.replace("\t", "").replace("\r", "").replace(" ", "").split(",");
				if(items.length !== 2){
					continue;
				}

				if(items[0] === 'debug.debugger.runtime'){
					//ランタイム指定
					this.runtimePath = items[1];
				}
				else if(items[0] === 'debug'){
					this.enableDebug = this.SettingsToBool(items[1]);
				}
			}
		}
		catch {
			
		}
	}

	private SettingsToBool(s:string){
		if(s === '0'){
			return false;
		}
		else if(s.toLowerCase() === "false"){
			return false;
		}
		return true;
	}
}


