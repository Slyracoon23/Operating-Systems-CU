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




typedef struct InputFile {

    pthread_mutex_t mutex_doc;
    FILE *file_descriptor;
    int finished;
    char *filename;

} InputFile;

typedef struct ListOfInputFiles { 

    pthread_mutex_t mutex_doc_list;
    int current_file;
    int total_files;
    int num_processed;
    InputFile files[];

} ListOfInputFiles;

typedef struct OutputFile {

    pthread_mutex_t mutex_doc; 
    FILE *file_descriptor; 
    char *filename;

} OutputFile;

typedef struct SharedBuffer {

    pthread_mutex_t mutex_buffer;
    pthread_cond_t buffer_full;
    pthread_cond_t buffer_empty;
    int full;
    int empty;
    int requesters_done;
    int total_size;
    int current_position;
    char *shared_array[];

} SharedBuffer;

struct ReqThreadPool {

    ListOfInputFiles *files;
    SharedBuffer *shared_buffer;
    OutputFile serviced_file;
};

struct ResThreadPool {

    SharedBuffer *shared_buffer;
    OutputFile results_file;
};

char** file_validity(char ** files2input, int numberOfPossibleFiles, int *total_files); //done
void *requesters_thread(void* files2input);
void *requesters_thread_helperFunction(void *files2input, int *num_processed);
void *resolvers_thread(void *arguments);




char *fgets_unlocked(char *s, int n, FILE *stream);
int fputs_unlocked(const char *s, FILE *stream);


#endif //MULTI_LOOKUP_H
