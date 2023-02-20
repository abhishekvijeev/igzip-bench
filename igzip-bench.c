#include <getopt.h>
#include <isa-l.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

// Declarations

#define OPTARGS "hl:m:t:n:w"
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
#define DEFAULT_BUF_SIZE 8
#define DEFAULT_COMPRESSION_WINDOW_SIZE 0
#define DEFAULT_COMPRESSION_MODE STATELESS
#define DEFAULT_BENCHMARK_TYPE LATENCY

struct benchmark_info {
	uint32_t level;
	uint32_t buf_size;
	uint32_t window_size;
	enum compression_mode mode;
	enum benchmark_type type;
};

static struct benchmark_info global_benchmark_info;

void set_default_benchmark_options(void);
void parse_cmd_line_args(int argc, char **argv);
int usage(void);
void print_benchmark_options(void);

// Function definitions
void set_default_benchmark_options(void)
{
	global_benchmark_info.level = DEFAULT_COMPRESSION_LEVEL;
	global_benchmark_info.buf_size = DEFAULT_BUF_SIZE;
	global_benchmark_info.window_size = DEFAULT_COMPRESSION_WINDOW_SIZE;
	global_benchmark_info.mode = DEFAULT_COMPRESSION_MODE;
	global_benchmark_info.type = DEFAULT_BENCHMARK_TYPE;
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
			global_benchmark_info.buf_size = atoi(optarg);
			if (global_benchmark_info.buf_size < 0) {
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
		case 'h':
		default:
			usage();
			break;
		}
	}
}

int usage(void)
{
	fprintf(stderr,
		"Usage: igzip-bench [options]\n"
		"  -h \t\t\t\t\t help, print this message\n"
		"  -l \t [" xstr(ISAL_DEF_MIN_LEVEL) "-" xstr(ISAL_DEF_MAX_LEVEL) "]"
		" \t\t\t\t compression level to test \n"
		"  -m \t [stateless/stateful] \t\t compression mode to test\n"
		"  -t \t [latency/throughput] \t\t compression benchmark type\n"
		"  -n \t <size> \t\t\t input buffer size\n"
		"  -w \t [9-15] \t\t\t log base 2 size of history window\n");
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
	printf("Input Buffer Size: %u\n", global_benchmark_info.buf_size);
	printf("\n");
}

int main(int argc, char **argv)
{
	set_default_benchmark_options();

	parse_cmd_line_args(argc, argv);

	print_benchmark_options();

	return 0;
}



