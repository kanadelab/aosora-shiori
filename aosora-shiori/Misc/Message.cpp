#include "Misc/Message.h"

#define ERROR_MESSAGES(ERR_ID, MESSAGE, HINT)	.Register("ERROR_MESSAGE" ERR_ID, MESSAGE).Register("ERROR_HINT" ERR_ID, HINT)

namespace sakura {

	TextSystem* TextSystem::instance = nullptr;

	TextSystem::TextSystem() {

		//にほんご
		RegisterLanguage("ja-jp")

			ERROR_MESSAGES("A001", "コードブロック終了の閉じ括弧 } が見つかりませんでした。", "コードブロックは { } で囲まれる一連の処理ですが、始まりに対して終わりが見つからないエラーです。コードブロックや、付近の { の閉じ括弧を忘れていないか確認してください。")
			ERROR_MESSAGES("A002", "演算子の使い方が正しくありません。", "2つの要素を足したり掛けたりするタイプの計算式で、足す側と足される側といったような２つの要素が揃っていないようです。")
			ERROR_MESSAGES("A003", "カッコの対応関係が間違っています。", "計算式に使うカッコ ( ) の対応関係が正しくないようです。計算式を確認してください。")
			ERROR_MESSAGES("A004", "演算子が必要か、ここでは使えない演算子です。", "値を2つ連続することはできません。足し合わせるなら + を使うなど、式にする必要があります。")
			ERROR_MESSAGES("A005", "メンバ名を指定する必要があります。", "Value.Item のように、ピリオドはオブジェクトのメンバーを参照するために使用します。")
			ERROR_MESSAGES("A006", "ここでセミコロン ; は使えません。", "この式ではセミコロン ; を使うことができません。機能の使い方が間違ってないか、確認してみてください。")
			ERROR_MESSAGES("A007", "ここでコロン : は使えません。", "この式ではコロン : を使うことができません。機能の使い方が間違ってないか、確認してみてください。")
			ERROR_MESSAGES("A008", "ここで閉じ括弧 } は使えません。", "この式では閉じ括弧 } を使うことができません。機能の使い方が間違ってないか、確認してみてください。")
			ERROR_MESSAGES("A009", "ここで閉じ括弧 ] は使えません。", "この式では閉じ括弧 ] を使うことができません。機能の使い方が間違ってないか、確認してみてください。")
			ERROR_MESSAGES("A010", "ここでカンマ , は使えません。", "この式ではカンマ , を使うことができません。機能の使い方が間違ってないか、確認してみてください。")
			ERROR_MESSAGES("A011", "ここで閉じ括弧 ) は使えません。", "ここでは閉じ括弧 ) を使うことができません。機能の使い方が間違ってないか、確認してみてください。")
			ERROR_MESSAGES("A012", "カッコの対応関係が正しくありません。", "リストの始端終端のカッコが揃っていないようです。")
			ERROR_MESSAGES("A013", "引数名が必要です。", "ここは引数リストなので、引数名を書かないといけません")
			ERROR_MESSAGES("A014", "引数リストが正しくありません。", "カンマ , で引数を区切るか、閉じ括弧 ) で引数リストを閉じる必要があります。")
			ERROR_MESSAGES("A015", "カッコの対応関係が正しくありません。", "引数リストが正しく閉じられていません。")
			ERROR_MESSAGES("A016", "キーが必要です。", "連想配列の式では キー: 値　の形式で内容を記述します。キーが正しい形式ではないようです。")
			ERROR_MESSAGES("A017", "コロン : が必要です。", "連想配列の式では キー: 値 の形式で内容を記述します。キーと値を分けるコロンが無いようです。")
			ERROR_MESSAGES("A018", "開き括弧 { が必要です。", "関数式では { から関数本体を始める必要があります。")
			ERROR_MESSAGES("A019", "開き括弧 { が必要です。", "トーク式では { からトーク本体を始める必要があります。")
			ERROR_MESSAGES("A020", "関数名が必要です。", "関数定義ではfunction に続けて関数名が必要です。")
			ERROR_MESSAGES("A021", "開き括弧 ( が必要です。", "関数定義の発生条件を使用する場合は if に続けて括弧 ( ) で条件を記述します。")
			ERROR_MESSAGES("A022", "開き括弧 { が必要です。", "関数定義では { から関数本体を始める必要があります。")
			ERROR_MESSAGES("A023", "書き込めない対象に書き込もうとしています。", "読み取り専用の情報に代入等で変更をしようとしています。")
			ERROR_MESSAGES("A024", "newキーワードには呼出式 ( ) が必要です。", "new Class() のように、newキーワードには呼出式が必要です。")
			ERROR_MESSAGES("A025", "クラス名が必要です。", "クラス定義では class キーワードに続いてクラス名を記述します。")
			ERROR_MESSAGES("A026", "継承クラス名が必要です。", "クラスを継承する場合はクラス名の後のコロン : に続けて継承元のクラス名を記述します。")
			ERROR_MESSAGES("A027", "開き括弧 { が必要です。", "クラス本体の記述前に始端の括弧 { が必要です。")
			ERROR_MESSAGES("A028", "開き括弧 { が必要です。", "initの本体の前に始端の括弧 { が必要です。")
			ERROR_MESSAGES("A029", "閉じ括弧 } が必要です。", "クラス終端の括弧 } が必要です。")
			ERROR_MESSAGES("A030", "変数名が必要です。", "ここでは変数名が必要です。変数名に使用できないキーワードか記号が使われているかもしれません。")
			ERROR_MESSAGES("A031", "開き括弧 ( が必要です。", "for文の開き括弧が必要です。")
			ERROR_MESSAGES("A032", "開き括弧 ( が必要です。", "while文の開き括弧が必要です。")
			ERROR_MESSAGES("A033", "開き括弧 ( が必要です。", "if文の開き括弧が必要です。")
			ERROR_MESSAGES("A034", "セミコロン ; が必要です。", "break文の終わりにはセミコロンが必要です。")
			ERROR_MESSAGES("A035", "セミコロン ; が必要です。", "continue文の終わりにはセミコロンが必要です。")
			ERROR_MESSAGES("A036", "開き括弧 { が必要です。", "tryブロック始端には開き括弧 { が必要です。")
			ERROR_MESSAGES("A037", "開き括弧 { が必要です。", "catchブロック始端には開き括弧 { が必要です。")
			ERROR_MESSAGES("A038", "開き括弧 { が必要です。", "finallyブロック始端には開き括弧 { が必要です。")
			ERROR_MESSAGES("A039", "セミコロン ; が必要です。", "変数宣言の終わりにはセミコロン ; が必要です。")

			ERROR_MESSAGES("T001", "括弧 { } の対応関係が正しくありません。", "対応する開き括弧のない中括弧があったため、読み込みに失敗しました。")
			ERROR_MESSAGES("T002", "トーク名として使用できない文字列です。", "変数名として使用できない文字をトーク名として使おうとしています。")
			ERROR_MESSAGES("T003", "開き括弧 ( が必要です。", "ifの条件式の前に開き括弧 ( が必要です。")
			ERROR_MESSAGES("T004", "開き括弧 { が必要です。", "トークブロック本文の開始前に中括弧 { が必要です。")
			ERROR_MESSAGES("T005", "構文的に識できない文字です。", "スクリプトのどの構文にも当てはまらない文字です。書き方が間違ってないか確認してみてください。")

			ERROR_MESSAGES("S001", "設定ファイル ghost.asproj が開けませんでした。", "")
			ERROR_MESSAGES("S002", "スクリプトファイルが開けませんでした。", "")

			.Register("AOSORA_ERROR_RELOADED_0", "蒼空 リロード完了")
			.Register("AOSORA_ERROR_RELOADED_1", "リロードして、起動エラーはありませんでした。")

			.Register("AOSORA_BOOT_ERROR_0", "蒼空 起動エラー / Aosora boot error")
			.Register("AOSORA_BOOT_ERROR_1", "(ゴーストをダブルクリックで再度開けます)")
			.Register("AOSORA_BOOT_ERROR_2", "蒼空 エラー詳細ビュー")
			.Register("AOSORA_BOOT_ERROR_3", "エラーリストに戻る")
			.Register("AOSORA_BOOT_ERROR_4", "エラー位置")
			.Register("AOSORA_BOOT_ERROR_5", "エラー")
			.Register("AOSORA_BOOT_ERROR_5", "解決のヒント")

			.Register("AOSORA_RUNTIME_ERROR_0", "蒼空 実行エラー / aosora runtime error")
			.Register("AOSORA_RUNTIME_ERROR_1", "エラーが発生しため、実行を中断しました。")
			.Register("AOSORA_RUNTIME_ERROR_2", "エラー位置")
			.Register("AOSORA_RUNTIME_ERROR_3", "エラー内容")
			.Register("AOSORA_RUNTIME_ERROR_4", "スタックトレース")
			.Register("AOSORA_RUNTIME_ERROR_5", "蒼空 実行エラー")

			.Register("AOSORA_BALLOON_CLOSE", "閉じる")
			.Register("AOSORA_BALLOON_RELOAD", "ゴーストを再読み込み")
			;

	}

}