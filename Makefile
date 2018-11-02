build: read_video.c
	gcc -Wall read_video.c -o filter

run: filter
	./filter

clean:
	rm -rf filter