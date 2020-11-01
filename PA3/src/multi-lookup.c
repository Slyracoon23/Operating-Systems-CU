#include <stdio.h>

#include "multi-lookup.h"
#include "util.h"

int main(int argc, char *argv[]) {
	//Intitalize time structs
	struct timeval start, end;
	//Get a start time
	gettimeofday(&start, NULL);

	//Ensure that the minimum number of arguments were specified
	if (argc < 6) {
		fprintf(stderr, "ERROR: invalid arguments.\nUsage: ./multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]\n");
		exit(EXIT_FAILURE);
	}

	//Get the number of requesters and resolvers
	int num_requesters = atoi(argv[1]);
	int num_resolvers = atoi(argv[2]);

	//Check the received thread numbers
	if (num_requesters <= 0) {
		fprintf(stderr, "ERROR: There must be at least one requester thread.\n");
		exit(EXIT_FAILURE);
	}
	if (num_resolvers <= 0) {
		fprintf(stderr, "ERROR: There must be at least one resolver thread.\n");
		exit(EXIT_FAILURE);
	}

	//Get the requester and resolver log files
	char *requester_log_file = argv[3];
	char *resolver_log_file = argv[4];

	//Check the existence of the requester and resolver log files
	if (access(requester_log_file, F_OK | W_OK) == -1) {
		fprintf(stderr, "ERROR: The requester log file does not exist.\n");
		exit(EXIT_FAILURE);
	}
	if (access(resolver_log_file, F_OK | W_OK) == -1) {
		fprintf(stderr, "ERROR: The resolver log file does not exist.\n");
		exit(EXIT_FAILURE);
	}

	//Get the number of actually valid input files, and a list of those valid input paths
	int valid_file_tot;
	char **valid_file_names = get_valid_input_files(argv + 5, argc - 5, &valid_file_tot);

	//Initialize the InputFileList struct
	InputFileList *files;
	files = malloc(sizeof(*files) + sizeof(InputFile) * valid_file_tot);
	files->file_list_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	files->total_files = valid_file_tot;
	files->current_file = 0;
	files->num_processed = 0;

	//Initialize an InputFile struct for each valid file and add it to the InputFileList struct list
	int i;
	for(i=0; i<valid_file_tot; i++){
		InputFile file;
		file.file_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
		file.finished = 0;
		file.filename = valid_file_names[i];
		file.fd = fopen(file.filename, "r");
		files->files[i] = file; // copy assignment from stack -> heap
	}

	////////////////////////////////////////////////////////// All struct of the Files have been created ///////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////// File Initialization Finished ////////////////////////////////////////////////////////////////

	//Create the shared buffer
	SharedBuffer *shared_buffer;
	shared_buffer = malloc(sizeof(*shared_buffer) + sizeof(char*) * SHARED_BUFFER_SIZE);

	shared_buffer->buffer_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	shared_buffer->buffer_full = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
	shared_buffer->buffer_empty = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
	shared_buffer->full = 0;
	shared_buffer->empty = 1;
	shared_buffer->requesters_done = 0;
	shared_buffer->total_size = SHARED_BUFFER_SIZE;
	shared_buffer->current_position = 0; //current_position in buffer


	//////////////////////////////////////////////////////// Shared Buffer initialization Finished /////////////////////////////////////////////////////

	//Create serviced output file
	OutputFile serviced_file;
	serviced_file.filename = requester_log_file;
	serviced_file.fd = fopen(requester_log_file, "w");
	serviced_file.file_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

	struct RequesterThreadArgs requester_args;
	requester_args.files = files;
	requester_args.shared_buffer = shared_buffer;
	requester_args.serviced_file = serviced_file;

	//////////////////////////////////////////////////////// Serviced Output initialization Finished /////////////////////////////////////////////////////


	//Create an array to store the thread IDs of the requester threads
	pthread_t *tids_requester = malloc(sizeof(pthread_t) * num_requesters);

	//Start all of the requester threads
	for (i = 0; i < num_requesters; i++) {
		pthread_create(&tids_requester[i], NULL, requester_process, (void *)&requester_args);
	}

	//Create results output file
	OutputFile results_file;
	results_file.file_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	results_file.filename = resolver_log_file;
	results_file.fd = fopen(resolver_log_file, "w");

	struct ResolverThreadArgs resolver_args;
	resolver_args.shared_buffer = shared_buffer;
	resolver_args.results_file = results_file;

	//Create an array to store the thread IDs of the resolver threads
	pthread_t *tids_resolver = malloc(sizeof(pthread_t) * num_resolvers);

	//Start all of the resolver threads
	for (i = 0; i < num_resolvers; i++) {

		pthread_create(&tids_resolver[i], NULL, resolver_process, (void *)&resolver_args);
	}

	//Join all of the requester threads
	for (i = 0; i < num_requesters; i++) {

		if(pthread_join(tids_requester[i], 0)){
			printf("ERROR: Could not join thread %ld\n",tids_requester[i]);
		}
	}

	pthread_mutex_lock(&shared_buffer->buffer_lock);
	shared_buffer->requesters_done = 1;
	pthread_cond_broadcast(&shared_buffer->buffer_empty);
	pthread_mutex_unlock(&shared_buffer->buffer_lock);

	//Join all of the resolver threads
	for (i = 0; i < num_resolvers; i++) {

		pthread_join(tids_resolver[i], 0);
	}

	//Close all of the open input files
	for(i = 0; i <  valid_file_tot; i++) {

		fclose(files->files[i].fd);
	}

	//Close serviced and results files
	fclose(serviced_file.fd);
	fclose(results_file.fd);

	//Free all of the dynamically allocated arrays
	free(valid_file_names);
	free(files);
	free(shared_buffer);
	free(tids_requester);
	free(tids_resolver);

	//Get end time
	gettimeofday(&end, NULL);
	//Present elapsed time to user
	printf ("Total run time: %f seconds\n", (double) (end.tv_usec - start.tv_usec) / 1000000 + (double) (end.tv_sec - start.tv_sec));

	exit(EXIT_SUCCESS);
}

char * output_request2sharedBuffer(int count){
	int length_str = snprintf(NULL, 0, "Thread %ld serviced %i files.\n", pthread_self(), count );
	
	char * out = malloc(length_str + 1);

	snprintf(out,length_str+1,"Thread %ld serviced %i files.\n", pthread_self(), count  );
	
	return out;
}



void *requester_process(void* input_files) {

	struct RequesterThreadArgs *requester_args = (struct RequesterThreadArgs *) input_files;
	int count = 0;

	// requester threads doing work and putting domain names in shared buffer
	requester_helper(input_files, &count);


	// int length = snprintf( NULL, 0, "%i", count); // Fancy way of checking the number of charcters needed for count
	// char* count_str = malloc( sizeof(char) * (length + 1));
	// snprintf(count_str, sizeof(char) * (length + 1), "%i", count);

	// length = snprintf( NULL, 0, "%ld", syscall(SYS_gettid)); // TODO use pthread_self
	// char* tid_str = malloc( sizeof(char) * (length + 1));
	// snprintf(tid_str, sizeof(char) * (length + 1), "%ld", syscall(SYS_gettid));

	// char* out = malloc(sizeof(char) * (26 + strlen(count_str) + strlen(tid_str)));
	// strcpy(out, "Thread ");
	// strcat(out, tid_str);
	// strcat(out, " serviced ");
	// strcat(out, count_str);
	// strcat(out, " files.\n");

	// free(count_str);
	// free(tid_str);

	char * out = output_request2sharedBuffer(count);

	pthread_mutex_lock(&requester_args->serviced_file.file_lock);
	// fputs_unlocked -> fputs
	fputs_unlocked(out, requester_args->serviced_file.fd);
	pthread_mutex_unlock(&requester_args->serviced_file.file_lock);
	printf("%s",out);

	free(out);

	return 0;
}

InputFile * find_unfinished_file(InputFileList * args){
	if(args->num_processed == args->total_files){
		fprintf(stderr, "Can't have processed all files" );
		exit(EXIT_FAILURE);
	}
	while (args->files[args->current_file].finished) {

			if (args->current_file == args->total_files - 1) {
				//reset current_file to zero
				args->current_file = 0;
			}
			else {

				args->current_file++;
			}
		}

		return &(args->files[args->current_file]);
}

void *requester_helper(void *input_files, int* num_processed) {

	struct RequesterThreadArgs *requester_args = (struct RequesterThreadArgs *) input_files;

	InputFileList *args = requester_args->files;
	SharedBuffer *shared_buffer = requester_args->shared_buffer;

	pthread_mutex_lock(&args->file_list_lock);
	//If all the files are processed then exit
	if (args->num_processed == args->total_files) {

		pthread_mutex_unlock(&args->file_list_lock);

		return 0;
	}
	// there must be an unfinished file because the mutex is locked
	else {

		//Find an unfinished file
		// while (args->files[args->current_file].finished) {

		// 	if (args->current_file == args->total_files - 1) {
		// 		//reset current_file to zero
		// 		args->current_file = 0;
		// 	}
		// 	else {

		// 		args->current_file++;
		// 	}
		// }
		// save pointer to file_to_process
		InputFile *file_to_process = find_unfinished_file(args);
		(*num_processed)++;

		// increment current_file by 1 like a circular buffer
		args->current_file = (args->current_file + 1) % args->total_files;
		// if (args->current_file == args->total_files - 1) {

		// 	args->current_file = 0;
		// }
		// else {

		// 	args->current_file++;
		// }

		pthread_mutex_unlock(&args->file_list_lock);

		while (1) {

			pthread_mutex_lock(&file_to_process->file_lock);

			// TODO check why malloc and not just stack
			char *hostname = (char *) malloc((MAX_NAME_LENGTH) * sizeof(char));
			//
			// fgets_unlocked ->`fgets` WHICH IS MT-SAFE !!! LET ME USE FGETS!!!
			// READ UNTIL EOF
			if (fgets_unlocked(hostname, MAX_NAME_LENGTH, file_to_process->fd)) {

				pthread_mutex_unlock(&file_to_process->file_lock);

				if (strlen(hostname) <= MAX_NAME_LENGTH - 1 && hostname[strlen(hostname) - 1] == '\n') {

					hostname[strlen(hostname) - 1] = '\0';
				}
				/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// Lock Shared memory buffer and do nessary operations
				pthread_mutex_lock(&shared_buffer->buffer_lock);
			
				while (shared_buffer->full) {

					pthread_cond_wait(&shared_buffer->buffer_full, &shared_buffer->buffer_lock);
				}

				// Put hostname in buffer at position current_position
				shared_buffer->shared_array[shared_buffer->current_position] = hostname;

				// check if buffer is full
				if (shared_buffer->current_position == shared_buffer->total_size - 1) {
					// set shared_buffer to full
					shared_buffer->full = 1;
				}
				// Increment shard_buffer current_position by 1
				shared_buffer->current_position++;

				//check if buffer was empty	
				if (shared_buffer->current_position - 1 == 0) {
					//set shared_buffer empty as false
					shared_buffer->empty = 0;
					pthread_cond_broadcast(&shared_buffer->buffer_empty);
				}

				pthread_mutex_unlock(&shared_buffer->buffer_lock);
				////////////////////////////////////////////////////////////////////////////////////////////////////
			}
			else {
				// free memory space
				free(hostname);
				
				// if file_to_process is not finished
				if (!file_to_process->finished) {
					// Set to finished
					file_to_process->finished = 1;

					pthread_mutex_unlock(&file_to_process->file_lock);

					pthread_mutex_lock(&args->file_list_lock);

					args->num_processed++;

					pthread_mutex_unlock(&args->file_list_lock);
					
					// restart the process and increment num_process by 1
					requester_helper(input_files, num_processed++);
				}
				else {

					pthread_mutex_unlock(&file_to_process->file_lock);
				}

				break;
			}
		}
	}

	return 0;
}

void *resolver_process(void *args) {

	struct ResolverThreadArgs *resolver_args = (struct ResolverThreadArgs *) args;

	SharedBuffer *shared_buffer = resolver_args->shared_buffer;

	while (1) {
		//Acquire buffer lock
		pthread_mutex_lock(&shared_buffer->buffer_lock);
		//Wait while the shared buffer is empty and the requesters are not done
		while (shared_buffer->empty && !shared_buffer->requesters_done) {
			pthread_cond_wait(&shared_buffer->buffer_empty, &shared_buffer->buffer_lock);
		}
		//Release the buffer lock if the buffer is empty and requesters are done
		if (shared_buffer->empty && shared_buffer->requesters_done){
			pthread_mutex_unlock(&shared_buffer->buffer_lock);
			break;
		}
		// take hostname and store it in char* hostname
		char *hostname = shared_buffer->shared_array[shared_buffer->current_position - 1];
		//set shared_buffer current_position - 1 pointer to NULL
		shared_buffer->shared_array[shared_buffer->current_position - 1] = NULL;
		
		// if current_position == 1 then it is empty
		if (shared_buffer->current_position == 1) {

			shared_buffer->empty = 1;
		}
		//Decrement current_possition in shared buffer
		shared_buffer->current_position--;
		//if it was full
		if (shared_buffer->current_position == SHARED_BUFFER_SIZE - 1) {

			shared_buffer->full = 0;
			pthread_cond_broadcast(&shared_buffer->buffer_full);
		}

		pthread_mutex_unlock(&shared_buffer->buffer_lock);

		char ip[MAX_IP_ADDRESS]; // add max ip length

		if (dnslookup(hostname, ip, MAX_IP_ADDRESS ) == UTIL_SUCCESS) {

			char* out = malloc(sizeof(char) * (strlen(hostname) + strlen(ip) + 3));
			strcpy(out, hostname);
			strcat(out, ",");
			strcat(out, ip);
			strcat(out, "\n");

			pthread_mutex_lock(&resolver_args->results_file.file_lock);
			fputs_unlocked(out, resolver_args->results_file.fd); //change fputs_unlocked -> fputs
			pthread_mutex_unlock(&resolver_args->results_file.file_lock);

			free(out);

		}
		else {
			char* out = malloc(sizeof(char) * (strlen(hostname) + 3));
			strcpy(out, hostname);
			strcat(out, ",");
			strcat(out, "\n");

			pthread_mutex_lock(&resolver_args->results_file.file_lock);
			fputs_unlocked(out, resolver_args->results_file.fd); //change fputs_unlocked -> fputs
			pthread_mutex_unlock(&resolver_args->results_file.file_lock);

			fprintf(stderr, "ERROR: Invalid hostname \"%s\".\n", hostname);

			free(out);
		}

		free(hostname);
	}

	return 0;
}

char** get_valid_input_files(char *input_files[], int potential_num, int *total_num_output) {

	int i;
	char **valid_files = malloc(sizeof(char *) * potential_num);
	int valid_files_index = 0;
	
	// check if it can open file. if yes add it to input_files array and increment valid_files_index. store max index to total_num
	for (i = 0; i < potential_num; i++) {
		if (access(input_files[i], F_OK | R_OK) == -1) {
			fprintf(stderr, "ERROR: Input file \"%s\" does not exist.\n", input_files[i]);
		}
		else {
			valid_files[valid_files_index++] = input_files[i];
		}
	}

	*total_num_output = valid_files_index;


	return valid_files;
}
