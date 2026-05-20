@echo off
setlocal

if /I "%~1"=="-s" (
  if "%~2"=="" exit /b 2
  for %%F in ("%~2") do (
    if exist "%%~fF" (
      if %%~zF GTR 0 exit /b 0
    )
  )
  exit /b 1
)

echo test.cmd supports only: test -s path 1>&2
exit /b 2
