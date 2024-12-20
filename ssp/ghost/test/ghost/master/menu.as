/*
	メニュー関係。
*/

talk MainMenu {
	\s[4]なーあにっと。
	
	\![*]\q[何か喋って,ランダムトーク]
	\![*]\q[喋り間隔変更,OnChagneTalkInterval]
	\![*]\q[呼ばれ方を変える,OnChangeUserName]
	\![*]\q[かいものリスト,OnItemList]
	
	\![*]\q[なんでもありません,OnMenuClose]
}

talk OnMenuClose {
	\s[2]そう…
}

talk OnMenuClose {
	\s[0]とじるよ。
}

talk OnMenuClose {
	\s[0]うい。
}

/*
	喋り間隔の設定
*/

talk OnChagneTalkInterval {
	\s[0]どうする？\_q
	{TalkIntervalItem(120, "２分")}
	{TalkIntervalItem(180, "３分")}
	{TalkIntervalItem(240, "４分")}
	{TalkIntervalItem(0, "喋らない")}
}

function TalkIntervalItem(seconds, label) {
	local item = "\![*]\q[{label},OnSetTalkInterval,{seconds}]";
	if(seconds == Save.Data.TalkInterval){
		item = item + "← いまこれ！";
	}
	return item;
}

function OnSetTalkInterval {
	local interval = Shiori.Reference[0];
	SetTalkInterval(interval);
	return interval + OnChagneTalkInterval();
}

/*
	ユーザ名の変更
*/

talk OnChangeUserName {
	\s[0]え。\w9\w9
	ずっと{ユーザ名}だったのに？\w9\w9
	まあもうお互いに大人だし？ \w9\w9なんて呼べばいいの？\![open,inputbox,OnUserNameInput,0,{ユーザ名}]
}

function OnUserNameInput {
	local name = Shiori.Reference[0];
	if(!name) {
		return ユーザ名が空打ちされた();
	}
	else if(name == Save.Data.UserName) {
		return ユーザ名変更なし();
	}
	Save.Data.UserName = name;
	return ユーザ名変更完了();
}

talk ユーザ名が空打ちされた {
	やめとくか。
}

talk ユーザ名変更完了 {
	{ユーザ名}。\w9\w9うい。\w9\w9頑張ってそう呼ぶね。
}

talk ユーザ名変更なし {
	やっぱり{ユーザ名}も慣れてんだよ。\w9\w9
	{一人称}に{ユーザ名}って呼ばれるのがさー。\w9\w9
	だからこれでいいじゃん。
}

/*
	かいものリスト
	（技術デモ的な）
*/

//かいものリスト
local itemListCount = {};

talk OnItemList {
	\_q\s[0]何を買うんだっけ？

	>かいものリスト表示
}

function かいものリスト表示 {

	local itemList = [
		"おにく",
		"おさかな",
		"おやさい",
		"くだもの",
		"きのこ",
		"とうふ"
	];

	local result = "";
	for(local i = 0; i < itemList.length; i++){
		local itemName = itemList[i];
		local count = itemListCount[itemName];
		if(!count){
			count = 0;
		}
		result += "\q[▲増やす,OnIncrementItem,{itemName}] \q[▼減らす,OnDecrementItem,{itemName}] {itemName}: {count}個\n";
	}

	result += "\n\![*]\q[とじる,OnMenuClose]";
	return result;
}

function OnIncrementItem {
	//ふやす
	local itemName = Shiori.Reference[0];
	if(!itemListCount[itemName]){
		itemListCount[itemName] = 1; 
	}
	else {
		itemListCount[itemName] ++;
	}
	
	//かいものリスト
	return OnItemList();
}

function OnDecrementItem {
	//へらす
	local itemName = Shiori.Reference[0];
	if(itemListCount[itemName]){
		itemListCount[itemName] --;
	}

	//かいものリスト
	return OnItemList();
}