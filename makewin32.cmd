@echo off
Rem
Rem Windows NT command file to build CVS Project
Rem
Rem Builds Win32 Debug configuration as the default
Rem
if %1%x == x goto compile_debug
if %1%x == -cx goto compile_clean
if %1%x == -cdx goto compile_clean_debug
if %1%x == -dcx goto compile_clean_debug
if %1%x == -crx goto compile_clean_release
if %1%x == -rcx goto compile_clean_release
if %1%x == -dx goto compile_debug
if %1%x == -rx goto compile_release
echo Error: Invalid option: %1%
echo Usage: makewin32
echo Usage: makewin32 -c
echo Usage: makewin32 -cd
echo Usage: makewin32 -cr
echo Usage: makewin32 -d
echo Usage: makewin32 -r
goto compile_done

:compile_clean
nmake /f cvsnt.mak CFG="cvsnt - Win32 Debug" CLEAN
nmake /f cvsnt.mak CFG="cvsnt - Win32 Release" CLEAN
goto compile_done

:compile_clean_debug
nmake /f cvsnt.mak CFG="cvsnt - Win32 Debug" CLEAN
goto compile_done

:compile_clean_release
nmake /f cvsnt.mak CFG="cvsnt - Win32 Release" CLEAN
goto compile_done

:compile_debug
nmake /f cvsnt.mak CFG="cvsnt - Win32 Debug"
goto compile_done

:compile_release
nmake /f cvsnt.mak CFG="cvsnt - Win32 Release"
goto compile_done

:compile_done
