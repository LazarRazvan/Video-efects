build: video.c
	gcc -Wall video.c -o effect

clean: effect
	rm -rf effect
