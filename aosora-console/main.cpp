#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include "Shiori.h"
#include "Misc/Message.h"
#include "Misc/ProjectParser.h"

// 起動時データ
struct AppOptions {
	bool showVersion = false;
	bool showHelp = false;
	bool noSave = false;
	std::optional<std::string> eventId;
	std::vector<std::string> references;
};

// 引数をパース
static bool parseArgs(int argc, char* argv[], AppOptions& opts) {
	bool endOfOptions = false;
	bool hasPositional = false;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];

		if (!endOfOptions && arg == "--") {
			endOfOptions = true;
			continue;
		}

		if (!endOfOptions && arg.size() >= 2 && arg[0] == '-') {
			// 値を取るオプションはここに追加する
			// 例: if (arg == "--output") { if (++i >= argc) { ... } opts.output = argv[i]; continue; }

			if (arg == "--version") {
				opts.showVersion = true;
			}
			else if (arg == "--help") {
				opts.showHelp = true;
			}
			else if (arg == "--nosave") {
				opts.noSave = true;
			}
			else {
				std::cerr << "不明なオプション: " << arg << std::endl;
				return false;
			}
		}
		else {
			if (!hasPositional) {
				opts.eventId = arg;
				hasPositional = true;
			}
			else {
				opts.references.emplace_back(arg);
			}
		}
	}

	return true;
}

/**
* コンソール版 蒼空
*/
int main(int argc, char* argv[])
{
	//UTF-8を設定
	SetConsoleOutputCP(CP_UTF8);

	AppOptions opts;
	if (!parseArgs(argc, argv, opts)) {
		return 1;
	}

	if (opts.showVersion) {
		std::cout << ("Aosora " AOSORA_SHIORI_VERSION) << std::endl;
		return 0;
	}

	if (opts.showHelp) {
		std::cout <<
			"Usage: aosora [options] [EventId] [Reference...]\n"
			"\n"
			"Options:\n"
			"  --version   バージョンを表示して終了\n"
			"  --help      このヘルプを表示して終了\n"
			"  --          以降の引数をオプションとして解釈しない\n";
		return 0;
	}

	//SHIORI同様にaosoraを起動
	sakura::Shiori* aosoraShiori = new sakura::Shiori();
	aosoraShiori->SetEnableConsoleIO(true);
	aosoraShiori->Load(std::filesystem::current_path().string());

	//エラーがあればコンソール出力
	if (aosoraShiori->HasScriptLoadError()) {
		for (const auto& item : aosoraShiori->GetScriptLoadErrors()) {
			std::cerr << item.MakeConsoleErrorString() << std::endl;
		}
		return 1;
	}

	if (aosoraShiori->HasBootExecuteError()) {
		std::cerr << aosoraShiori->GetBootingExecuteErrorLog() << std::endl;
		return 1;
	}

	//リクエストを作成して実行
	sakura::ShioriRequest request;
	sakura::ShioriResponse response;

	request.SetSecurityLevel(sakura::SecurityLevel::LOCAL);
	if (opts.eventId.has_value()) {
		request.SetEventId(opts.eventId.value());
		for (int i = 0; i < static_cast<int>(opts.references.size()); ++i) {
			request.SetReference(i, opts.references[i]);
		}
		//TODO: opts.noSave を request に反映する
	}

	//イベントIDが渡らないなら初期実行までで打ち切りになる
	if (opts.eventId.has_value()) {
		aosoraShiori->Request(request, response);

		if (response.HasError()) {
			for (const auto& item : response.GetErrorCollection()) {
				std::cerr << item.GetErrorMessage() << std::endl;
			}
			return 1;
		}

		//戻り値があればコンソールに流す
		if (!response.GetValue().empty()) {
			std::cout << response.GetValue() << std::endl;
		}
	}

	//アンロード
	aosoraShiori->Unload();
	delete aosoraShiori;

	return 0;
}
