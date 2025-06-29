#pragma once

#include "Base.h"

#if defined(AOSORA_REQUIRED_WIN32)
#include <windows.h>
#else
#endif // AOSORA_REQUIRED_WIN32
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
#if defined(AOSORA_REQUIRED_WIN32)
		HWND hWnd;
#endif // AOSORA_REQUIRED_WIN32

		FMORecord(const std::string& recordId) :
#if defined(AOSORA_REQUIRED_WIN32)
			id(recordId),
			hWnd(NULL) {}
#else
			id(recordId) {}
#endif // AOSORA_REQUIRED_WIN32

#if defined(AOSORA_REQUIRED_WIN32)
		FMORecord() :
			hWnd(NULL) {}
#else
		FMORecord() {}
#endif // AOSORA_REQUIRED_WIN32
	};

	//FMOの読み込み
	bool ReadSakuraFMO(std::vector<FMORecord>& items);

	//スクリプトの送信
	void SendDirectSSTP(const std::string& script, const FMORecord& target);

}
