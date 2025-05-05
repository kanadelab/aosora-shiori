#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace sakura {

	//FMOレコード
	class FMORecord {
	public:
		std::string id;
		std::string sakuraName;
		std::string keroName;
		std::string ghostName;
		std::string ghostPath;
		std::string executablePath;
		HWND hWnd;

		FMORecord(const std::string& recordId) :
			id(recordId),
			hWnd(NULL) {}

		FMORecord() :
			hWnd(NULL) {}
	};

	//FMOの読み込み
	bool ReadSakuraFMO(std::vector<FMORecord>& items);

	//スクリプトの送信
	void SendDirectSSTP(const std::string& script, const FMORecord& target);

}