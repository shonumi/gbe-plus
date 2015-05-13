export CPLUS_INCLUDE_PATH=./

if g++ -o gba/core.o -c -O3 -funroll-loops gba/core.cpp; then
	echo -e "Compiling Core...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling Core...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/arm7.o -c -O3 -funroll-loops gba/arm7.cpp; then
	echo -e "Compiling ARMv7...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling ARMv7...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/dma.o -c -O3 -funroll-loops gba/dma.cpp; then
	echo -e "Compiling DMA...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling DMA...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/arm_instr.o -c -O3 -funroll-loops gba/arm_instr.cpp; then
	echo -e "Compiling ARM instructions...		\E[32m[DONE]\E[37m"
else
	echo -e "Compiling ARM instructions...		\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/thumb_instr.o -c -O3 -funroll-loops gba/thumb_instr.cpp; then
	echo -e "Compiling THUMB instructions...		\E[32m[DONE]\E[37m"
else
	echo -e "Compiling THUMB instructions...		\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/swi.o -c -O3 -funroll-loops gba/swi.cpp; then
	echo -e "Compiling SWI...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling SWI...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/mmu.o -c -O3 -funroll-loops gba/mmu.cpp; then
	echo -e "Compiling MMU...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling MMU...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/lcd.o -c -O3 -funroll-loops gba/lcd.cpp -lSDL; then
	echo -e "Compiling LCD...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling LCD...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/apu.o -c -O3 -funroll-loops gba/apu.cpp -lSDL; then
	echo -e "Compiling APU...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling APU...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/opengl.o -c -O3 -funroll-loops gba/opengl.cpp -lSDL -lGL; then
	echo -e "Compiling OpenGL...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling OpenGL...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gba/gamepad.o -c -O3 -funroll-loops gba/gamepad.cpp -lSDL; then
	echo -e "Compiling Gamepad...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling Gamepad...			\E[31m[ERROR]\E[37m"
	exit
fi

echo -e "\E[32mGBA core complete...\E[37m"

#
#
#GBA core is done
#Move onto DMG/GBC core
#
#

if g++ -o dmg/mbc1.o -c -O3 -funroll-loops dmg/mbc1.cpp; then
	echo -e "Compiling MBC1...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling MBC1...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o dmg/mbc2.o -c -O3 -funroll-loops dmg/mbc2.cpp; then
	echo -e "Compiling MBC2...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling MBC2...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o dmg/mbc3.o -c -O3 -funroll-loops dmg/mbc3.cpp; then
	echo -e "Compiling MBC3...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling MBC3...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o dmg/mbc5.o -c -O3 -funroll-loops dmg/mbc5.cpp; then
	echo -e "Compiling MBC5...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling MBC5...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o dmg/mmu.o -c -O3 -funroll-loops dmg/mmu.cpp; then
	echo -e "Compiling MMU...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling MMU...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o dmg/mmu.o -c -O3 -funroll-loops dmg/gamepad.cpp; then
	echo -e "Compiling GamePad...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling GamePad...			\E[31m[ERROR]\E[37m"
	exit
fi

echo -e "\E[32mGB/GBC core complete...\E[37m"

#
#
#DMG core is done
#Move onto final compilation
#
#

if g++ -o common/config.o -c -O3 -funroll-loops common/config.cpp -lSDL; then
	echo -e "Compiling Config...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling Config...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o main.o -c -O3 -funroll-loops main.cpp; then
	echo -e "Compiling Main...			\E[32m[DONE]\E[37m"
else
	echo -e "Compiling Main...			\E[31m[ERROR]\E[37m"
	exit
fi

if g++ -o gbe_plus gba/core.o gba/arm7.o gba/dma.o gba/arm_instr.o gba/thumb_instr.o gba/swi.o gba/mmu.o gba/gamepad.o gba/lcd.o gba/apu.o gba/opengl.o common/config.o main.o -lSDL -lGL; then
	echo -e "Linking Project...			\E[32m[DONE]\E[37m"
else
	echo -e "Linking Project...			\E[31m[ERROR]\E[37m"
	exit
fi