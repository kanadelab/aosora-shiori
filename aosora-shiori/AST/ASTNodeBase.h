#pragma once
#include <memory>
#include <string>
#include <cassert>
#include <vector>
#include <map>
#include "Base.h"
#include "Tokens/Tokens.h"

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
		ResolveSymbol,
		AssignSymbol,
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

	//ASTノード基底
	class ASTNodeBase {
	private:
		SourceCodeRange sourceRange;

	public:
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

	//クラスベース
	//ネイティブクラスとスクリプトクラスと両方でインスタンス化できる型情報
	class ClassBase {
	private:

		//解析時に登録する情報
		std::string parentClassName;
		std::string name;

		//クラスID
		const uint32_t typeId;

	public:
		ClassBase(uint32_t classTypeId):
			typeId(classTypeId)
		{}

		//スクリプトクラスかネイティブクラスか
		virtual bool IsScriptClass() const = 0;

		void SetName(const std::string& className) {
			name = className;
		}

		const std::string& GetName() const {
			return name;
		}

		const std::string& GetParentClassName() const {
			return parentClassName;
		}

		void SetParentClassName(const std::string& className) {
			parentClassName = className;
		}

		bool HasParentClass() const {
			return !parentClassName.empty();
		}

		const std::string& GetParentClassName() {
			return parentClassName;
		}

		uint32_t GetTypeId() const {
			return typeId;
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

	public:
		template<typename T>
		static std::shared_ptr<NativeClass> Make(const std::string& name, ScriptNativeFunction initFunction = nullptr) {
			std::shared_ptr<NativeClass> result(new NativeClass(initFunction, ObjectTypeIdGenerator::Id<T>()));
			result->SetName(name);
			result->staticGetFunc = &T::StaticGet;
			result->staticSetFunc = &T::StaticSet;
			result->staticInitFunc = &T::StaticInit;
			result->staticDestructFunc = &T::StaticDestruct;
			return result;
		}

		template<typename T>
		static std::shared_ptr<NativeClass> Make(const std::string& name, const std::string& parentName, ScriptNativeFunction initFunction = nullptr) {
			std::shared_ptr<NativeClass> result(new NativeClass(initFunction, ObjectTypeIdGenerator::Id<T>()));
			result->SetName(name);
			result->SetParentClassName(parentName);
			result->staticGetFunc = &T::StaticGet;
			result->staticSetFunc = &T::StaticSet;
			result->staticInitFunc = &T::StaticInit;
			result->staticDestructFunc = &T::StaticDestruct;
			return result;
		}
		
		NativeClass(ScriptNativeFunction initFunction, uint32_t classId) : ClassBase(classId),
			initFunc(initFunction)
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
	};

	class ScriptClassMember;
	class ScriptClass;
	using ScriptClassMemberRef = std::shared_ptr<ScriptClassMember>;
	using ScriptClassRef = std::shared_ptr<ScriptClass>;

	//スクリプトクラス
	class ScriptClass : public ClassBase {
	private:
		//メンバ
		std::vector<ScriptClassMemberRef> members;

		//関数
		std::vector<ScriptFunctionDef> functions;

		//コンストラクタ
		ScriptFunctionRef initFunc;

		//親コンストラクタ
		std::vector<ASTNodeRef> parentClassInitArguments;

	public:
		ScriptClass() : ClassBase(0) {
		}

		virtual bool IsScriptClass() const override { return true; }


		//メンバの追加
		void AddMember(const std::string& name, const ASTNodeRef& initNode) {
			members.push_back(ScriptClassMemberRef(new ScriptClassMember(name, initNode)));
		}

		size_t GetMemberCount() const {
			return members.size();
		}

		ScriptClassMemberRef GetMember(size_t index) const {
			return members[index];
		}

		bool ContainsMember(const std::string& name) const {
			//TODO: 高速化
			for (const auto& item : members) {
				if (item->GetName() == name) {
					return true;
				}
			}
			return false;
		}

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

		ASTNodeRef GetParentClassInitArgument(size_t index) const {
			return parentClassInitArguments[index];
		}

	};

}