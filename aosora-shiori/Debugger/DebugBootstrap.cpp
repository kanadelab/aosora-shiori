
#include <cassert>
#include <windows.h>
#include <CommCtrl.h>
#include "Debugger/DebuggerCore.h"

#if defined(AOSORA_ENABLE_DEBUGGER)
namespace sakura
{
	//デバッグブートストラップウインドウ
	//SSPを起動してからAosoraデバッガが接続してくるまで待機するたための、接続待ちを案内するウインドウ
	class DebugBootstrapWindow
	{
	private:
		static DebugBootstrapWindow* instance;
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		HWND hWnd;				//本体ウインドウハンドル
		HWND hCancelButton;		//キャンセルボタンハンドル
		HWND hExitButton;		//強制終了ボタン

	public:
		static void Run();
	};

	DebugBootstrapWindow* DebugBootstrapWindow::instance = nullptr;

	LRESULT CALLBACK DebugBootstrapWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
				DrawTextW(hdc, L"Aosora Debuggerの接続を待機しています...", -1, &ps.rcPaint, 0);

				EndPaint(hwnd, &ps);
				return 0;
			}
			case WM_COMMAND:
			{
				if ((HWND)lParam == instance->hCancelButton) {
					//強制終了
					Debugger::TerminateProcess();
				}
				/*
				else if ((HWND)lParam == instance->hExitButton) {
					//強制終了
					exit(0);
				}
				*/
			}
			case WM_CLOSE:
			{
				DestroyWindow(hwnd);
			}
			default:
				return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	void ProcessMessage(HWND hWnd)
	{
		MSG msg;
		if (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void DebugBootstrapWindow::Run()
	{
		assert(instance == nullptr);
		if (instance != nullptr) {
			return;
		}
		instance = new DebugBootstrapWindow();

		//ウインドウ作成～待機を行う
		const char* bootstrapWindowClassName = "aosoraDebugBootstrap";

		WNDCLASS wc = {};
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = GetModuleHandleA(nullptr);
		wc.lpszClassName = bootstrapWindowClassName;
		RegisterClass(&wc);

		//ウインドウを作成
		HWND hWnd = CreateWindowEx(0, bootstrapWindowClassName, "Test Window", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 400, 120,
			NULL, NULL, wc.hInstance, NULL);
		ShowWindow(hWnd, TRUE);
		assert(hWnd != 0);

		HWND hwndButton = CreateWindowW(WC_BUTTONW, L"強制終了", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 30, 100, 30, hWnd, NULL, wc.hInstance, NULL);
		//HWND hwndButton2 = CreateWindowW(WC_BUTTONW, L"強制終了", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 120, 30, 100, 30, hWnd, NULL, wc.hInstance, NULL);

		instance->hWnd = hWnd;
		instance->hCancelButton = hwndButton;
		//instance->hExitButton = hwndButton2;
		bool isConnected = false;

		//メッセージループ
		while (true) {
			ProcessMessage(hWnd);

			//接続できたらウインドウを閉じる
			if (!isConnected && Debugger::IsConnected()) {
				isConnected = true;
				DestroyWindow(hWnd);
			}

			if (!IsWindow(hWnd)) {
				//ウインドウがしんだら抜ける
				break;
			}
		}

		delete instance;
		instance = nullptr;
	}

	void Debugger::Bootstrap() {	
		//指定の環境変数が設定されていれば機動隊気に入る
		const char* bootstrapEnv = getenv("AOSORA_DEBUG_BOOTSTRAP");
		bool enableDebugBootstrap = false;
		if (bootstrapEnv != nullptr) {
			if (strcmp(bootstrapEnv, "0") != 0) {
				_putenv_s("AOSORA_DEBUG_BOOTSTRAP", "0");
				enableDebugBootstrap = true;
			}
		}
		
		if (enableDebugBootstrap) {
			Debugger::SetDebugBootstrapped(true);
			sakura::DebugBootstrapWindow::Run();
		}

	}
}
#endif