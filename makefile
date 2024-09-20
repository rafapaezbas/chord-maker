compile:
	gcc -o build/midi.exe midi.c -lportmidi

compile-list:
	gcc -o build/list.exe list.c -lportmidi
