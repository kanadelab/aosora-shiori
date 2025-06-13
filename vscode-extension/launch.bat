setlocal
cd /d %3
set AOSORA_DEBUG_BOOTSTRAP=1
%1 /g %2
endlocal
exit %errorlevel%