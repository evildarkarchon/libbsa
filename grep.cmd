@echo off
setlocal

:parse_options
if "%~1"=="" goto usage
set "arg=%~1"
if "%arg:~0,1%"=="-" (
  shift
  goto parse_options
)

set "LIBBSA_GREP_NEEDLE=%~1"
shift
if "%~1"=="" goto usage
set "LIBBSA_GREP_FILE=%~1"

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$needle = $env:LIBBSA_GREP_NEEDLE; $file = $env:LIBBSA_GREP_FILE; if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { exit 2 }; $path = (Resolve-Path -LiteralPath $file).ProviderPath; $text = [System.IO.File]::ReadAllText($path); if ($text.Contains($needle)) { exit 0 } exit 1"
exit /b %ERRORLEVEL%

:usage
echo grep.cmd supports fixed-string smoke checks such as: grep -Fq token file 1>&2
exit /b 2
