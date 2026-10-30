GBE_HASH=$(git rev-parse HEAD)
GBE_INSTALL_FOLDER=$2
GBE_EXEC_FOLDER=$3
GBE_SYS_NAME=$4

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
echo -e "std::string get_install_folder() { return \"${GBE_INSTALL_FOLDER}\"; }" >> info.cpp
echo -e "std::string get_system_name() { return \"${GBE_SYS_NAME}\"; }" >> info.cpp

echo -e "\nstd::string get_platform(int id)\n{" >> info.cpp
echo -e "\tswitch(id)\n\t{" >> info.cpp
echo -e "\t\tcase 0: return \"WINDOWS\";" >> info.cpp
echo -e "\t\tcase 1: return \"APPLE\";" >> info.cpp
echo -e "\t\tcase 2: return \"UNIX\";" >> info.cpp
echo -e	"\t}\n" >> info.cpp
echo -e "\treturn \"OTHER\";\n}" >> info.cpp

echo -e "\n}" >> info.cpp

cd ./../data/icons/

echo -e "[Desktop Entry]" > gbe_plus.desktop
echo -e "Type=Application" >> gbe_plus.desktop
echo -e "Version=1.10" >> gbe_plus.desktop
echo -e "Name=GBE+" >> gbe_plus.desktop
echo -e "Comment=GB Enhanced+ is a DMG/GBC/GBA/Pokemon Mini emulator and experimental NDS emulator." >> gbe_plus.desktop
echo -e "Exec=${GBE_EXEC_FOLDER}/gbe_plus_qt" >> gbe_plus.desktop
echo -e "Icon=${GBE_INSTALL_FOLDER}/data/icons/gbe_plus.png" >> gbe_plus.desktop
echo -e "Terminal=false" >> gbe_plus.desktop
echo -e "Categories=Game;Emulation" >> gbe_plus.desktop