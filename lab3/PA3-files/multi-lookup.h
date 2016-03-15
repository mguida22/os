#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "queue.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

queue lookup_queue;

void* request_handler();
void* resolver_handler();
void read_file(char*);
int lookup_name(char*);

pthread_mutex_t req_lock;
pthread_mutex_t res_lock;

int current = 1;
int done = 0;
int global_argc;
char** global_argv;
