#include "SakuraFMOReader.h"
#include <map>
#include <sstream>

namespace sakura {


	bool ReadSakuraFMO(std::vector<FMORecord>& items)
	{
		const char* MutexName = "SakuraFMO";
		const char* FMOName = "Sakura";
		const std::string Delimiter = "\r\n";

		std::map<std::string, FMORecord> fmoMap;

		//Openのがいいかも
		HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, MutexName);
		DWORD err = GetLastError();

		//TODO: WAIT_OBJECT_0 出ない場合の扱いがきになる
		if (WaitForSingleObject(hMutex, INFINITE) != WAIT_FAILED) {
			HANDLE hObject = OpenFileMapping(FILE_MAP_READ, FALSE, FMOName);
			const void* fmoData = MapViewOfFile(hObject, FILE_MAP_READ, 0, 0, 0);

			MEMORY_BASIC_INFORMATION memInfo;
			if (VirtualQuery(fmoData, &memInfo, sizeof(memInfo)) != 0)
			{
				//先頭4バイトがfmoのサイズになっているのでその範囲で読み取る
				const uint32_t* size = static_cast<const uint32_t*>(fmoData);

				//読み終えるまで１行ずつ処理
				std::string_view fmoView(static_cast<const char*>(fmoData) + sizeof(uint32_t), *size - sizeof(uint32_t));

				while (true) {
					
					//レコード終端を示すCRLFを探す
					const size_t dIndex = fmoView.find(Delimiter);
					if (dIndex == std::string_view::npos) {
						break;
					}

					//バイト値0でも終了
					if (fmoView[0] == '\0') {
						break;
					}

					//1行とりだし
					const std::string_view record = fmoView.substr(0, dIndex);
					fmoView = fmoView.substr(dIndex + Delimiter.size());

					//バイト値1でkey-value分離
					const size_t sIndex = record.find("\1");
					const size_t pIndex = record.find(".");
					if (sIndex == std::string_view::npos || pIndex == std::string_view::npos
						|| pIndex > sIndex) {
						//レコード不適格
						continue;
					}

					const std::string id = std::string(record.substr(0, pIndex));
					const std::string_view key = record.substr(pIndex + 1, sIndex - (pIndex + 1));
					const std::string value = std::string(record.substr(sIndex + 1));
					printf("%s : %s\n", std::string(key).c_str(), std::string(value).c_str());

					//レコードなかったら作る
					if (!fmoMap.contains(id)) {
						fmoMap[id] = FMORecord(id);
					}

					//かきこみ
					if (key == "hwnd") {
						fmoMap[id].hWnd = reinterpret_cast<HWND>(std::stoull(value));
					}
					else if (key == "name") {
						fmoMap[id].sakuraName = value;
					}
					else if (key == "ghostpath") {
						fmoMap[id].ghostPath = value;
					}
					else if (key == "keroname") {
						fmoMap[id].keroName = value;
					}
					else if (key == "fullname") {
						fmoMap[id].ghostName = value;
					}
					else if (key == "path") {
						fmoMap[id].executablePath = value;
					}
				}
			}

			ReleaseMutex(hMutex);
		}
		else {
			return false;
		}

		CloseHandle(hMutex);

		//出力
		for (auto item : fmoMap) {
			items.push_back(item.second);
		}

		return true;
	}


	//DirectSSTPの送信
	void SendDirectSSTP(const std::string& script, const FMORecord& target) {
		std::ostringstream ost;
		ost << "SEND SSTP/1.4\r\nCharset: UTF-8\r\nSender: Aosora SHIORI\r\nScript: " << script << "\\e\r\n\r\n";
		std::string str = ost.str();

		//ゴーストにデータを送信する
		COPYDATASTRUCT cds = {};
		cds.dwData = 9801;	//Direct SSTP
		cds.cbData = str.size();
		cds.lpData = &str[0];

		SendMessageTimeoutA(target.hWnd, WM_COPYDATA, 0, (LPARAM)&cds, SMTO_ABORTIFHUNG, 5000, NULL);
	}
};

