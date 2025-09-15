#pragma once
#include <memory>
#include <string>
#include <cassert>
#include <vector>
#include <map>
#include <set>
#include "Base.h"
#include "Tokens/Tokens.h"
#include "Misc/Utility.h"

namespace sakura {

	//内部関数型
	class FunctionRequest;
	class FunctionResponse;
	class ScriptExecuteContext;
	class ScriptValue;
	class ScriptInterpreter;
	class ASTNodeBase;
	using ScriptValueRef = std::shared_ptr<ScriptValue>;
	using ASTNodeRef = std::shared_ptr<ASTNodeBase>;
	using ConstASTNodeRef = std::shared_ptr<const ASTNodeBase>;

	using ScriptNativeFunction = void(*)(const FunctionRequest& request, FunctionResponse& response);
	using ScriptNativeStaticSetFunction = void(*)(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext);
	using ScriptNativeStaticGetFunction = ScriptValueRef(*)(const std::string& key, ScriptExecuteContext& executeContext);
	using ScriptNativeStaticInitFunction = void(*)(ScriptInterpreter& interpreter);
	using ScriptNativeStaticDestructFunction = void(*)(ScriptInterpreter& interpreter);

	//ASTノードの種類
	enum class ASTNodeType {
		Invalid,
		CodeBlock,
		FormatString,
		StringLiteral,
		NumberLiteral,
		BooleanLiteral,
		UnitRoot,
		ResolveSymbol,
		AssignSymbol,
		ContextValue,
		ArrayInitializer,
		ObjectInitializer,
		FunctionStatement,
		FunctionInitializer,
		LocalVariableDeclaration,
		LocalVariableDeclarationList,
		For,
		While,
		If,
		Break,
		Continue,
		Return,
		Operator2,
		Operator1,
		NewClassInstance,
		FunctionCall,
		ResolveMember,
		AssignMember,
		Try,
		Throw,

		//トークブロック
		TalkSetSpeaker,
		TalkJump,
		TalkSpeak
	};

	//演算子
	enum class OperatorType {
		Add,
		Sub,
		Mul,
		Div,
		Mod,
		Minus,
		Plus,

		Gt,
		Lt,
		Ge,
		Le,
		Eq,
		Ne,

		LogicalAnd,
		LogicalOr,
		LogicalNot,

		Assign,
		AssignAdd,
		AssignSub,
		AssignMul,
		AssignDiv,
		AssignMod,

		Call,
		Bracket,
		Comma,
		Member,
		Index,
		New
	};

	//演算子情報
	// C++ を参考に https://learn.microsoft.com/ja-jp/cpp/cpp/cpp-built-in-operators-precedence-and-associativity?view=msvc-170
	struct OperatorInformation {
		OperatorType type;
		int32_t priority;
		uint8_t argCount;
		bool isLeftToRight;
		const char* preview;
	};

	//スクリプトユニット
	//namespaceのようなものとしてモジュール分離のために
	class ScriptUnit {
	private:
		std::vector<std::string> unitPath;
		std::string unitName;
		bool explicitUnit;

	public:
		ScriptUnit() {
			//デフォルトで "main" が使用される
			SetUnit("main");
			explicitUnit = false;
		}

		void SetUnit(const std::string& unit) {
			unitName = unit;
			SplitString(unit, unitPath, '.');
			explicitUnit = true;	//明示的な指定としてマーク
		}

		const std::string& GetUnit() const {
			return unitName;
		}

		std::string MakeChildUnitName(const char* nodeName) const {
			return unitName + "." + nodeName;
		}

		size_t GetUnitPathLevel() const {
			return unitPath.size();
		}

		const std::string& GetUnitPath(size_t index) const {
			return unitPath[index];
		}

		bool IsExplicitUnit() const {
			return explicitUnit;
		}
	};
	using ScriptUnitRef = std::shared_ptr<ScriptUnit>;

	struct AliasItem {
		//エイリアス１つあたりの情報。
		std::string fullPath;	//フルエイリアス（{親ユニット}.{ターゲット名}）
		std::string parentUnit;	//親ユニット
		std::string targetName;	//ターゲット名
	};

	//スクリプトユニットエイリアス usingのような
	class ScriptUnitAlias {
	private:
		std::map<std::string, AliasItem> aliasMap;
		std::vector<std::string> wildcardAliases;
		std::set<std::string> wildcardAliasKeys;

	public:
		bool RegisterAlias(const std::string& key, const std::string& path) {
			if (aliasMap.contains(key)) {
				//多重追加は不可
				return false;
			}

			//エイリアスを登録
			AliasItem item;
			item.fullPath = path;
			const size_t dotPos = path.rfind(".");
			if (dotPos != std::string::npos) {
				//最後とそれまでとで切り離す
				item.parentUnit = path.substr(0, dotPos);
				item.targetName = path.substr(dotPos + 1);
			}
			else {
				item.parentUnit = "";
				item.targetName = path;
			}

			aliasMap.insert(decltype(aliasMap)::value_type(key, item));
			return true;
		}

		bool RegisterWildcardAlias(const std::string& key) {
			if (!wildcardAliasKeys.contains(key)) {
				wildcardAliases.push_back(key);
				wildcardAliasKeys.insert(key);
				return true;
			}
			return false;
		}

		const AliasItem* FindAlias(const std::string& key) const {
			auto found = aliasMap.find(key);
			if (found != aliasMap.end()) {
				return &found->second;
			}
			return nullptr;
		}

		size_t GetWildcardAliasCount() const {
			return wildcardAliases.size();
		}

		const std::string& GetWildcardAlias(size_t index) const {
			return wildcardAliases[index];
		}
	};

	//ソーススコープのメタデータのこと
	class ScriptSourceMetadata {
	private:
		ScriptUnitRef scriptUnit;
		ScriptUnitAlias alias;

	public:
		ScriptSourceMetadata() {
			//TODO: ここをリファレンスにする必要ないと思われ
			scriptUnit.reset(new ScriptUnit());
		}

		const ScriptUnitRef& GetScriptUnit() const {
			return scriptUnit;
		}
		ScriptUnitAlias& GetAlias() {
			return alias;
		}
		const ScriptUnitAlias& GetAlias() const {
			return alias;
		}

		//メタデータの確定
		void CommitMetadata() {

			//今のユニットからエイリアスを決定
			std::string unitName;
			unitName.reserve(scriptUnit->GetUnit().size());

			for (size_t i = 0; i < scriptUnit->GetUnitPathLevel(); i++) {
				if (!unitName.empty()) {
					unitName += ".";
				}
				unitName += scriptUnit->GetUnitPath(i);

				//ワイルドカードに登録
				alias.RegisterWildcardAlias(unitName);
			}

			//systemユニットを登録
			alias.RegisterWildcardAlias("system");
		}
	};
	using ScriptSourceMetadataRef = std::shared_ptr<ScriptSourceMetadata>;

	//ASTノード基底
	class ASTNodeBase {
	private:
		SourceCodeRange sourceRange;
		ScriptSourceMetadataRef sourceMetadata;

	public:
		ASTNodeBase(const ScriptSourceMetadataRef& sourceMeta):
			sourceMetadata(sourceMeta)
		{}

		//ソースコード範囲を追加
		void SetSourceRange(const ScriptToken& begin, const ScriptToken& includedEnd) {
			sourceRange.SetRange(begin.sourceRange, includedEnd.sourceRange);
		}
		void SetSourceRange(const ScriptToken& token) {
			sourceRange = token.sourceRange;
		}
		void SetSourceRange(const SourceCodeRange& range) {
			sourceRange = range;
		}
		void SetSourceRange(const SourceCodeRange& begin, const SourceCodeRange& includedEnd) {
			sourceRange.SetRange(begin, includedEnd);
		}

		const SourceCodeRange& GetSourceRange() const { return sourceRange; }

		//ゲット系をセット系にコンバート可能かどうか
		virtual bool CanConvertToSetter() const { return false; }
		virtual ASTNodeRef ConvertToSetter(const ASTNodeRef& valueNode) const { assert(false); return nullptr; }

		virtual ASTNodeType GetType() const { return ASTNodeType::Invalid; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const {};

		virtual const char* DebugName() const = 0;
		virtual std::string DebugToString() const { return ""; }
		virtual void DebugDump(int32_t indent) const {
			std::string indentSpace(indent * 2, ' ');
			printf("%s[%s] %s\n", indentSpace.c_str(), DebugName(), DebugToString().c_str());
		}

		//再帰的にノードを全部取得
		void GetChildrenRecursive(std::vector<ConstASTNodeRef>& node) const {
			const size_t beginIndex = node.size();
			GetChildren(node);
			const size_t endIndex = node.size();
			//今回追加した範囲をたどる
			for (size_t i = beginIndex; i < endIndex; i++) {
				node[i]->GetChildrenRecursive(node);
			}
		}

		//スクリプトソースメタデータ
		const ScriptSourceMetadataRef& GetSourceMetadata() const {
			return sourceMetadata;
		}

		//スクリプトユニット取得
		const ScriptUnitRef& GetScriptUnit() const {
			return sourceMetadata->GetScriptUnit();
		}
	};

	//スクリプト関数
	class ScriptFunction {
	private:
		//関数ルートのAST
		ASTNodeRef functionBody;

		//引数リスト
		std::vector<std::string> argumentList;

	public:
		ScriptFunction(const ASTNodeRef& body, const std::vector<std::string>& args) :
			functionBody(body),
			argumentList(args)
		{}

		const ASTNodeRef& GetFunctionBody() const { return functionBody; }
		const std::vector<std::string>& GetArguments() const { return argumentList; }

		void DebugDump(int32_t indent) const{
			functionBody->DebugDump(indent);
		}
	};

	using ScriptFunctionRef = std::shared_ptr<ScriptFunction>;
	using ConstScriptFunctionRef = std::shared_ptr<const ScriptFunction>;

	//スクリプト定義
	struct ScriptFunctionDef {
		std::vector<std::string> names;		//名前リスト
		ScriptFunctionRef func;				//本体
		ASTNodeRef condition;				//呼出条件
	};

	//スクリプトクラスメンバ
	class ScriptClassMember {
	private:
		std::string name;

		//メンバイニシャライザ
		ASTNodeRef initializer;

	public:
		ScriptClassMember(const std::string& memberName, const ASTNodeRef& initializerNode) :
			name(memberName),
			initializer(initializerNode)
		{}

		ScriptClassMember(const std::string& memberName) :
			name(memberName),
			initializer(nullptr)
		{}

		std::string GetName() {
			return name;
		}

		ASTNodeRef GetInitializer() {
			return initializer;
		}
	};

	class ClassBase;
	using ClassRef = std::shared_ptr<ClassBase>;

	//型に対するIDジェネレータ
	class ObjectTypeIdGenerator {
	private:
		static uint32_t counter;

	public:
		//0は無効値として予約
		static const uint32_t INVALID_ID = 0;

		//ネイティブ型ID
		template<typename T>
		static uint32_t Id() {
			static uint32_t typeId = ++counter;
			return typeId;
		}

		static uint32_t GenerateScriptClassId(uint32_t classIndex) {
			//スクリプトクラスの場合はインタプリタ側のインデックスの最上位ビットを立てたものとしておく
			return (1u << 31) | (classIndex);
		}
	};

	//クラスパス
	//クラスだけはスクリプト起動前に検索する形になるので、スクリプトは実行せずに階層検索するためのパス情報
	class ClassPath {
	private:
		bool isFullPath;	//unitキーワードから始める場合はフルパス扱いになる
		std::vector<std::string> pathList;

	public:
		ClassPath():
			isFullPath(false)
		{ }

		//C++側向けに簡易的に親クラス指定できるタイプ
		ClassPath(const char* className, const char* unit = "system") :
			isFullPath(true)
		{
			pathList.push_back(unit);
			pathList.push_back(className);
		}

		bool IsFullPath() const { return isFullPath; }
		void SetFullPath(bool fullPath) { isFullPath = fullPath; }

		void AddFullPath(const std::string& pathNode) { pathList.push_back(pathNode); }
		size_t GetPathNodeCount() const { return pathList.size(); }
		const std::string& GetPathNode(size_t index) const { return pathList[index]; }
		const std::vector<std::string>& GetPathNodeCollection() const { return pathList; }

		bool IsValid() const { return !pathList.empty(); }
	};

	//クラスベース
	//ネイティブクラスとスクリプトクラスと両方でインスタンス化できる型情報
	class ClassBase {
	private:

		//解析時に登録する情報
		std::string name;
		ClassPath parentClassPath;

		//クラスID
		uint32_t typeId;

	public:
		ClassBase(uint32_t classTypeId):
			typeId(classTypeId)
		{}

		//スクリプトクラスかネイティブクラスか
		virtual bool IsScriptClass() const = 0;

		//ユニット名
		virtual std::string GetUnitName() const = 0;

		void SetName(const std::string& className) {
			name = className;
		}

		const std::string& GetName() const {
			return name;
		}

		bool HasParentClass() const {
			return parentClassPath.IsValid();
		}

		const ClassPath& GetParentClassPath() const {
			return parentClassPath;
		}

		ClassPath& GetParentClassPath() {
			return parentClassPath;
		}

		void SetParentClassPath(const ClassPath& classPath) {
			parentClassPath = classPath;
		}

		uint32_t GetTypeId() const {
			return typeId;
		}

		//スクリプトクラスのための後付けID指定
		void SetTypeId(uint32_t classTypeId) {
			assert(typeId == 0);	//後付け専用なのですでに設定されていたら使用禁止
			if (typeId != 0) {
				return;
			}
			typeId = classTypeId;
		}	
	};

	//ネイティブクラス
	class NativeClass : public ClassBase {
	private:
		ScriptNativeFunction initFunc;
		ScriptNativeStaticGetFunction staticGetFunc;
		ScriptNativeStaticSetFunction staticSetFunc;
		ScriptNativeStaticInitFunction staticInitFunc;
		ScriptNativeStaticDestructFunction staticDestructFunc;
		std::string unitName;

	public:
		template<typename T>
		static std::shared_ptr<NativeClass> Make(const std::string& name, ScriptNativeFunction initFunction = nullptr, const std::string& unitName = "system") {
			std::shared_ptr<NativeClass> result(new NativeClass(initFunction, ObjectTypeIdGenerator::Id<T>(), unitName));
			result->SetName(name);
			result->staticGetFunc = &T::StaticGet;
			result->staticSetFunc = &T::StaticSet;
			result->staticInitFunc = &T::StaticInit;
			result->staticDestructFunc = &T::StaticDestruct;
			return result;
		}

		template<typename T>
		static std::shared_ptr<NativeClass> Make(const std::string& name, const ClassPath& parentClassPath, ScriptNativeFunction initFunction = nullptr, const std::string& unitName = "system") {
			std::shared_ptr<NativeClass> result(new NativeClass(initFunction, ObjectTypeIdGenerator::Id<T>(), unitName));
			result->SetName(name);
			result->SetParentClassPath(parentClassPath);
			result->staticGetFunc = &T::StaticGet;
			result->staticSetFunc = &T::StaticSet;
			result->staticInitFunc = &T::StaticInit;
			result->staticDestructFunc = &T::StaticDestruct;
			return result;
		}
		
		NativeClass(ScriptNativeFunction initFunction, uint32_t classId, const std::string& unitName) : ClassBase(classId),
			initFunc(initFunction),
			unitName(unitName)
		{}

		ScriptNativeFunction GetInitFunc() const {
			return initFunc;
		}

		ScriptNativeStaticGetFunction GetStaticGetFunc() const {
			return staticGetFunc;
		}

		ScriptNativeStaticSetFunction GetStaticSetFunc() const {
			return staticSetFunc;
		}

		ScriptNativeStaticInitFunction GetStaticInitFunc() const {
			return staticInitFunc;
		}

		ScriptNativeStaticInitFunction GetStaticDestructFunc() const {
			return staticDestructFunc;
		}

		virtual bool IsScriptClass() const override { return false; }
		virtual std::string GetUnitName() const override { return unitName; }
	};

	class ScriptClassMember;
	class ScriptClass;
	using ScriptClassMemberRef = std::shared_ptr<ScriptClassMember>;
	using ScriptClassRef = std::shared_ptr<ScriptClass>;

	//スクリプトクラス
	class ScriptClass : public ClassBase {
	private:
		//関数
		std::vector<ScriptFunctionDef> functions;

		//コンストラクタ
		ScriptFunctionRef initFunc;

		//親コンストラクタ呼び出しAST
		std::vector<ConstASTNodeRef> parentClassInitArguments;

		//スクリプト情報
		ScriptSourceMetadataRef sourceMetadata;

		//宣言ソースレンジ
		SourceCodeRange declareSourceRange;

	public:
		ScriptClass(const ScriptSourceMetadataRef& sourcemeta) : ClassBase(0),
			sourceMetadata(sourcemeta){
		}

		virtual bool IsScriptClass() const override { return true; }
		virtual std::string GetUnitName() const override { return sourceMetadata->GetScriptUnit()->GetUnit(); }

		void AddFunction(const ScriptFunctionDef& def) {
			functions.push_back(def);
		}

		size_t GetFunctionCount() const {
			return functions.size();
		}

		const ScriptFunctionDef& GetFunction(size_t index) const {
			return functions[index];
		}

		//初期化関数
		void SetInitFunc(const ScriptFunctionRef& func) {
			initFunc = func;
		}

		ScriptFunctionRef GetInitFunc() const {
			return initFunc;
		}

		//初期化関数の引数情報
		size_t GetParentClassInitArgumentCount() const {
			return parentClassInitArguments.size();
		}

		ConstASTNodeRef GetParentClassInitArgument(size_t index) const {
			return parentClassInitArguments[index];
		}

		void SetParentClassInitArguments(const std::vector<ConstASTNodeRef>& args) {
			parentClassInitArguments = args;
		}

		const ScriptSourceMetadataRef& GetSourceMetadata() const {
			return sourceMetadata;
		}

		//宣言部ソースレンジ
		void SetDeclareSourceRange(const SourceCodeRange& sourceRange) {
			declareSourceRange = sourceRange;
		}

		const SourceCodeRange& GetDeclareSourceRange() const {
			return declareSourceRange;
		}

	};

}