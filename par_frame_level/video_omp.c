#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "omp.h"

/*
 * Function receive a frame and apply effects to it.
 * Frames are stored using a 3D matrix, but instead of matrix we use
 * a linearized array.
 * You can see the offset of eache pixel_{r,g,b} taken out from the
 * liniarized array
 */
void apply_negative_on_frame(unsigned char *frame, int height, int width) {
	int pixel_r, pixel_g, pixel_b, i;

	#pragma omp parallel num_threads(1)
	{
		#pragma omp for private(i, pixel_r, pixel_g, pixel_b) nowait
		for (i = 0; i < height; i++)
			for (int j = 0; j < width; j++) {
				pixel_r = i * (width * 3) + j * 3 + 0;
				pixel_g = i * (width * 3) + j * 3 + 1;
				pixel_b = i * (width * 3) + j * 3 + 2;
				frame[pixel_r] = 255 - frame[pixel_r];
				frame[pixel_g] = 255 - frame[pixel_g];
				frame[pixel_b] = 255 - frame[pixel_b];
			}
	}
	
}

void apply_sepia_on_frame(unsigned char *frame, int height, int width) {
	int pixel_r, pixel_g, pixel_b, i;

	#pragma omp parallel num_threads(8)
	{
		#pragma omp for private(i, pixel_r, pixel_g, pixel_b) nowait
		for (i = 0; i < height; i++)
			for (int j = 0; j < width; j++) {
				int tr, tg, tb;
				pixel_r = i * (width * 3) + j * 3 + 0;
				pixel_g = i * (width * 3) + j * 3 + 1;
				pixel_b = i * (width * 3) + j * 3 + 2;
				tr = (frame[pixel_r] * 0.393) + (frame[pixel_g] * 0.769) + (frame[pixel_b] * 0.189);
				tg = (frame[pixel_r] * 0.349) + (frame[pixel_g] * 0.686) + (frame[pixel_b] * 0.168);
				tb = (frame[pixel_r] * 0.272) + (frame[pixel_g] * 0.534) + (frame[pixel_b] * 0.131);

				if (tr > 255)
					frame[pixel_r] = 255;
				else
					frame[pixel_r] = tr;
				if (tg > 255)
					frame[pixel_g] = 255;
				else
					frame[pixel_g] = tg;
				if (tb > 255)
					frame[pixel_b] = 255;
				else
					frame[pixel_b] = tb;
			}
	}
	
}

void apply_blur_on_frame (unsigned char *frame, int height, int width) {

    int pixel_r, pixel_g, pixel_b, i;
	int sum_r, sum_g, sum_b;
	float Kernel[3][3] = {{1/9.0, 1/9.0, 1/9.0},
                              {1/9.0, 1/9.0, 1/9.0},
                              {1/9.0, 1/9.0, 1/9.0}
                             };
    #pragma omp parallel num_threads(8)
    {
    	for (int count = 0; count < 5; count++){
    		#pragma omp for private(i, pixel_r, pixel_g, pixel_b, sum_r, sum_g, sum_b) nowait
        	for (i = 1; i < height - 1; i++){
        	        for (int j = 1; j < width - 1; j++) {
				sum_r = 0;
				sum_g = 0;
				sum_b = 0;

				for( int k = -1; k <= 1; k++) {
					for( int y = -1; y <= 1; y++){
						pixel_r = (i - y) * (width * 3) + (j - k) * 3 + 0;
                			        pixel_g = (i - y) * (width * 3) + (j - k) * 3 + 1;
                	        		pixel_b = (i - y) * (width * 3) + (j - k) * 3 + 2;
						sum_r += Kernel[y+1][k+1] * frame[pixel_r];
						sum_g += Kernel[y+1][k+1] * frame[pixel_g];
						sum_b += Kernel[y+1][k+1] * frame[pixel_b];
					}
				}
				frame[pixel_r] = sum_r;
				frame[pixel_g] = sum_g;
				frame[pixel_b] = sum_b;
			}
		}
	}
    }
	
}

int main(int argc, char **argv) {
	FILE *in = NULL, *out = NULL;
	int width, height, count, frame_size;
	unsigned char *frame;
	char *filter;
	clock_t start, end;
	double time;

	/*
	 * Check if the application was start corectly
	 */
	if (argc != 4) {
		printf("Usage : ./filter width height filter\n");
		exit(-1);
	}

	width = atoi(argv[1]);
	height = atoi(argv[2]);
	filter = strdup(argv[3]);
	frame_size = height * width * 3;
	frame = (unsigned char *)calloc(frame_size, sizeof(unsigned char));
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


	/*
	 * Start the timer to get the run time of the sequential algorithm time.
	 * We read frames from video until no one is left. After completing the
	 * read we process the stream and then write the result stream back to
	 * the output pipe
	 */
	if (strcmp(filter, "sepia") == 0) {
		start = omp_get_wtime();
		for (;;) {
			count = fread(frame, 1, frame_size, in);
			if (count != frame_size)
				break;
			apply_sepia_on_frame(frame, height, width);
			fwrite(frame, 1, frame_size, out);
		}
		end = omp_get_wtime();
	}
	else if (strcmp(filter, "negative") == 0) {
		start = omp_get_wtime();
		for (;;) {
			count = fread(frame, 1, frame_size, in);
			if (count != frame_size)
				break;
			apply_negative_on_frame(frame, height, width);
			fwrite(frame, 1, frame_size, out);
		}
		end = omp_get_wtime();
	}
	else if (strcmp(filter, "blur") == 0) {
		start = omp_get_wtime();
                for (;;) {
                        count = fread(frame, 1, frame_size, in);
                        if (count != frame_size)
                                break;
                        apply_blur_on_frame(frame, height, width);
                        fwrite(frame, 1, frame_size, out);
                }
                end = omp_get_wtime();
	}
	else {
		printf("Please choose a filter from sepia/negative/blur\n");
		exit(-1);
	}

	time = (end - start);
	printf("------------------------------------\n");
	printf("| Sequential time is : %lf    |\n", time);
	printf("------------------------------------\n");

	fflush(in);
	pclose(in);
	fflush(out);
	pclose(out);
	return 0;
}
