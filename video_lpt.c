#include "video_lpt.h"

/*
 * Function receive a frame and apply effects to it.
 * Frames are stored using a 3D matrix, but instead of matrix we use
 * a linearized array.
 * You can see the offset of eache pixel_{r,g,b} taken out from the
 * liniarized array
 */
void apply_negative_on_frame(unsigned char *frame, int frame_size, int width) {
	int pixel_r, pixel_g, pixel_b, height;

	height = frame_size / (width * 3);

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) {
			pixel_r = i * (width * 3) + j * 3 + 0;
			pixel_g = i * (width * 3) + j * 3 + 1;
			pixel_b = i * (width * 3) + j * 3 + 2;
			frame[pixel_r] = 255 - frame[pixel_r];
			frame[pixel_g] = 255 - frame[pixel_g];
			frame[pixel_b] = 255 - frame[pixel_b];
		}
}

void apply_sepia_on_frame(unsigned char *frame, int frame_size, int width) {
	int pixel_r, pixel_g, pixel_b, height;

	height = frame_size / (width * 3);

	for (int i = 0; i < height; i++)
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

void apply_blur_on_frame (unsigned char *frame, int frame_size, int width) {
	int pixel_r, pixel_g, pixel_b, height;
	int sum_r, sum_g, sum_b;
	unsigned char *dst;
	float Kernel[3][3] = {{1/9.0, 1/9.0, 1/9.0},
							  {1/9.0, 1/9.0, 1/9.0},
							  {1/9.0, 1/9.0, 1/9.0}
							 };

	height = frame_size / (width * 3);
	dst = (unsigned char *)calloc(frame_size, sizeof(unsigned char));

	for (int i = 0; i < height; i++){
				for (int j = 0; j < width; j++) {
			pixel_r = i * (width * 3) + j * 3 + 0;
			pixel_g = i * (width * 3) + j * 3 + 1;
			pixel_b = i * (width * 3) + j * 3 + 2;
			dst[pixel_r] = frame[pixel_r];
			dst[pixel_g] = frame[pixel_g];
			dst[pixel_b] = frame[pixel_b];
		}
	}

	for(int count = 0; count < 5; count ++){
			for (int i = 1; i < height - 1; i++){
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
				dst[pixel_r] = sum_r;
				dst[pixel_g] = sum_g;
				dst[pixel_b] = sum_b;
			}
		}
	}
	for (int i = 0; i < height; i++){
				for (int j = 0; j < width; j++) {
			pixel_r = i * (width * 3) + j * 3 + 0;
			pixel_g = i * (width * 3) + j * 3 + 1;
			pixel_b = i * (width * 3) + j * 3 + 2;
			frame[pixel_r] = dst[pixel_r];
			frame[pixel_g] = dst[pixel_g];
			frame[pixel_b] = dst[pixel_b];
		}
	}
}

/*
 * Function called by each thread at start. It receive the struct that contain
 * the chunk from the frame to be processed and return a processed frame
 */
void *thread_frame_process(void *th_info) {
	thread_info *info = (thread_info *)th_info;	// cast infos

	if (info->filter_id == NEGATIVE)
		apply_negative_on_frame(info->thread_frame + info->displs, info->send_count, info->width);
	else if (info->filter_id == BLUR)
		apply_blur_on_frame(info->thread_frame + info->displs, info->send_count, info->width);
	else if (info->filter_id == SEPIA)
		apply_sepia_on_frame(info->thread_frame + info->displs, info->send_count, info->width);

	return NULL;
}

int main (int argc, char *argv[])
{
	int *send_counts, *displs;
	int width, height, frame_size, frames_nr, threads_num, filter_id;
	unsigned char *frame;
	char *filter;
	FILE *in, *out;
	clock_t start, end;
	double time;
	pthread_t *threads;
	thread_info **infos;

	/*
	 * Check if the application was start corectly
	 */
	if (argc != 6) {
		printf("Usage : ./filter width height frames_number filter threads_num\n");
		exit(-1);
	}

	threads_num = atoi(argv[5]);
	send_counts = (int *)calloc(threads_num, sizeof(int));
	displs = (int *)calloc(threads_num, sizeof(int));
	width = atoi(argv[1]);
	height = atoi(argv[2]);
	frames_nr = atoi(argv[3]);
	filter = strdup(argv[4]);
	frame_size = height * width * 3;

	/*
	 * Open input and output pipes for video frames communication.
	 * After opening the input pipe, it will read a frame from the video, check
	 * it's size and make the properly division to the each process.
	 */ 
	frame = (unsigned char *)calloc(frame_size, sizeof(unsigned char));
	if (!frame)
		exit(-1);

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

	/* Compute the number of bytes each thread will process */
	/* Compute index in frame to start sending bytes to process */
	for (int i = 0; i < threads_num; i++) {
		if (i != threads_num - 1) {
			send_counts[i] = frame_size / threads_num;
			displs[i] = i * (frame_size / threads_num);
		} else {
			/* last thread gets what's remaining */
			send_counts[i] = frame_size / threads_num + frame_size % threads_num;
			displs[i] = i * (frame_size / threads_num);
		}
	}

	/* Alloc memory for all threads */
	threads = (pthread_t *)calloc(threads_num, sizeof(pthread_t));
	if (!threads) {
		printf("Failt to alloc threads memory! Exit !\n");
		exit (-1);
	}

	/* Alloc memory for each thread structure */
	infos = (thread_info **) calloc(threads_num, sizeof(thread_info*));
	for (int i = 0; i < threads_num; i++)
		infos[i] = (thread_info *)calloc(1, sizeof(thread_info));

	/* Inspect filter to get the id */
	if (strcmp(filter, "sepia") == 0)
		filter_id = SEPIA;
	else if (strcmp(filter, "negative") == 0)
		filter_id = NEGATIVE;
	else if (strcmp(filter, "blur") == 0)
		filter_id = BLUR;
	else {
		printf("The filter is not correct. Exit!\n");
		exit(-1);
	}

	/* Complete standalone data in each thread structure */
	for (int i = 0; i < threads_num; i++) {
		infos[i]->filter_id = filter_id;
		infos[i]->send_count = send_counts[i];
		infos[i]->displs = displs[i];
		infos[i]->width = width;
	}

	for (int  frm_nr = 0; frm_nr < frames_nr; frm_nr++) {
		fread(frame, 1, frame_size, in);

		/* Add current frame to each thread */
		for (int i = 0; i < threads_num; i++)
			infos[i]->thread_frame = frame;

		for (int i = 0; i < threads_num; i++)
			pthread_create(&threads[i], NULL, thread_frame_process, infos[i]);

		for (int i = 0; i < threads_num; i++)
			pthread_join(threads[i], NULL);
		/* each process perform changes of chunk of data it receive */

		fwrite(frame, 1, frame_size, out);
	}

	/* Free memory */
	free (threads);
	free(send_counts);
	free(displs);
	free(frame);
	for (int i = 0; i < threads_num; i++)
		free(infos[i]);
	free(infos);

	return 0;
}
