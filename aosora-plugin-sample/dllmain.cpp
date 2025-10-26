// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#include "AosoraPluginWrapper.h"

void TestFunction(const aosora::AosoraAccessor accessor) {

    accessor.SetReturnValue(accessor.CreateString("てすとーーーー"));

}

extern "C" __declspec(dllexport) void __cdecl aosora_plugin_get_version(const aosora::PluginVersionInfo versionInfo) {
    if (!versionInfo.CheckBinaryCompatibility()) {
        return;
    }
    versionInfo.VersionCheckOk();
}

extern "C" __declspec(dllexport) void __cdecl aosora_plugin_load(const aosora::AosoraAccessor accessor) {

    auto m = accessor.CreateMap();
    m.MapSetValue("TestFunction", accessor.CreateFunction(TestFunction));
    accessor.SetReturnValue(m);

}