#include "SakuraFMOReader.h"
#include <map>
#include <sstream>

#if !defined(AOSORA_REQUIRED_WIN32)
#include <climits>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

struct shm_t {
	uint32_t size;
	sem_t sem;
	char buf[PATH_MAX];
};

const int BUFFER_SIZE = 1024;

#endif // not AOSORA_REQUIRED_WIN32

namespace sakura {


	bool ReadSakuraFMO(std::vector<FMORecord>& items)
	{
#if defined(AOSORA_REQUIRED_WIN32)
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
#else
		shm_t *shm;
		int fd = shm_open("/ninix", O_RDWR, 0);
		if (fd == -1) {
			return false;
		}
		shm = static_cast<shm_t *>(mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
		close(fd);
		if (shm == MAP_FAILED) {
			return false;
		}
		if (sem_wait(&shm->sem) == -1) {
			return false;
		}
		std::string path(shm->buf, shm->size);
		if (sem_post(&shm->sem) == -1) {
			return false;
		}
		path.append("ninix");
		sockaddr_un addr;
		if (path.length() >= sizeof(addr.sun_path)) {
			return false;
		}
		int soc = socket(AF_UNIX, SOCK_STREAM, 0);
		if (soc == -1) {
			return false;
		}
		memset(&addr, 0, sizeof(sockaddr_un));
		addr.sun_family = AF_UNIX;
		// null-terminatedも書き込ませる
		strncpy(addr.sun_path, path.c_str(), path.length() + 1);
		if (connect(soc, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1) {
			return false;
		}
		char buffer[BUFFER_SIZE] = {};
		if (read(soc, buffer, sizeof(uint32_t)) != sizeof(uint32_t)) {
			close(soc);
			return false;
		}
		uint32_t remain = *reinterpret_cast<uint32_t *>(buffer);
		std::string data(buffer, sizeof(uint32_t));
		while (true) {
			int ret = read(soc, buffer, BUFFER_SIZE);
			if (ret == -1) {
				close(soc);
				return false;
			}
			if (ret == 0) {
				close(soc);
				break;
			}
			if (remain > ret) {
				data.append(buffer, ret);
			}
			else {
				data.append(buffer, remain);
			}
			remain -= ret;
		}
		const std::string Delimiter = "\r\n";
		std::map<std::string, FMORecord> fmoMap;
		const void *fmoData = reinterpret_cast<const void *>(data.c_str());
#endif // AOSORA_REQUIRED_WIN32
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
					//printf("%s : %s\n", std::string(key).c_str(), std::string(value).c_str());

					//レコードなかったら作る
					if (!fmoMap.contains(id)) {
						fmoMap[id] = FMORecord(id);
					}

					//かきこみ
					if (key == "hwnd") {
#if defined(AOSORA_REQUIRED_WIN32)
						fmoMap[id].hWnd = reinterpret_cast<HWND>(std::stoull(value));
#endif // AOSORA_REQUIRED_WIN32
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
#if defined(AOSORA_REQUIRED_WIN32)
			}

			ReleaseMutex(hMutex);
		}
		else {
			return false;
		}

		CloseHandle(hMutex);

#endif // AOSORA_REQUIRED_WIN32

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

#if defined(AOSORA_REQUIRED_WIN32)
		//ゴーストにデータを送信する
		COPYDATASTRUCT cds = {};
		cds.dwData = 9801;	//Direct SSTP
		cds.cbData = str.size();
		cds.lpData = &str[0];

		SendMessageTimeoutA(target.hWnd, WM_COPYDATA, 0, (LPARAM)&cds, SMTO_ABORTIFHUNG, 5000, NULL);
#else
		shm_t *shm;
		int fd = shm_open("/ninix", O_RDWR, 0);
		if (fd == -1) {
			return;
		}
		shm = static_cast<shm_t *>(mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
		close(fd);
		if (shm == MAP_FAILED) {
			return;
		}
		if (sem_wait(&shm->sem) == -1) {
			return;
		}
		std::string path(shm->buf, shm->size);
		if (sem_post(&shm->sem) == -1) {
			return;
		}
		path.append(target.id);
		sockaddr_un addr;
		if (path.length() >= sizeof(addr.sun_path)) {
			return;
		}
		int soc = socket(AF_UNIX, SOCK_STREAM, 0);
		if (soc == -1) {
			return;
		}
		memset(&addr, 0, sizeof(sockaddr_un));
		addr.sun_family = AF_UNIX;
		// null-terminatedも書き込ませる
		strncpy(addr.sun_path, path.c_str(), path.length() + 1);
		if (connect(soc, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1) {
			return;
		}
		if (write(soc, str.c_str(), str.length()) == -1) {
			close(soc);
			return;
		}
		char buffer[BUFFER_SIZE] = {};
		std::string data;
		while (true) {
			int ret = read(soc, buffer, BUFFER_SIZE);
			if (ret == -1) {
				close(soc);
				return;
			}
			if (ret == 0) {
				close(soc);
				break;
			}
			data.append(buffer, ret);
		}
#endif // AOSORA_REQUIRED_WIN32
	}
};

