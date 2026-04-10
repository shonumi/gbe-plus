GBE_HASH=$(git rev-parse HEAD)

cd $1
cd ./../

echo -e "// GB Enhanced Copyright Daniel Baxter 2026" > info.cpp
echo -e "// Licensed under the GPLv2" >> info.cpp
echo -e "// See LICENSE.txt for full license text\n" >> info.cpp

echo -e "// File : info.cpp" >> info.cpp
echo -e "// Description : GBE+ Compile-Time Information\n//" >> info.cpp
echo -e "// Stores information such as git commit hash, install directory, etc" >> info.cpp
echo -e "// Generated dynamically when building source code\n" >> info.cpp

echo -e "#include \"info.h\"\n" >> info.cpp

echo -e "namespace gbe_info\n{\n" >> info.cpp
echo -e "std::string get_hash() { return \"${GBE_HASH}\"; }" >> info.cpp
echo -e "\n}" >> info.cpp
