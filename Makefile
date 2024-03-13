run: wsh
	./wsh
wsh: wsh.c
	gcc wsh.c -o wsh -Wall -Werror

debug: wsh.c
	gcc wsh.c -o wsh -Wall -Werror -g
