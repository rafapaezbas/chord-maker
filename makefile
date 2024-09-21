compile:
	gcc -o build/midi.exe midi.c -lportmidi
compile-list:
	gcc -o build/list.exe list.c -lportmidi
compile-win:
	gcc -o build/midi.exe midi.c -I"C:/Program Files (x86)/portmidi/include" -L"C:/Program Files (x86)/portmidi/lib"  -lportmidi
compile-list-win:
	gcc -o build/list.exe list.c -I"C:/Program Files (x86)/portmidi/include" -L"C:/Program Files (x86)/portmidi/lib"  -lportmidi
format:
	clang-format -i *.c
