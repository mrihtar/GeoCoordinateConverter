@echo off
@rem Define this to point to your version of awk
set awk=C:\Cygwin\bin\awk.exe
if not exist %awk% echo There is a problem with %awk%
@rem
@rem Extract GK points from GURS data
@rem
%awk% -f extgk.awk virtualne_vezne_tocke_v3.0.txt > vvt-gk.node
if errorlevel 1 exit /b 1
.\triangle.exe vvt-gk.node
if errorlevel 1 exit /b 1
@rem
@rem Extract TM points from GURS data
@rem
%awk% -f exttm.awk virtualne_vezne_tocke_v3.0.txt > vvt-tm.node
if errorlevel 1 exit /b 1
.\triangle.exe vvt-tm.node
if errorlevel 1 exit /b 1
@rem
@rem Generate include files for gk-slo
@rem
.\ctt.exe -d vvt-gk.node vvt-tm.node vvt-gk.1.ele
if errorlevel 1 exit /b 1
@rem
exit /b 0
