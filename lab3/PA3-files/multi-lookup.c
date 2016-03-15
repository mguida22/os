#include "multi-lookup.h"

void* request_handler() {
  // while we have input files
  int local = 0;
  while(current < (global_argc-1)) {
    pthread_mutex_lock(&req_lock);

    local = current;
    current++;

    pthread_mutex_unlock(&req_lock);

    // don't read inputs we don't have, just cause we have threads
    if (local < (global_argc-1)) {
      read_file(global_argv[local_argv]);
    }
  }

  return NULL;
}

void* resolver_handler() {
  char* output_file = global_argv[global_argc-1];

  int thread_count = 0;

  while(done == 0 || queue_is_empty(&lookup_queue) == 0) {
    thread_count += lookup_name(output_file);
  }
}

void* read_file(void* f_name)
{
  FILE* inputfp = NULL;
  char errorstr[SBUFSIZE];
  char hostname[SBUFSIZE];
  char* host_address;

  if (strcmp(f_name, "") == 0) {
    return;
  }

  inputfp = fopen(f_name, "r");
  if (!inputfp) {
    sprintf(errorstr, "")
  }

  // Open Input File
  inputfp = fopen(file_name, "r");
  if(!inputfp){
    sprintf(errorstr, "Error Opening Input File: %s", file_name);
    perror(errorstr);

    return;
  }

  int thread_count = 0;
  // Read File and Process
  while(fscanf(inputfp, INPUTFS, hostname) > 0){
    // save in memory
    char* host_address = (char*)malloc(strlen(hostname) + 1)
    host_address = hostname;

    // add to queue
    while(queue_push(&lookup_queue, host_address) == QUEUE_FAILURE) {
      // wait if couldn't add (queue full)
      usleep(random() % 100);
    }

    thread_count++;
  }

  // display whats happening. show howmany hostnames added
  printf("Requester added %d hostname to queue.\n", thread_count);

  fclose(inputfp);

  return NULL;
}

void* lookup_name(void* o_file)
{
  char firstipstr[INET6_ADDRSTRLEN];
  FILE* outputfp = NULL;
  char* address;
  int invalid_dns = 0;

  // open file to append our results
  outputfp = fopen(o_file);
  if (!outputfp) {
    perror("Error Opening Output File");

    return 0;
  }

  // take next off the queue
  if ((address = queue_pop(&lookup_queue)) == NULL) {
    // if nothing is here to lookup, return thread to use again
    return 0;
  }

  // dnslookup our address
  if(dnslookup(address, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE) {
    fprintf(stderr, "dnslookup error: %s\n", hostname);
    strncpy(firstipstr, "", sizeof(firstipstr));

    // mark that it was invalid
    invalid_dns = 1;
  }

  pthread_mutex_lock(&res_lock);

  if (invalid_dns == 0) {
    // add the information to the file
    fprintf(outputfp, "%s,%s\n", address, firstipstr);
  } else {
    // add information to file (without ip address)
    fprintf(outputfp, "%s\n", address);
  }

  pthread_mutex_unlock(&res_lock);

  // cleanup
  fclose(outputfp);
  free(address);

  return 1;
}

int main(int argc, char *argv[]) {
  // could be any number of threads
  int req_thread_count = 5;

  // could also be anything, but this is for the E.C.
  int res_thread_count = sysconf(_SC_NPROCESSORS_ONLN);

  // could again be anything
  int queue_size = 20;
  int i;
  int rc;

  // Check Arguments
  if(argc < MINARGS) {
    fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
    fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);

    return EXIT_FAILURE;
  }

  // save globally for later
  global_argc = argc;
  global_argv = argv;

  // create file so threads can write to it later
  FILE* output_file = NULL;
  output_file = fopen(argv[argc - 1], "w");
  fclose(output_file);

  // create queue to hold names to resolve
  if (queue_init(&lookup_queue, queue_size) == QUEUE_FAILURE) {
    fprintf(stderr, "error: queue_init failed!\n");

    return EXIT_FAILURE;
  }

  if (pthread_mutex_init(&req_lock, NULL)) {
    fprintf(stderr, "req_lock pthread_mutex_init failed\n");

    return EXIT_FAILURE;
  }

  if (pthread_mutex_init(&res_lock, NULL)) {
    fprintf(stderr, "res_lock pthread_mutex_init failed\n");

    return EXIT_FAILURE;
  }

  // create specified number of requester threads
  pthread_t req_thread_pool[req_thread_count];

  for (i = 0; i < req_thread_count; i++) {
    rc = pthread_create(&(req_thread_pool[i]), NULL, request_handler, NULL);

    if (rc) {
      printf("ERROR: [requester] return code from pthread_create() is %d\n", rc);

      return EXIT_FAILURE;
    }
  }

  // create specified number of resolver threads
  pthread_t res_thread_pool[res_thread_count];

  for (i = 0; i < res_thread_count; i++) {
    rc = pthread_create(&(res_thread_pool[i]), NULL, resolver_handler, NULL);

    if (rc) {
      printf("ERROR: [resolver] return code from pthread_create() is %d\n", rc);

      return EXIT_FAILURE;
    }
  }

  // when all of the resolvers are done, set done to done!
  for (i = 0; i < req_thread_count; i++) {
    pthread_join(req_thread_pool[i], NULL);
  }
  done = 1;

  // when all the requesters are done, destroy and return success!
  for (i = 0; i < res_thread_count; i++) {
    pthread_join(res_thread_pool[i], NULL);
  }
  pthread_mutex_destroy(&req_lock);
  pthread_mutex_destroy(&res_lock);

  return EXIT_SUCCESS;
}
