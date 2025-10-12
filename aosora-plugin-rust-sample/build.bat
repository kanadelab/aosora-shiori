cargo build --target=i686-pc-windows-msvc
copy /Y %~dp0\target\i686-pc-windows-msvc\debug\aosora_plugin_rust_sample.dll %~dp0\..\ssp\ghost\test\ghost\master\aosora_plugin_rust.dll

