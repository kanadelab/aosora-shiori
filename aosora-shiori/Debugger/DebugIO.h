#pragma once

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
		static void NotifyASTExecute(const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext);
		static void NotifyError(const RuntimeError& runtimeError, const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext);
		static void Create();
		static void Destroy();
		static void Bootstrap();
		static bool IsConnected();
	};
}