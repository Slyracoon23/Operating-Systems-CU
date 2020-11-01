#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "util.h"

#define MAX_NAME_LENGTH 1025
#define SHARED_BUFFER_SIZE 20
#define MAX_IP_ADDRESS INET6_ADDRSTRLEN

int main(int argc, char *argv[]);
char** get_valid_input_files(char *input_files[], int potential_num, int *total_num_output);
void *requester_process(void* input_files);
void *requester_helper(void *input_files, int *num_processed);
void *resolver_process(void *args);

typedef struct InputFile {

    pthread_mutex_t file_lock;
    FILE *fd;
    int finished;
    char *filename;

} InputFile;

typedef struct InputFileList {

    pthread_mutex_t file_list_lock;
    int current_file;
    int total_files;
    int num_processed;
    InputFile files[];

} InputFileList;

typedef struct OutputFile {

    pthread_mutex_t file_lock;
    FILE *fd;
    char *filename;

} OutputFile;

typedef struct SharedBuffer {

    pthread_mutex_t buffer_lock;
    pthread_cond_t buffer_full;
    pthread_cond_t buffer_empty;
    int full;
    int empty;
    int requesters_done;
    int total_size;
    int current_position;
    char *shared_array[];

} SharedBuffer;

struct RequesterThreadArgs {

    InputFileList *files;
    SharedBuffer *shared_buffer;
    OutputFile serviced_file;
};

struct ResolverThreadArgs {

    SharedBuffer *shared_buffer;
    OutputFile results_file;
};


char *fgets_unlocked(char *s, int n, FILE *stream);
int fputs_unlocked(const char *s, FILE *stream);


#endif //MULTI_LOOKUP_H
