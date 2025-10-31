/*
	うかフィード解析
*/

unit ukafeed;
use std.*;

//よみこんだうかフィード情報
local ukafeedData = {};

function RequestUkafeed {
	return "うかフィードを読むよ。\![execute,http-get,https://feed.ukagaka.net/json,--async=OnUkafeedLoaded]";
}

function OnUkafeedLoaded {
	//うかフィードの読み込み結果を取得
	try {
		local filePath = Shiori.Reference[3];
		local json = File.ReadAllText(filePath);
		ukafeedData = JsonSerializer.Deserialize(json);
		if(!ukafeedData){
			throw new Error();
		}
		return OnUkafeedList();
	}
	catch{
		return OnUkafeedLoadedFailed();
	}
}

talk OnUkafeedLoadedFailed {
	\s[0]うかフィードの読み込みに失敗しました。
}

function OnUkafeedList {
	local result = "";
	for(local i = 0; i < ukafeedData.items.length; i++){
		local item = ukafeedData.items[i];
		result += "\![*]\__q[OnUkafeedItemDetail,{i}]\f[bold,1][{item.tags[0].EscapeSakuraScript()}]\f[bold,default]{item.title.EscapeSakuraScript()}\__q\n";
	}

	return "\0\s[0]\b[2]\![quicksession,true]うかフィード\n\n{result}\n\n\![*]\q[とじる,OnMenuClose]";
}

talk OnUkafeedItemDetail {
	%{
		local item = ukafeedData.items[0];
	}
	\s[0]\b[2]\![quicksession,true]\f[bold,1][{item.tags[0].EscapeSakuraScript()}]\f[bold,default]{item.title.EscapeSakuraScript()}

	{item.content_text.EscapeSakuraScript()}

	\![*]\__q[{item.url.EscapeSakuraScript()}]{item.url.EscapeSakuraScript()}\__q

	\![*]\q[もどる,OnUkafeedList]
	\![*]\q[とじる,OnMenuClose]
}

//mainユニットにエクスポート
unit.main.OnUkafeedLoaded = OnUkafeedLoaded;
unit.main.OnUkafeedLoadedFailed = OnUkafeedLoadedFailed;
unit.main.OnUkafeedList = OnUkafeedList;
unit.main.OnUkafeedItemDetail = OnUkafeedItemDetail;