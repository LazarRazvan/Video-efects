#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/*
 * in 	: pipe open for read
 * out 	: pipe open for write
 * Function reads video streams from the input pipe, process them and
 * send back to the output pipe
 * TODO : Paralize this
 */
void process_video(FILE *in, FILE *out, int width, int height) {
	unsigned char frm_arr[height * width * 3];
	int count, frame_size, pixel_r, pixel_g, pixel_b;

	frame_size = width * height * 3;
	/*
	 * We read from in pipe until the count bytes read are less than a
	 * frame size
	 */
	for (;;) {
		count = fread(frm_arr, 1, frame_size, in);
		if (count != frame_size)
			break;

		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++) {
				pixel_r = i * (width * 3) + j * 3 + 0;
				pixel_g = i * (width * 3) + j * 3 + 1;
				pixel_b = i * (width * 3) + j * 3 + 2;
				frm_arr[pixel_r] = 255 - frm_arr[pixel_r];
				frm_arr[pixel_g] = 255 - frm_arr[pixel_g];
				frm_arr[pixel_b] = 255 - frm_arr[pixel_b];
			}
		count = fwrite(frm_arr, 1, frame_size, out);
	}
}

int main(int argc, char **argv) {
	FILE *in = NULL, *out = NULL;
	int width, height;
	clock_t start, end;
	double time;

	/*
	 * Check if the application was start corectly
	 */
	if (argc != 3) {
		printf("Usage : ./filter width height\n");
		exit(-1);
	}

	width = atoi(argv[1]);
	height = atoi(argv[2]);

	/*
	 * Video streams will be read using pipes. A pipe can only be used
	 * for read or for write because it's unidirectional. Flags :
	 * -i 		: specify the input file
	 * -f 		: force the input format to pipe
	 * -vcodec 	: set the video codec
	 * -pix_fmt	: set the pixels format. In our case rgb24 means that 
	 * we have 3 components with 24 bits (1 byte per color)
	 * 
	 */
	in = popen("ffmpeg -i teapot.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "r");
	if (!in) {
		printf("Error in opening pipe for reading streams\n");
		exit(-1);
	}

	/*
	 * Flags for output pipe :
	 * -y		: ensure overwrite if necessary
	 * -f 		: set the pipe infos back to a video
	 * -vcodec 	: set the video codec
	 * -pix_fmt	: set the pixels format. In our case rgb24 means that 
	 * we have 3 components with 24 bits (1 byte per color)
	 * -s 		: set the frame rezolution
	 * -r 		: frames per second
	 * -i 		: read data from stdin
	 * -f 		: video format
	 * -an 		: no audio in the file
	 */
	out = popen("ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 30 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 output.mp4", "w");
	if (!out) {
		pclose(in);
		printf("Error in opening pipe for output streams\n");
		exit(-1);
	}

	start = clock();
	process_video(in, out, width, height);
	end = clock();
	time = (double)(end - start ) / CLOCKS_PER_SEC;
	printf("------------------------------------\n");
	printf("| Time of running is : %lf    |\n", time);
	printf("------------------------------------\n");

	fflush(in);
	pclose(in);
	fflush(out);
	pclose(out);
	return 0;
}