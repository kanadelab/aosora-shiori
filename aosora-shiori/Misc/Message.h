#pragma once
#include <string>
#include <map>
#include <assert.h>

namespace sakura {
	
	//テキスト多言語化系
	//単純にキーバリューで文字列を引き当てる仕組みとする
	//スクリプト周りを基本非シングルトンにしてないのでちょっと微妙だけどシングルトン実装にしてしまう
	class TextSystem {
	public:
		using TextMap = std::map<std::string, const char*>;

	private:
		static TextSystem* instance;
		std::map<std::string, TextMap> languages;

	public:

		static void CreateInstance()
		{
			instance = new TextSystem();
		}

		static void DestroyInstance()
		{
			delete instance;
			instance = nullptr;
		}

		static TextSystem* GetInstance()
		{
			return instance;
		}

	public:
		//テキストデータ設定
		TextSystem();

		//登録用のオブジェクト
		class RegisterContext {
		private:
			TextMap& target;

		public:
			RegisterContext(TextMap& textMap):
				target(textMap)
			{}

			//登録
			RegisterContext& Register(const std::string& key, const char* text) {
				target.insert(TextMap::value_type(key, text));
				return *this;
			}
		};

		//読み込み用のオブジェクト
		class ReadContext {
		private:
			TextMap& target;
			TextMap& defaultTarget;

		public:

			ReadContext(TextMap& textMap, TextMap& defaultMap):
				target(textMap),
				defaultTarget(defaultMap)
			{}

			//検索
			const char* Find(const std::string& key) {
				auto item = target.find(key);
				if (item == target.end()) {
					auto defaultItem = defaultTarget.find(key);
					if (defaultItem == defaultTarget.end()) {
						assert(false);
						return "(message_error)";
					}
					else {
						return defaultItem->second;
					}
				}
				else {
					return item->second;
				}
			}
		};

		//登録
		RegisterContext RegisterLanguage(const std::string& langCode) {
			languages.insert(std::map<std::string, TextMap>::value_type(langCode, TextMap()));
			return RegisterContext(languages[langCode]);
		}

		//取得
		static const char* Find(const std::string& key) {
			return ReadContext(instance->languages["ja-jp"], instance->languages["ja-jp"]).Find(key);
		}

	};
	
}