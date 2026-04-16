echo OFF

cd %1
cd ./../

set GBE_HASH=

where git
if %ERRORLEVEL%==0 (
git.exe rev-parse HEAD > info.cpp
set /p GBE_HASH= < info.cpp
)

echo // GB Enhanced Copyright Daniel Baxter 2026 > info.cpp
echo // Licensed under the GPLv2 >> info.cpp
echo // See LICENSE.txt for full license text >> info.cpp
echo: >> info.cpp
echo // File : info.cpp >> info.cpp
echo // Description : GBE+ Compile-Time Information >> info.cpp
echo // >> info.cpp
echo // Stores information such as git commit hash, install directory, etc >> info.cpp
echo // Generated dynamically when building source code >> info.cpp

echo: >> info.cpp
echo #include ^"info.h^" >> info.cpp
echo: >> info.cpp
echo namespace gbe_info >> info.cpp
echo { >> info.cpp
echo(	std::string get_hash^(^) { return ^"%GBE_HASH%^"^; } >> info.cpp
echo(	std::string get_install_folder^(^) { return ^"^"^; } >> info.cpp
echo } >> info.cpp