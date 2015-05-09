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


if g++ -o gba/config.o -c -O3 -funroll-loops gba/config.cpp -lSDL; then
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

if g++ -o gbe_plus gba/core.o gba/arm7.o gba/dma.o gba/arm_instr.o gba/thumb_instr.o gba/swi.o gba/mmu.o gba/gamepad.o gba/lcd.o gba/apu.o gba/opengl.o gba/config.o main.o -lSDL -lGL; then
	echo -e "Linking Project...			\E[32m[DONE]\E[37m"
else
	echo -e "Linking Project...			\E[31m[ERROR]\E[37m"
	exit
fi