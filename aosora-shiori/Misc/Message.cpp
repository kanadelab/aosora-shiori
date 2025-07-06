#include "Misc/Message.h"

#define ERROR_MESSAGES(ERR_ID, MESSAGE, HINT)	.Register("ERROR_MESSAGE" ERR_ID, MESSAGE).Register("ERROR_HINT" ERR_ID, HINT)

namespace sakura {

	TextSystem* TextSystem::instance = nullptr;

	TextSystem::TextSystem() {

		//デフォルト
		primaryLanguage = "ja-jp";

		//ja-jp
		RegisterLanguage("ja-jp")

			ERROR_MESSAGES("A999", "内部エラー", "蒼空のバグの可能性のあるエラーが発生しました。作者に報告をお願いします。")
			ERROR_MESSAGES("A000", "構文エラー", "スクリプトとして解釈できない記述です")
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
			ERROR_MESSAGES("A040", "式が必要です。", "式や値を書く必要がありますが、書かずに進もうとしているようです。")

			ERROR_MESSAGES("T001", "括弧 { } の対応関係が正しくありません。", "対応する開き括弧のない中括弧があったため、読み込みに失敗しました。")
			ERROR_MESSAGES("T002", "トーク名として使用できない文字列です。", "変数名として使用できない文字をトーク名として使おうとしています。")
			ERROR_MESSAGES("T003", "開き括弧 ( が必要です。", "ifの条件式の前に開き括弧 ( が必要です。")
			ERROR_MESSAGES("T004", "開き括弧 { が必要です。", "トークブロック本文の開始前に中括弧 { が必要です。")
			ERROR_MESSAGES("T005", "構文的に認識できない文字です。", "スクリプトのどの構文にも当てはまらない文字です。書き方が間違ってないか確認してみてください。")

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
			.Register("AOSORA_BOOT_ERROR_6", "解決のヒント")

			.Register("AOSORA_RUNTIME_ERROR_0", "蒼空 実行エラー / aosora runtime error")
			.Register("AOSORA_RUNTIME_ERROR_1", "エラーが発生しため、実行を中断しました。")
			.Register("AOSORA_RUNTIME_ERROR_2", "エラー位置")
			.Register("AOSORA_RUNTIME_ERROR_3", "エラー内容")
			.Register("AOSORA_RUNTIME_ERROR_4", "スタックトレース")
			.Register("AOSORA_RUNTIME_ERROR_5", "蒼空 実行エラー")

			.Register("AOSORA_BALLOON_CLOSE", "閉じる")
			.Register("AOSORA_BALLOON_RELOAD", "ゴーストを再読み込み")
			.Register("AOSORA_RELOAD_DEBUGGER_HINT", "(デバッグツールからゴーストを再起動できます)")

			.Register("AOSORA_BUILTIN_ERROR_001", "1リクエスト内の実行処理数が制限を超えました。無限ループになってませんか？")
			.Register("AOSORA_BUILTIN_ERROR_002", "関数またはトークではないため、関数呼び出しができません。")

			.Register("AOSORA_DEBUGGER_001", "ゴーストに接続しました。")
			.Register("AOSORA_DEBUGGER_002", "ローカル変数")
			;


		//en-us
		RegisterLanguage("en-us")

			ERROR_MESSAGES("A001", "Closing brace } at the end of code block not found.", "A code block is a sequence of operations enclosed in {}, but there is no ending brace to go with the beginning brace. Check to make sure you did not misplace a code block or the closing brace for a nearby {")
			ERROR_MESSAGES("A002", "Operator is used incorrectly.", "This formula involves adding or multiplying two elements, but it seems that one of the two sides is missing.")
			ERROR_MESSAGES("A003", "The parenthesis don't seem to match up correctly.", "The parenthesis () used in the formula seem to have an invalid match. Please check the calculation formula.")
			ERROR_MESSAGES("A004", "An operator is required, or the operator cannot be used here.", "You cannot have two consecutive values. You need to make it an expression, such as using + to add them together.")
			ERROR_MESSAGES("A005", "A member name must be specified.", "A period is used to refer to a member of an object, as in Value.Item")
			ERROR_MESSAGES("A006", "A semicolon ; cannot be used here.", "You cannot use a semicolon ; in this expression. Please check that you are using the function correctly.")
			ERROR_MESSAGES("A007", "A colon : cannot be used here.", "You cannot use a colon : in this expression. Please check that you are using the function correctly.")
			ERROR_MESSAGES("A008", "A closing brace } cannot be used here.", "You cannot use a closing brace } in this expression. Please check that you are using the function correctly.")
			ERROR_MESSAGES("A009", "A closing bracket ] cannot be used here.", "You cannot use a closing bracket ] in this expression. Please check that you are using the function correctly.")
			ERROR_MESSAGES("A010", "A comma , cannot be used here.", "You cannot use a comma , in this expression. Please check that you are using the function correctly.")
			ERROR_MESSAGES("A011", "A closing parenthesis ) cannot be used here.", "You cannot use a closing parenthesis ) here. Please check that you are using the function correctly.")
			ERROR_MESSAGES("A012", "The parenthesis do not match correctly.", "It appears that the parenthesis at the beginning and end of the list do not match.")
			ERROR_MESSAGES("A013", "An argument name is required.", "Since this is an argument list, you must write the argument names.")
			ERROR_MESSAGES("A014", "The argument list is incorrect.", "You must use a comma , to separate arguments, or close the argument list with closing parenthesis )")
			ERROR_MESSAGES("A015", "The parenthesis do not match correctly.", "The argument list is not closed properly.")
			ERROR_MESSAGES("A016", "Key is required.", "Associative array expressions describe their contents in a key:value format. The key does not seem to be in the correct format.")
			ERROR_MESSAGES("A017", "Colon : is required.", "Associative array expressions describe their contents in a key:value format. There does not appear to be a colon separating the key and value.")
			ERROR_MESSAGES("A018", "Opening brace { is required.", "In a function expression, you must begin the body of the function with {")
			ERROR_MESSAGES("A019", "Opening brace { is required.", "In a talk expression, you must begin the body of the function with {")
			ERROR_MESSAGES("A020", "A function name is required.", "A function definition requires the word 'function' followed by a function name.")
			ERROR_MESSAGES("A021", "Opening parenthesis ( is required.", "When using a function occurence condition, write 'if' followed by the condition in parenthesis ()")
			ERROR_MESSAGES("A022", "Opening brace { is required.", "In a function definition, you must begin the body of the function with {")
			ERROR_MESSAGES("A023", "You are trying to write to a target that cannot be written to.", "An attempt is being made to change read-only information by assigning or otherwise modifying it.")
			ERROR_MESSAGES("A024", "The 'new' keyword requires a call expression ()", "The 'new' keyword requires a call expression, such as new Class()")
			ERROR_MESSAGES("A025", "Class name is required.", "In a class definition, the 'class' keyword must be followed by the class name.")
			ERROR_MESSAGES("A026", "Inherited class name is required.", "When inheriting a class, write a colon : and then the name of the class you want to inherit from.")
			ERROR_MESSAGES("A027", "Opening brace { is required.", "An opening brace { is required before the description of the class body.")
			ERROR_MESSAGES("A028", "Opening brace { is required.", "An opening brace { is required before the body of init.")
			ERROR_MESSAGES("A029", "Closing brace } is required.", "A brace } to close the class is required.")
			ERROR_MESSAGES("A030", "Variable name is required.", "A variable name is required here. The variable name may contain keywords or symbols that cannot be used.")
			ERROR_MESSAGES("A031", "Opening parenthesis ( is required.", "The opening parenthesis of the 'for' statement are required.")
			ERROR_MESSAGES("A032", "Opening parenthesis ( is required.", "The opening parenthesis of the 'while' statement are required.")
			ERROR_MESSAGES("A033", "Opening parenthesis ( is required.", "The opening parenthesis of the 'if' statement are required.")
			ERROR_MESSAGES("A034", "A semicolon ; is required.", "A semicolon is required at the end of the 'break' statement.")
			ERROR_MESSAGES("A035", "A semicolon ; is required.", "A semicolon is required at the end of the 'continue' statement.")
			ERROR_MESSAGES("A036", "Opening brace { is required.", "An opening brace { is required at the beginning of the 'try' block.")
			ERROR_MESSAGES("A037", "Opening brace { is required.", "An opening brace { is required at the beginning of the 'catch' block.")
			ERROR_MESSAGES("A038", "Opening brace { is required.", "An opening brace { is required at the beginning of the 'finally' block.")
			ERROR_MESSAGES("A039", "A semicolon ; is required.", "Variable declarations must end with a semicolon ;")
			ERROR_MESSAGES("A040", "A formula is required.", "You need to write an expression or value, you seem to be trying to proceed without writing one.")

			ERROR_MESSAGES("T001", "The braces {} do not match.", "Loading failed because there are closing braces without opening braces.")
			ERROR_MESSAGES("T002", "This string cannot be used as a talk name.", "You are trying to use characters that cannot be used as variable names in a talk name.")
			ERROR_MESSAGES("T003", "Opening parenthesis ( is required.", "The 'if' conditional expression must be preceded by an opening parenthesis (")
			ERROR_MESSAGES("T004", "Opening brace { is required.", "A brace { is required before the start of the talk block body.")
			ERROR_MESSAGES("T005", "These characters are not recognized syntax.", "This character does not fit any of the script syntax. Please check to see if you have written it correctly.")

			ERROR_MESSAGES("S001", "ghost.asproj was not found.", "")
			ERROR_MESSAGES("S002", "Could not open script file.", "")

			.Register("AOSORA_ERROR_RELOADED_0", "Aosora reload completed")
			.Register("AOSORA_ERROR_RELOADED_1", "Reloaded, no startup errors.")

			.Register("AOSORA_BOOT_ERROR_0", "Aosora boot error")
			.Register("AOSORA_BOOT_ERROR_1", "(Double click on the ghost to open it again)")
			.Register("AOSORA_BOOT_ERROR_2", "Aosora error detail view")
			.Register("AOSORA_BOOT_ERROR_3", "Return to error list")
			.Register("AOSORA_BOOT_ERROR_4", "Error position")
			.Register("AOSORA_BOOT_ERROR_5", "Error")
			.Register("AOSORA_BOOT_ERROR_6", "Solution hint")

			.Register("AOSORA_RUNTIME_ERROR_0", "Aosora runtime error")
			.Register("AOSORA_RUNTIME_ERROR_1", "Execution was aborted due to an error.")
			.Register("AOSORA_RUNTIME_ERROR_2", "Error position")
			.Register("AOSORA_RUNTIME_ERROR_3", "Error description")
			.Register("AOSORA_RUNTIME_ERROR_4", "Stack trace")
			.Register("AOSORA_RUNTIME_ERROR_5", "Aosora runtime error")

			.Register("AOSORA_BALLOON_CLOSE", "Close")
			.Register("AOSORA_BALLOON_RELOAD", "Reload ghost")

			.Register("AOSORA_BUILTIN_ERROR_001", "The number of processes performed in one request has exceeded the limit. Is there an infinite loop?")
			.Register("AOSORA_BUILTIN_ERROR_002", "Function call cannot be made because it is not a function or talk.")
			;

	}

}
