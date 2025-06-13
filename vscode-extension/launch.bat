setlocal
cd /d %3
set AOSORA_DEBUG_BOOTSTRAP=1
%1 /o nobootcheck,standalone,bootunlock /g %2 
endlocal
exit %errorlevel%