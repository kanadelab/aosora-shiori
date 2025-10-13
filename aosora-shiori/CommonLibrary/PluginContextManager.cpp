#include <vector>
#include "CommonLibrary/PluginContextManager.h"
#include "CoreLibrary/CoreClasses.h"
#include "CommonLibrary/CommonClasses.h"
#include "Misc/PluginLoader.h"

namespace sakura {

	std::vector<PluginContext*> PluginContextManager::contextStack;
	std::map<LoadedPluginModule*, PluginHandleManager*> PluginContextManager::plugins;

	namespace {
		const aosora::StringContainer STRING_CONTAINER_EMPTY = { "", 0 };
	}

	//NOTE: ここズレるとおかしくなるので注意
	const aosora::AosoraAccessor PluginContextManager::accessor = {
		ReleaseHandle,
		AddRefHandle,

		CreateNumber,
		CreateBool,
		CreateString,
		CreateNull,
		CreateMap,
		CreateArray,
		CreateFunction,
		CreateMemoryBuffer,

		ToNumber,
		ToBool,
		ToString,
		ToMemoryBuffer,

		GetValueType,
		GetObjectTypeId,
		GetClassObjectTypeId,
		ObjectInstanceOf,
		IsCallable,
		
		GetValue,
		SetValue,

		GetArgumentCount,
		GetArgument,

		SetReturnValue,
		SetError,
		SetPluginError,

		FunctionCall,
		NewClassInstance,

		GetLastReturnValue,
		HasLastError,
		GetLastError,
		GetLastErrorMessage,
		GetLastErrorCode,

		GetErrorMessage,
		GetErrorCode,

		FindUnit,
		CreateUnit,

		MapGetLength,
		MapContains,
		MapClear,
		MapRemove,
		MapGetKeys,
		MapGetValue,
		MapSetValue,

		ArrayClear,
		ArrayAdd,
		ArrayAddRange,
		ArrayInsert,
		ArrayRemove,
		ArrayGetLength,
		ArrayGetValue,
		ArraySetValue,

		{},	//SPACE0

		static_cast<uint32_t>(ScriptValueType::Null),
		static_cast<uint32_t>(ScriptValueType::Number),
		static_cast<uint32_t>(ScriptValueType::Boolean),
		static_cast<uint32_t>(ScriptValueType::String),
		static_cast<uint32_t>(ScriptValueType::Object),

		ScriptArray::TypeId(),
		ScriptObject::TypeId(),
		MemoryBuffer::TypeId(),
		ClassData::TypeId(),
		ScriptError::TypeId(),
		
		{}	//SPACE1
	};

	void PluginHandleManager::FetchReferencedItems(std::list<CollectableBase*>& result) {
		for (auto it : valueMap) {
			if (it.second.valueRef != nullptr && it.second.valueRef->IsObject()) {
				result.push_back(it.second.valueRef->GetObjectRef().Get());
			}
		}
	}

	void PluginContextManager::FetchReferencedItems(LoadedPluginModule* pluginModule, std::list<CollectableBase*>& result) {
		auto it = plugins.find(pluginModule);
		if (it != plugins.end()) {
			it->second->FetchReferencedItems(result);
		}
	}

	//プラグインロード関数の実行
	ScriptValueRef PluginContextManager::ExecuteModuleLoadFunction(LoadedPluginModule& pluginModule, ScriptExecuteContext& executeContext) {

		//プラグイン用の情報領域を作成
		RegisterPlugin(&pluginModule);

		//コンテキストをプッシュ、ポップを行う
		PluginContext* pluginContext = new PluginContext(executeContext, pluginModule);
		PushContext(pluginContext);

		//ここでプラグイン実行
		pluginModule.fLoad(&accessor);

		//戻り値をフェッチ
		ScriptValueRef returnValue = PeekContext().GetCallContext().returnValue;

		//ポップ
		PopContext();

		if (returnValue != nullptr) {
			return returnValue;
		}
		return ScriptValue::Null;
	}

	//プラグイン関数の一般実行
	void PluginContextManager::ExecutePluginFunction(LoadedPluginModule& pluginModule, aosora::PluginFunctionType pluginFunction, const ScriptValueRef& thisValue, const FunctionRequest& request, FunctionResponse& response) {

		//コンテキストを準備
		PluginContext* pluginContext = new PluginContext(request.GetContext(), pluginModule);
		PushContext(pluginContext);

		//引数をプッシュ
		PeekContext().GetCallContext().thisValue = thisValue;
		for (size_t i = 0; i < request.GetArgumentCount(); i++) {
			PeekContext().GetCallContext().args.push_back(request.GetArgument(i));
		}

		//プラグイン実行
		pluginFunction(&accessor);

		//戻り値をフェッチ
		ScriptValueRef returnValue = PeekContext().GetCallContext().returnValue;
		ScriptValueRef threwError = PeekContext().GetCallContext().threwError;
	
		if (threwError != nullptr && threwError->IsObject()) {
			//例外が発生してる
			response.SetThrewError(PeekContext().GetCallContext().threwError->GetObjectRef());
		}
		else if (returnValue != nullptr) {
			//戻り値
			response.SetReturnValue(returnValue);
		}

		//ポップ
		PopContext();
	}

	//アクセサ関数
	void PluginContextManager::ReleaseHandle(aosora::ValueHandle handle) {
		GetCurrentHandleManager().Release(handle);
	}

	void PluginContextManager::AddRefHandle(aosora::ValueHandle handle) {
		GetCurrentHandleManager().AddRef(handle);
	}

	aosora::ValueHandle PluginContextManager::CreateNumber(double value) {
		return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(value));
	}

	aosora::ValueHandle PluginContextManager::CreateBool(bool value) {
		return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(value));
	}

	aosora::ValueHandle PluginContextManager::CreateString(aosora::StringContainer value) {
		return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(std::string(value.body, value.len)));
	}

	aosora::ValueHandle PluginContextManager::CreateNull() {
		return GetCurrentHandleManager().CreateHandle(ScriptValue::Null);
	}

	aosora::ValueHandle PluginContextManager::CreateFunction(aosora::ValueHandle thisValue, aosora::PluginFunctionType functionBody) {
		auto pluginDelegate = GetCurrentInterpreter().CreateNativeObject<PluginDelegate>(
			GetCurrentPluginModule(),
			functionBody,
			GetCurrentHandleManager().GetValue(thisValue)
		);

		return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(pluginDelegate));
	}

	aosora::ValueHandle PluginContextManager::CreateMap() {
		return GetCurrentHandleManager().CreateHandle(
			ScriptValue::Make(GetCurrentInterpreter().CreateObject())
		);
	}

	aosora::ValueHandle PluginContextManager::CreateArray() {
		return GetCurrentHandleManager().CreateHandle(
			ScriptValue::Make(GetCurrentInterpreter().CreateArray())
		);
	}

	aosora::ValueHandle PluginContextManager::CreateMemoryBuffer(size_t size, void** buffer, aosora::BufferDestructFunctionType destructFunc) {
		if(size == 0){
			return aosora::INVALID_VALUE_HANDLE;
		}
		Reference<MemoryBuffer> obj = GetCurrentInterpreter().CreateNativeObject<MemoryBuffer>(MemoryBuffer::BufferUsage::Plugin, size);

		//アドレス書き込み先があれば渡す
		if (buffer != nullptr) {
			*buffer = obj->GetPtr();
		}

		//プラグイン側の情報をとりこむ
		obj->GetPluginData().pluginModule = &PeekContext().GetPluginModule();
		obj->GetPluginData().destructFunc = destructFunc;

		return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(obj));
	}

	double PluginContextManager::ToNumber(aosora::ValueHandle handle) {
		return GetCurrentHandleManager().GetValue(handle)->ToNumber();
	}

	bool PluginContextManager::ToBool(aosora::ValueHandle handle) {
		return GetCurrentHandleManager().GetValue(handle)->ToBoolean();
	}

	aosora::StringContainer PluginContextManager::ToString(aosora::ValueHandle handle) {
		//プラグイン側に読んでよい文字列を渡すために一時的な文字列キャッシュをつくる
		//プラグインから関数が戻ると使用不可になるのでその場で読み終える必要がある
		const std::string& cachedString = PeekContext().CacheString(
			GetCurrentHandleManager().GetValue(handle)->ToString()
		);
		aosora::StringContainer result;
		result.body = cachedString.c_str();
		result.len = cachedString.size();
		return result;
	}

	void* PluginContextManager::ToMemoryBuffer(aosora::ValueHandle handle, size_t* size) {
		//プラグインが作成したバッファを返す
		//プラグインが意図しない変更を避けるために異なるプラグインが作成したバッファ以外は参照を拒否する
		ScriptValueRef v = GetCurrentHandleManager().GetValue(handle);
		MemoryBuffer* buffer = GetCurrentInterpreter().InstanceAs<MemoryBuffer>(v);
		if (buffer != nullptr) {
			if (buffer->GetUsage() == MemoryBuffer::BufferUsage::Plugin && buffer->GetPluginData().pluginModule == &PeekContext().GetPluginModule()) {
				if (size != nullptr) {
					*size = buffer->GetSize();
				}
				return buffer->GetPtr();
			}
		}
		return nullptr;
	}

	uint32_t PluginContextManager::GetValueType(aosora::ValueHandle handle) {
		//データ型種別を取得する
		return static_cast<uint32_t>(
			GetCurrentHandleManager().GetValue(handle)->GetValueType()
			);
	}

	uint32_t PluginContextManager::GetObjectTypeId(aosora::ValueHandle handle) {
		//オブジェクト型IDを取得する
		return GetCurrentHandleManager().GetValue(handle)->GetObjectInstanceTypeId();
	}

	uint32_t PluginContextManager::GetClassObjectTypeId(aosora::ValueHandle handle) {
		//クラスオブジェクトの型IDを取得する
		ClassData* cls = GetCurrentInterpreter().InstanceAs<ClassData>(GetCurrentHandleManager().GetValue(handle)->GetObjectRef());
		if (cls != nullptr) {
			return cls->GetClassTypeId();
		}
		return ObjectTypeIdGenerator::INVALID_ID;
	}

	bool PluginContextManager::ObjectInstanceOf(aosora::ValueHandle handle, uint32_t objectTypeId) {
		//オブジェクトが継承ツリーにふくまれるかを確認する
		ScriptValueRef v = GetCurrentHandleManager().GetValue(handle);
		if (!v->IsObject()) {
			return false;
		}
		return GetCurrentInterpreter().InstanceIs(v->GetObjectRef(), objectTypeId);
	}

	bool PluginContextManager::IsCallable(aosora::ValueHandle handle) {
		ScriptValueRef v = GetCurrentHandleManager().GetValue(handle);
		if (v->IsObject() && v->GetObjectRef()->CanCall()) {
			return true;
		}
		return false;
	}

	void PluginContextManager::SetValue(aosora::ValueHandle target, aosora::ValueHandle key, aosora::ValueHandle value) {
		auto targetValue = GetCurrentHandleManager().GetValue(target);
		auto keyValue = GetCurrentHandleManager().GetValue(key);
		if (targetValue->IsObject()) {
			targetValue->GetObjectRef()->Set(keyValue->ToString(), GetCurrentHandleManager().GetValue(value), GetCurrentExecuteContext());
		}
	}

	aosora::ValueHandle PluginContextManager::GetValue(aosora::ValueHandle target, aosora::ValueHandle key) {
		auto targetValue = GetCurrentHandleManager().GetValue(target);
		auto keyValue = GetCurrentHandleManager().GetValue(key);
		if (targetValue->IsObject()) {
			return GetCurrentHandleManager().CreateHandle(
				targetValue->GetObjectRef()->Get(keyValue->ToString(), GetCurrentExecuteContext())
			);
		}
		else {
			return aosora::INVALID_VALUE_HANDLE;
		}
	}

	size_t PluginContextManager::GetArgumentCount() {
		return PeekContext().GetCallContext().args.size();
	}

	aosora::ValueHandle PluginContextManager::GetArgument(size_t index) {
		if (index < GetArgumentCount()) {
			return GetCurrentHandleManager().CreateHandle(
				PeekContext().GetCallContext().args[index]
			);
		}
		else {
			return aosora::INVALID_VALUE_HANDLE;
		}
	}

	void PluginContextManager::SetReturnValue(aosora::ValueHandle value) {
		PeekContext().GetCallContext().returnValue = GetCurrentHandleManager().GetValue(value);
	}

	bool PluginContextManager::SetError(aosora::ValueHandle value) {
		ScriptValueRef v = GetCurrentHandleManager().GetValue(value);
		ScriptError* scriptError = GetCurrentInterpreter().InstanceAs<ScriptError>(v);
		
		//ScriptErrorオブジェクトでなければ失敗とする
		if (scriptError != nullptr) {
			PeekContext().GetCallContext().threwError = v;
			return true;
		}
		else {
			PeekContext().GetCallContext().threwError = ScriptValue::Null;
			return false;
		}
	}

	void PluginContextManager::SetPluginError(aosora::StringContainer errorMessage, int32_t errorCode) {
		Reference<PluginError> pluginError = GetCurrentInterpreter().CreateNativeObject<PluginError>(std::string(errorMessage.body, errorMessage.len));
		PeekContext().GetCallContext().threwError = ScriptValue::Make(pluginError);
	}

	void PluginContextManager::FunctionCall(aosora::ValueHandle function, const aosora::ValueHandle* argv, size_t argc) {

		ScriptValueRef functionValue = GetCurrentHandleManager().GetValue(function);

		//呼び出せなければ無視
		if (functionValue->IsObject() && functionValue->GetObjectRef()->CanCall()) {
			FunctionResponse response;

			//引数の展開
			std::vector<ScriptValueRef> args;
			for (size_t i = 0; i < argc; i++) {
				args.push_back(GetCurrentHandleManager().GetValue(argv[i]));
			}

			PeekContext().GetInterpreter().CallFunction(
				*functionValue, response, args, GetCurrentExecuteContext(), nullptr
			);

			// 戻り値と例外の記録
			PeekContext().SetLastFunctionReturnValue(response.GetReturnValue());
			if (response.IsThrew()) {
				PeekContext().SetLastError(ScriptValue::Make(response.GetThrewError()));
			}

		}
	}

	aosora::ValueHandle PluginContextManager::NewClassInstance(aosora::ValueHandle classObject, const aosora::ValueHandle* argv, size_t argc) {
		ScriptValueRef classValue = GetCurrentHandleManager().GetValue(classObject);

		if (GetCurrentInterpreter().InstanceIs<ClassData>(classValue)) {

			//引数の展開
			std::vector<ScriptValueRef> args;
			for (size_t i = 0; i < argc; i++) {
				args.push_back(GetCurrentHandleManager().GetValue(argv[i]));
			}

			ObjectRef inst = PeekContext().GetInterpreter().NewClassInstance(classValue, args, GetCurrentExecuteContext());
			return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(inst));
		}
		return aosora::INVALID_VALUE_HANDLE;
	}

	aosora::ValueHandle PluginContextManager::GetLastReturnValue() {
		return GetCurrentHandleManager().CreateHandle(PeekContext().GetLastFunctionReturnValue());
	}

	bool PluginContextManager::HasLastError() {
		//エラーがあるかどうか、ハンドル化するまえに有無の問い合わせだけできる仕組み
		return PeekContext().GetLastError()->IsObject();
	}

	aosora::ValueHandle PluginContextManager::GetLastError() {
		if (PeekContext().GetLastError()->IsObject()) {
			return GetCurrentHandleManager().CreateHandle(PeekContext().GetLastError());
		}
		return aosora::INVALID_VALUE_HANDLE;
	}

	aosora::StringContainer PluginContextManager::GetLastErrorMessage() {
		ScriptError* err = GetCurrentInterpreter().InstanceAs<ScriptError>(
			PeekContext().GetLastError()
		);

		if (err != nullptr) {
			const std::string& msg = PeekContext().CacheString(err->GetErrorMessage());
			return { msg.c_str(), msg.size() };
		}
		return STRING_CONTAINER_EMPTY;
	}

	int32_t PluginContextManager::GetLastErrorCode() {
		ScriptError* err = GetCurrentInterpreter().InstanceAs<ScriptError>(
			PeekContext().GetLastError()
		);

		if (err != nullptr) {
			return err->GetErrorCode();
		}
		return 0;
	}

	aosora::StringContainer PluginContextManager::GetErrorMessage(aosora::ValueHandle handle) {
		ScriptError* err = GetCurrentInterpreter().InstanceAs<ScriptError>(
			GetCurrentHandleManager().GetValue(handle)
		);

		if (err != nullptr) {
			const std::string& msg = PeekContext().CacheString(err->GetErrorMessage());
			return { msg.c_str(), msg.size() };
		}
		return STRING_CONTAINER_EMPTY;
	}
	
	int32_t PluginContextManager::GetErrorCode(aosora::ValueHandle handle) {
		ScriptError* err = GetCurrentInterpreter().InstanceAs<ScriptError>(
			GetCurrentHandleManager().GetValue(handle)
		);

		if (err != nullptr) {
			return err->GetErrorCode();
		}
		return 0;
	}

	aosora::ValueHandle PluginContextManager::FindUnit(aosora::StringContainer unitName) {
		Reference<UnitObject> obj = GetCurrentInterpreter().FindUnit(FromStringContainer(unitName));
		if (obj != nullptr) {
			return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(obj));
		}
		return aosora::INVALID_VALUE_HANDLE;
	}

	aosora::ValueHandle PluginContextManager::CreateUnit(aosora::StringContainer unitName) {
		//getunitは暗黙的にunitを作るので問題ないはず
		Reference<UnitObject> obj = GetCurrentInterpreter().GetUnit(FromStringContainer(unitName));
		if (obj != nullptr) {
			return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(obj));
		}
		return aosora::INVALID_VALUE_HANDLE;
	}

	uint32_t PluginContextManager::MapGetLength(aosora::ValueHandle handle) {
		auto val =  GetCurrentHandleManager().GetValue(handle);
		ScriptObject* m = GetCurrentInterpreter().InstanceAs<ScriptObject>(val);
		if (m != nullptr) {
			return m->GetLength();
		}
		return 0;
	}

	bool PluginContextManager::MapContains(aosora::ValueHandle handle, aosora::StringContainer key) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		ScriptObject* m = GetCurrentInterpreter().InstanceAs<ScriptObject>(val);
		if (m != nullptr) {
			return m->Contains(FromStringContainer(key));
		}
		return false;
	}

	void PluginContextManager::MapClear(aosora::ValueHandle handle) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		ScriptObject* m = GetCurrentInterpreter().InstanceAs<ScriptObject>(val);
		if (m != nullptr) {
			m->Clear();
		}
	}

	void PluginContextManager::MapRemove(aosora::ValueHandle handle, aosora::StringContainer key) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		ScriptObject* m = GetCurrentInterpreter().InstanceAs<ScriptObject>(val);
		if (m != nullptr) {
			m->Remove(FromStringContainer(key));
		}
	}

	aosora::ValueHandle PluginContextManager::MapGetKeys(aosora::ValueHandle handle) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		ScriptObject* m = GetCurrentInterpreter().InstanceAs<ScriptObject>(val);
		if (m != nullptr) {
			Reference<ScriptArray> items = GetCurrentInterpreter().CreateArray();
			for (auto it : m->GetInternalCollection()) {
				items->Add(it.second);
			}
			return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(items));
		}
		return aosora::INVALID_VALUE_HANDLE;
	}

	aosora::ValueHandle PluginContextManager::MapGetValue(aosora::ValueHandle handle, aosora::StringContainer key) {
		ScriptObject* m = GetCurrentInterpreter().InstanceAs<ScriptObject>(
			GetCurrentHandleManager().GetValue(handle)
		);
		if (m != nullptr) {
			return GetCurrentHandleManager().CreateHandle(m->RawGet(FromStringContainer(key)));
		}
		return aosora::INVALID_VALUE_HANDLE;
	}

	void PluginContextManager::MapSetValue(aosora::ValueHandle handle, aosora::StringContainer key, aosora::ValueHandle value) {
		ScriptObject* m = GetCurrentInterpreter().InstanceAs<ScriptObject>(
			GetCurrentHandleManager().GetValue(handle)
		);
		if (m != nullptr) {
			m->RawSet(FromStringContainer(key), GetCurrentHandleManager().GetValue(value));
		}
	}

	void PluginContextManager::ArrayClear(aosora::ValueHandle handle) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		ScriptArray* a = GetCurrentInterpreter().InstanceAs<ScriptArray>(val);
		if (a != nullptr) {
			a->Clear();
		}
	}

	void PluginContextManager::ArrayAdd(aosora::ValueHandle handle, aosora::ValueHandle item) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		ScriptArray* a = GetCurrentInterpreter().InstanceAs<ScriptArray>(val);
		if (a != nullptr) {
			a->Add(GetCurrentHandleManager().GetValue(item));
		}
	}

	void PluginContextManager::ArrayAddRange(aosora::ValueHandle handle, aosora::ValueHandle items) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		auto itemsVal = GetCurrentHandleManager().GetValue(items);

		ScriptArray* a = GetCurrentInterpreter().InstanceAs<ScriptArray>(val);
		ScriptArray* itemsArray = GetCurrentInterpreter().InstanceAs<ScriptArray>(itemsVal);
		if (a != nullptr && itemsArray != nullptr) {
			for (size_t i = 0; i < itemsArray->Count(); i++) {
				a->Add(itemsArray->At(i));
			}
		}
	}

	void PluginContextManager::ArrayInsert(aosora::ValueHandle handle, aosora::ValueHandle item, uint32_t index) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		ScriptArray* a = GetCurrentInterpreter().InstanceAs<ScriptArray>(val);
		if (a != nullptr) {
			a->Insert(GetCurrentHandleManager().GetValue(item), index);
		}
	}

	void PluginContextManager::ArrayRemove(aosora::ValueHandle handle, uint32_t index) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		ScriptArray* a = GetCurrentInterpreter().InstanceAs<ScriptArray>(val);
		if (a != nullptr) {
			a->Remove(index);
		}
	}

	uint32_t PluginContextManager::ArrayGetLength(aosora::ValueHandle handle) {
		auto val = GetCurrentHandleManager().GetValue(handle);
		ScriptArray* a = GetCurrentInterpreter().InstanceAs<ScriptArray>(val);
		if (a != nullptr) {
			return a->Count();
		}
		return 0;
	}

	aosora::ValueHandle PluginContextManager::ArrayGetValue(aosora::ValueHandle handle, uint32_t index) {
		ScriptArray* a = GetCurrentInterpreter().InstanceAs<ScriptArray>(
			GetCurrentHandleManager().GetValue(handle)
		);
		if (a != nullptr) {
			return GetCurrentHandleManager().CreateHandle(a->At(index));
		}
		return aosora::INVALID_VALUE_HANDLE;
	}

	void PluginContextManager::ArraySetValue(aosora::ValueHandle handle, uint32_t index, aosora::ValueHandle value) {
		ScriptArray* a = GetCurrentInterpreter().InstanceAs<ScriptArray>(
			GetCurrentHandleManager().GetValue(handle)
		);
		if (a != nullptr) {
			a->SetAt(index, GetCurrentHandleManager().GetValue(value));
		}
	}

}