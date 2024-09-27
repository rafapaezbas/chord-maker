compile:
	gcc -Wall -Werror -Wpedantic -o build/midi midi.c -lportmidi
compile-win:
	gcc -o build/midi.exe midi.c -I"C:/Program Files (x86)/portmidi/include" -L"C:/Program Files (x86)/portmidi/lib"  -lportmidi
format:
	clang-format -i *.c
