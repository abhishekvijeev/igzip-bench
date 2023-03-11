#include <getopt.h>
#include <isa-l.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <x86intrin.h>

// Declarations

#define OPTARGS "hl:m:t:n:w:i:"
#define xstr(a) str(a)
#define str(a) #a

enum benchmark_type {
	LATENCY,
	THROUGHPUT,
};

enum compression_mode {
	STATELESS,
	STATEFUL
};

#define DEFAULT_COMPRESSION_LEVEL 0
#define DEFAULT_INPUT_BUF_SIZE 1024
#define DEFAULT_COMPRESSION_WINDOW_SIZE 15
#define DEFAULT_COMPRESSION_MODE STATELESS
#define DEFAULT_BENCHMARK_TYPE LATENCY
#define DEFAULT_BENCHMARK_ITERATIONS 1000
#define MILLION 1E3
#define BILLION 1E9

struct benchmark_info {
	uint32_t level;
	size_t input_buf_size;
	uint32_t window_size;
	enum compression_mode mode;
	enum benchmark_type type;
	uint32_t iterations;
};

static struct benchmark_info global_benchmark_info;

int level_buf_sizes[4] = {
#ifdef ISAL_DEF_LVL0_DEFAULT
	ISAL_DEF_LVL0_DEFAULT,
#else
	0,
#endif

#ifdef ISAL_DEF_LVL1_DEFAULT
	ISAL_DEF_LVL1_DEFAULT,
#else
	0,
#endif

#ifdef ISAL_DEF_LVL2_DEFAULT
	ISAL_DEF_LVL2_DEFAULT,
#else
	0,
#endif

#ifdef ISAL_DEF_LVL3_DEFAULT
	ISAL_DEF_LVL3_DEFAULT,
#else
	0,
#endif
};

void set_default_benchmark_options(void);
void parse_cmd_line_args(int argc, char **argv);
int usage(void);
void print_benchmark_options(void);
uint64_t measure_stateless_comp_latency(void);
void fill_buffer_rand(uint8_t *buf, size_t size);

// Function definitions
void set_default_benchmark_options(void)
{
	global_benchmark_info.level = DEFAULT_COMPRESSION_LEVEL;
	global_benchmark_info.input_buf_size = DEFAULT_INPUT_BUF_SIZE;
	global_benchmark_info.window_size = DEFAULT_COMPRESSION_WINDOW_SIZE;
	global_benchmark_info.mode = DEFAULT_COMPRESSION_MODE;
	global_benchmark_info.type = DEFAULT_BENCHMARK_TYPE;
	global_benchmark_info.iterations = DEFAULT_BENCHMARK_ITERATIONS;
}

void parse_cmd_line_args(int argc, char **argv) 
{
	char c;
	while ((c = getopt(argc, argv, OPTARGS)) != -1) {
		switch (c) {
		case 'l':
			global_benchmark_info.level = atoi(optarg);
			if (global_benchmark_info.level > ISAL_DEF_MAX_LEVEL) {
				printf("Unsupported compression level\n");
				exit(0);
			}
			break;
		case 'm':
			if (strcasecmp(optarg, "stateless") == 0) {
				global_benchmark_info.mode = STATELESS;
			} else if (strcasecmp(optarg, "stateful") == 0) {
				global_benchmark_info.mode = STATEFUL;
			} else {
				usage();
			}
			break;
		case 't':
			if (strcasecmp(optarg, "latency") == 0) {
				global_benchmark_info.mode = LATENCY;
			} else if (strcasecmp(optarg, "throughput") == 0) {
				global_benchmark_info.mode = THROUGHPUT;
			} else {
				usage();
			}
			break;
		case 'n':
			global_benchmark_info.input_buf_size = atoi(optarg);
			if (global_benchmark_info.input_buf_size < 0) {
				printf("Unsupported input buffer size\n");
				exit(0);
			}
			break;
		case 'w':
			global_benchmark_info.window_size = atoi(optarg);
			if (global_benchmark_info.window_size < 0) {
				printf("Unsupported window_size size\n");
				exit(0);
			}
			break;
		case 'i':
			global_benchmark_info.iterations = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}
}

void fill_buffer_rand(uint8_t *buf, size_t buf_size)
{
	if (buf == NULL) {
		fprintf(stderr, "Passed buffer is empty");
		exit(0);
	}

	for (int i = 0; i < buf_size; i++) {
		buf[i] = rand();
	}
}

uint64_t measure_stateless_comp_latency(void)
{
	uint8_t *input_buf;
	uint8_t *output_buf;
	uint8_t *level_buf;
	size_t input_buf_size;
	size_t output_buf_size;
	size_t level_buf_size;
	struct isal_zstream stream;
	unsigned int rdtscp_dummy;
	// uint64_t start_time, end_time, latency;
	int error;
	struct timespec start_time, end_time;
	uint64_t latency;

	input_buf_size = global_benchmark_info.input_buf_size;
	output_buf_size = ISAL_DEF_MAX_HDR_SIZE + input_buf_size;
	level_buf_size = level_buf_sizes[global_benchmark_info.level];

	input_buf = (uint8_t *)malloc(input_buf_size * sizeof (uint8_t));
	if (input_buf == NULL) {
		fprintf(stderr, "Can't allocate memory for input buffer\n");
		exit(0);
	}

	output_buf = (uint8_t *)malloc(output_buf_size * sizeof (uint8_t));
	if (output_buf == NULL) {
		fprintf(stderr, "Can't allocate memory for output buffer\n");
		exit(0);
	}

	level_buf = (uint8_t *)malloc(level_buf_size * sizeof (uint8_t));
	if (output_buf == NULL) {
		fprintf(stderr, "Can't allocate memory for level buffer\n");
		exit(0);
	}

	fill_buffer_rand(input_buf, input_buf_size);

	isal_deflate_init(&stream);
	stream.end_of_stream = 1;	/* Do the entire file at once */
	stream.flush = NO_FLUSH;
	stream.next_in = input_buf;
	stream.avail_in = input_buf_size;
	stream.next_out = output_buf;
	stream.avail_out = output_buf_size;
	stream.level = global_benchmark_info.level;
	stream.level_buf = level_buf;
	stream.level_buf_size = level_buf_size;
	stream.hist_bits = global_benchmark_info.window_size;

	/* Compress stream */
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	// start_time = __rdtscp(&rdtscp_dummy);
	error = isal_deflate_stateless(&stream);
	// end_time = __rdtscp(&rdtscp_dummy);
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	// latency = (end_time.tv_sec - start_time.tv_sec)
	// 	+ (end_time.tv_nsec - start_time.tv_nsec)
	// 	/ BILLION;

	latency = (end_time.tv_nsec - start_time.tv_nsec) / MILLION;
	/* Verify compression success */
	if (error) {
		fprintf(stderr, "error %d\n", error);
		exit(0);
	}


	free(input_buf);
	free(output_buf);
	free(level_buf);

	return latency;
}

int usage(void)
{
	fprintf(stderr,
		"Usage: igzip-bench [options]\n"
		"  -h \t\t\t\t\t help, print this message\n"
		"  -l \t [" xstr(ISAL_DEF_MIN_LEVEL) "-" xstr(ISAL_DEF_MAX_LEVEL) "]"
		" \t\t\t\t compression level to test (default = 0) \n"
		"  -m \t [stateless/stateful] \t\t compression mode to test (default = stateless)\n"
		"  -t \t [latency/throughput] \t\t compression benchmark type (default = latency)\n"
		"  -n \t <size> \t\t\t input buffer size (default = 1024)\n"
		"  -w \t [9-15] \t\t\t log base 2 size of history window (default = 16)\n"
		"  -i \t <iter> \t\t\t number of test iterations (default = 1000)\n");
	exit(0);
}

void print_benchmark_options(void)
{
	printf("Benchmark options:\n\n");
	printf("Compression Mode: %s\n", 
		(global_benchmark_info.mode == STATELESS)
		? "STATELESS" 
		: "STATEFUL");
	printf("Benchmark Type: %s\n", 
		(global_benchmark_info.type == LATENCY)
		? "LATENCY" 
		: "THROUGHPUT");
	printf("Compression Level: %u\n", global_benchmark_info.level);
	printf("Compression Window Size: %u\n", global_benchmark_info.window_size);
	printf("Input Buffer Size: %lu\n", global_benchmark_info.input_buf_size);
	printf("\n");
}

int main(int argc, char **argv)
{
	uint64_t total_cycles;

	srand(time(NULL));

	set_default_benchmark_options();

	parse_cmd_line_args(argc, argv);

	print_benchmark_options();

	if (global_benchmark_info.mode == STATELESS &&
	  global_benchmark_info.type == LATENCY) {
		total_cycles = 0;
		for (int i = 0; i < global_benchmark_info.iterations; i++) {
			total_cycles += measure_stateless_comp_latency();
		}
		printf("Average cycles: %lf\n",
			total_cycles / (global_benchmark_info.iterations * 1.0));
	}

	return 0;
}
