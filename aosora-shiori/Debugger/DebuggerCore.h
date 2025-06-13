#pragma once

#include "Base.h"
#include "AST/AST.h"
#include "Interpreter/Interpreter.h"

namespace sakura {

	class ILock {
	public:
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
	};

	class IEvent {
	public:
		virtual bool Wait(int32_t timeoutMs = -1) = 0;
		virtual void Raise() = 0;
	};

	class LockScope {
	private:
		ILock& lock;

	public:
		LockScope() = delete;

		LockScope(ILock& lock) :
			lock(lock) {
			lock.Lock();
		}

		~LockScope() {
			lock.Unlock();
		}
	};

	//デバッグインターフェース
	class Debugger {
	public:
		//ASTノードの実行時に通知（いまのところ行単位の実行の想定）
		static void NotifyASTExecute(const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext);

		//エラースロー時の通知（いまのところキャッチに関係なく）
		static void NotifyError(const RuntimeError& runtimeError, const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext);

		//スクリプトファイル読み込み時の通知
		static void NotifyScriptFileLoaded(const std::string& fileBody, const std::string& fullName);

		//ログ出力時に通知
		static void NotifyLog(const std::string& log, bool isError = false);
		static void NotifyLog(const std::string& log, const ASTNodeBase& node, bool isError = false);
		static void NotifyLog(const std::string& log, const SourceCodeRange& range, bool isError = false);

		//イベントから処理が戻った。stepin等の次回ブレーク処理を無効化する。
		static void NotifyEventReturned();

		static void Create(uint32_t connectionPort);
		static void Destroy();
		static bool IsCreated();
		static void Bootstrap();
		static bool IsConnected();

		//起動処理からデバッグツールから行われているとマークする（切断時強制終了）
		static void SetDebugBootstrapped(bool isBootstrapped);
		static bool IsDebugBootstrapped();

		//プロセスの強制終了
		static void TerminateProcess();
	};
}