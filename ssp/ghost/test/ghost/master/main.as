

//デフォルトセーブデータ
function OnDefaultSaveData {
	Save.Data.UserName = "おにいちゃん";
	Save.Data.TalkInterval = 180;	
}

//SHIORIロード後
function OnAosoraLoad {
	//ランダムトークの設定
	TalkTimer.RandomTalk = Reflection.Get("ランダムトーク");
	TalkTimer.RandomTalkIntervalSeconds = Save.Data.TalkInterval;

	//なでられトークの設定
	TalkTimer.NadenadeTalk = Reflection.Get("OnNadenade");
}

//喋り間隔の設定
function SetTalkInterval(intervalSeconds){
	//間隔を設定して、待ち時間をリセット
	TalkTimer.RandomTalkIntervalSeconds = intervalSeconds;
	TalkTimer.RandomTalkElapsedSeconds = 0;
	Save.Data.TalkInterval = intervalSeconds;
}

//選択肢
function OnChoiceSelect{
	return Reflection.Get(Shiori.Reference[0]);
}

talk OnBoot {
	>めりくり: Time.GetNowMonth() == 12 && Time.GetNowDate() == 24
	>起動
}

function OnClose() {
	//少しウェイトをいれてやる
	return 終了時トーク() + "\w9\w9\w9";
}

talk 終了時トーク {
	>終了_宿泊: Time.GetNowHour() >= 19
	>終了_就寝: Time.GetNowHour() >= 22 || Time.GetNowHour() <= 4
	>終了
}

function 泊まりの時間 {
	if(Time.GetNowHour() >= 19 || Time.GetNowHour() <= 4)
		return false;
	return true;
}

//通常の表情にもどす
talk OnSurfaceRestore {
	\s[0]
}

/*
	触り反応
*/
local collisions = {
	head: "頭",
	bust: "胸"
};
		
talk OnMouseDoubleClick {
	%{
		local colName = collisions[Shiori.Reference[4]];
	}
	>MainMenu : Shiori.Reference[4] == ""
	>Reflection.Get(colName + "つつかれ")
}

talk OnNadenade {
	%{
		local colName = collisions[Shiori.Reference[4]];
	}
	>Reflection.Get(colName + "なでられ")
}

/*
	基本単語
*/
function ユーザ名 {
	return Save.Data.UserName;
}

function 一人称 {
	return "妹の私";
}