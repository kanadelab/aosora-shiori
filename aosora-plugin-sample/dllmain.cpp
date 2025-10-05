// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#include "pch.h"
#include "AosoraPluginWrapper.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void TestFunction(const aosora::AosoraAccessor* accessor) {
    std::string keyString("Test Return Value");

    aosora::ValueHandle returnValueHandle = accessor->CreateString({ keyString.c_str(), keyString.length() });
    accessor->SetReturnValue(returnValueHandle);
    accessor->ReleaseHandle(returnValueHandle);
}

extern "C" __declspec(dllexport) void __cdecl load(const aosora::AosoraAccessor* accessor) {
    std::string keyString("TestFunction");

    aosora::ValueHandle mapHandle = accessor->CreateMap();
    aosora::ValueHandle keyHandle = accessor->CreateString({ keyString.c_str(), keyString.length() });
    aosora::ValueHandle funcHandle = accessor->CreateFunction(aosora::INVALID_VALUE_HANDLE, TestFunction);

    accessor->SetValue(mapHandle, keyHandle, funcHandle);
    accessor->SetReturnValue(mapHandle);

    accessor->ReleaseHandle(mapHandle);
    accessor->ReleaseHandle(keyHandle);
    accessor->ReleaseHandle(funcHandle);
}