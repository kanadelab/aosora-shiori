#include <vector>
#include "Debugger/DebuggerUtility.h"
#include "string_view"

namespace sakura {

	//受信データバッファ
	//通信プロトコルに従ってデータをバッファリング＆切り出しを行う
	//ただ、すべてのリクエストを送りっぱなしではなくちゃんとレスポンスを返すようにするなら不要そうな気もする。
	class ReceivedDataBuffer {
	private:
		std::vector<uint8_t> internalBuffer;

	public:

		//受信データの投入
		void AddReceivedData(const uint8_t* data, size_t size)
		{
			//末尾にデータを挿入
			size_t currentSize = internalBuffer.size();
			internalBuffer.resize(currentSize + size);
			memcpy(&internalBuffer[currentSize], data, size);
		}

		//受信データの取得
		bool ReadReceivedString(std::string_view& receivedString)
		{
			//区切りの0バイトを検索
			void* addr = std::memchr(&internalBuffer[0], 0, internalBuffer.size());
			if (addr != nullptr) {
				//範囲を切り出す
				const size_t size = reinterpret_cast<const uint8_t*>(addr) - &internalBuffer[0];
				receivedString = std::string_view(reinterpret_cast<const char*>(&internalBuffer[0]), size);
				return true;
			}
			else {
				return false;
			}
		}

		//受信データの削除
		//ReadReceivedStringで受信した string_view を返却する形でバッファをシフトします
		void RemoveReceivedData(const std::string_view& receivedString)
		{
			//すべての領域を削除する場合は同じベクタをバッファとして使う
			if (receivedString.size() + 1 == internalBuffer.size()) {
				internalBuffer.clear();
				return;
			}

			//あたらしい領域をとりなおしてシフト
			std::vector<uint8_t> newBuffer;
			newBuffer.resize(internalBuffer.size() - (receivedString.size() + 1));
			memcpy(&newBuffer[0], &internalBuffer[receivedString.size() + 1], newBuffer.size());
			internalBuffer = std::move(newBuffer);
		}
	};
}