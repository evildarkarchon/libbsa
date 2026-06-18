Get-ChildItem -Recurse -Include *.cpp, *.h, *.cc, *.hpp | Where-Object { $_.FullName -notmatch "TES5Edit" } | ForEach-Object { clang-format -i $_.FullName }
graphify update .