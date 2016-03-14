#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "queue.h"

#define NUM_THREADS	10
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

queue lookup_queue;

void* read_file(void* f_name)
{
    char* file_name = f_name;
    FILE* inputfp = NULL;
    char errorstr[SBUFSIZE];
    char hostname[SBUFSIZE];

    /* Open Input File */
    inputfp = fopen(file_name, "r");
    if(!inputfp){
        sprintf(errorstr, "Error Opening Input File: %s", file_name);
        perror(errorstr);
    }

    /* Read File and Process*/
    while(fscanf(inputfp, INPUTFS, hostname) > 0){
        // add to queue
        if(queue_push(&lookup_queue, hostname) == QUEUE_FAILURE){
            fprintf(stderr,
                "error: queue_push failed! queue probably full.\n");
        }
    }

    /* Close Input File */
    fclose(inputfp);

    return NULL;
}

void* lookup_name(void* o_file)
{
    char* output_file = o_file;
    char firstipstr[INET6_ADDRSTRLEN];
    char hostname[SBUFSIZE];
    FILE* outputfp = NULL;

    char* address_array;

    // read one from queue
    address_array = queue_pop(&lookup_queue);
    if (address_array == NULL){
        fprintf(stderr, "error: queue_pop failed!\n");
        return NULL;
    }

    /* Open Output File */
    outputfp = fopen(output_file, "w");

    /* Lookup hostname and get IP string */
    if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
       == UTIL_FAILURE){
        fprintf(stderr, "dnslookup error: %s\n", hostname);
        strncpy(firstipstr, "", sizeof(firstipstr));
    }

    /* Write to Output File */
    fprintf(outputfp, "%s,%s\n", hostname, firstipstr);

    /* Close Output File */
    fclose(outputfp);

    return NULL;
}

int main(int argc, char *argv[])
{
    // check args
    // read in the file line by line, one file per thread
    // pull string into the queue (needs to be thread safe)
    // pull strings out of queue, dns lookup (thread safe)
    // print results into a file (thread safe)
    // exit when all
    
    /* Check Arguments */
    if(argc < MINARGS){
        fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
        fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);

        return EXIT_FAILURE;
    }

    if(queue_init(&lookup_queue, 10) == QUEUE_FAILURE){
        fprintf(stderr, "error: queue_init failed!\n");
    }

    int i = 0;
    int j = 0;
    /* Loop Through Input Files */
    for(i=1; i<(argc-1); i++) {
        read_file(argv[i]);
    }

    for(j = 0; j < NUM_THREADS; j++) {
        lookup_name(argv[argc]);
    }

    return EXIT_SUCCESS;
}
