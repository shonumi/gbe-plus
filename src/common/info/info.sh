GBE_HASH=$(git rev-parse HEAD)

cd $1
cd ./../

echo -e "// GB Enhanced Copyright Daniel Baxter 2026" > info.h
echo -e "// Licensed under the GPLv2" >> info.h
echo -e "// See LICENSE.txt for full license text\n" >> info.h

echo -e "// File : info.h" >> info.h
echo -e "// Description : GBE+ Compile-Time Information\n//" >> info.h
echo -e "// Stores information such as git commit hash, install directory, etc" >> info.h
echo -e "// Generated dynamically when building source code\n" >> info.h

echo -e "namespace gbe_info\n{" >> info.h
echo -e "\tstd::string hash = \"${GBE_HASH}\";" >> info.h
echo -e "}" >> info.h
