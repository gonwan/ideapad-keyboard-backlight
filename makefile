all: ideapad_keyboard_backlight.c
	gcc -O2 ideapad_keyboard_backlight.c -o ideapad_keyboard_backlight
package: all
	zip ideapad_keyboard_backlight_$(VERSION).zip ideapad_keyboard_backlight.exe setup.bat setup.ps1
