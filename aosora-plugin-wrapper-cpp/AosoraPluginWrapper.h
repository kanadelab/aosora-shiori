#pragma once
#include "AosoraPlugin.h"

namespace aosora {
	class AosoraAccessor;

	class ValueWrapper {
		friend class AosoraAccessor;
	private:
		const raw::AosoraRawAccessor* accessor;
		raw::ValueHandle handle;

	public:
		ValueWrapper(const raw::AosoraRawAccessor* accessor, raw::ValueHandle handle):
			accessor(accessor),
			handle(handle)
		{ }



	};

	class AosoraAccessor {
	private:
		const raw::AosoraRawAccessor* rawAccessor;

	private:
		raw::StringContainer ToStringContainer(const std::string& str) {
			if (str.empty()) {
				return { nullptr, 0 };
			}
			return { str.c_str(), str.size() };
		}

	public:
		AosoraAccessor(raw::AosoraRawAccessor* rawAccessor):
			rawAccessor(rawAccessor)
		{ }

		// aosoraの数値型を作成
		ValueWrapper CreateNumber(double value) {
			return ValueWrapper(rawAccessor, rawAccessor->CreateNumber(value));
		}

		// aosoraのbool型を作成
		ValueWrapper CreateBool(bool value) {
			return ValueWrapper(rawAccessor, rawAccessor->CreateBool(value));
		}

		// aosoraの文字列型を作成
		ValueWrapper CreateString(const std::string& str) {
			return ValueWrapper(rawAccessor, rawAccessor->CreateString(ToStringContainer(str)));
		}

		// aosoraのnullを作成
		ValueWrapper CreateNull() {
			return ValueWrapper(rawAccessor, rawAccessor->CreateNull());
		}

		// aosoraの連想配列を作成
		ValueWrapper CreateMap() {
			return ValueWrapper(rawAccessor, rawAccessor->CreateMap());
		}

		// aosoraの配列を作成
		ValueWrapper CreateArray() {
			return ValueWrapper(rawAccessor, rawAccessor->CreateArray());
		}

		// aosoraの関数を作成
		ValueWrapper CreateFunction() {

		}
	};

}