#include <stdio.h>

#include "multi-lookup.h"
#include "util.h"

void checkMinArgs(int argc){
//Ensure that the minimum number of arguments were specified
	if (argc < 6) {
		fprintf(stderr, "ERROR: invalid arguments.\nUsage: ./multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]\n");
		exit(EXIT_FAILURE);
	}


}

void checkReceivedThreadNum(int req_threads, int res_threads){
	if (req_threads <= 0) {
		fprintf(stderr, "ERROR: There must be at least one requester thread.\n");
		exit(EXIT_FAILURE);
	}
	if (res_threads <= 0) {
		fprintf(stderr, "ERROR: There must be at least one resolver thread.\n");
		exit(EXIT_FAILURE);
	}

}

void checkReqResLogFilesExistance(char * req_log_file, char * res_log_file){
	if (access(req_log_file, F_OK | W_OK) == -1) {
		fprintf(stderr, "ERROR: The requester log file does not exist.\n");
		exit(EXIT_FAILURE);
	}
	if (access(res_log_file, F_OK | W_OK) == -1) {
		fprintf(stderr, "ERROR: The resolver log file does not exist.\n");
		exit(EXIT_FAILURE);
	}

}

ListOfInputFiles *initializeListOfInputFiles(int valid_file_tot){
	ListOfInputFiles * files = malloc(sizeof(*files) + sizeof(InputFile) * valid_file_tot);
	files->mutex_doc_list = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	files->total_files = valid_file_tot;
	files->current_file = 0;
	files->num_processed = 0;

	return files;
}

InputFile InitializeInputFile(char ** valid_file_names, int index){

		InputFile file;
		file.mutex_doc = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
		file.finished = 0;
		file.filename = valid_file_names[index];
		file.file_descriptor = fopen(file.filename, "r");

		return file;
}

SharedBuffer *createSharedBuffer(void){
	SharedBuffer *shared_buffer;
	shared_buffer = malloc(sizeof(*shared_buffer) + sizeof(char*) * SHARED_BUFFER_SIZE);

	shared_buffer->mutex_buffer = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	shared_buffer->buffer_full = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
	shared_buffer->buffer_empty = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
	shared_buffer->full = 0;
	shared_buffer->empty = 1;
	shared_buffer->requesters_done = 0;
	shared_buffer->total_size = SHARED_BUFFER_SIZE;
	shared_buffer->current_position = 0; //current_position in buffer

	return shared_buffer;
}

OutputFile createOutputFile(char * requester_log_file){
	OutputFile serviced_file;
	serviced_file.filename = requester_log_file;
	serviced_file.file_descriptor = fopen(requester_log_file, "w");
	serviced_file.mutex_doc = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	
	return serviced_file;
}

struct ReqThreadPool createReqThreadPool(ListOfInputFiles *files, SharedBuffer *shared_buffer, OutputFile serviced_file){
	struct ReqThreadPool requester_args;
	requester_args.files = files;
	requester_args.shared_buffer = shared_buffer;
	requester_args.serviced_file = serviced_file;

	return requester_args;
}

struct ResThreadPool createResThreadPool( SharedBuffer *shared_buffer, OutputFile results_file){
	struct ResThreadPool resolver_args;
	resolver_args.shared_buffer = shared_buffer;
	resolver_args.results_file = results_file;

	return resolver_args;
}


int main(int argc, char *argv[]) {
	//Intitalize time structs
	struct timeval start, end;
	//Get a start time
	gettimeofday(&start, NULL);

	//Ensure that the minimum number of arguments were specified
	checkMinArgs(argc);

	//Get the number of requesters and resolvers
	int num_requesters = atoi(argv[1]);
	int num_resolvers = atoi(argv[2]);

	//Check the received thread numbers
	checkReceivedThreadNum(num_requesters, num_resolvers);

	//Get the requester and resolver log files
	char *requester_log_file = argv[3];
	char *resolver_log_file = argv[4];

	//Check the existence of the requester and resolver log files
	checkReqResLogFilesExistance(requester_log_file, resolver_log_file);

	//Get the number of actually valid input files, and a list of those valid input paths
	int valid_file_tot;
	char **valid_file_names = file_validity(argv + 5, argc - 5, &valid_file_tot);

	//Initialize the ListOfInputFiles struct
	ListOfInputFiles *files= initializeListOfInputFiles(valid_file_tot);

	//Initialize an InputFile struct for each valid file and add it to the ListOfInputFiles struct list
	int i;
	for(i=0; i<valid_file_tot; i++){

		files->files[i] = InitializeInputFile(valid_file_names, i); // copy assignment from stack -> heap
	}

	////////////////////////////////////////////////////////// All struct of the Files have been created ///////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////// File Initialization Finished ////////////////////////////////////////////////////////////////

	//Create the shared buffer
	SharedBuffer *shared_buffer= createSharedBuffer();
	


	//////////////////////////////////////////////////////// Shared Buffer initialization Finished /////////////////////////////////////////////////////

	//Create serviced output file
	OutputFile serviced_file = createOutputFile(requester_log_file);

	// Create ReqThreadPool
	struct ReqThreadPool requester_args = createReqThreadPool(files,shared_buffer, serviced_file);
	//////////////////////////////////////////////////////// Serviced Output initialization Finished /////////////////////////////////////////////////////


	//Create an array to store the thread IDs of the requester threads
	pthread_t *tids_requester = malloc(sizeof(pthread_t) * num_requesters);

	//Start all of the requester threads
	for (i = 0; i < num_requesters; i++) {
		pthread_create(&tids_requester[i], NULL, requesters_thread, (void *)&requester_args);
	}

	//Create results output file
	OutputFile results_file= createOutputFile(resolver_log_file);
	
	//Create ResThreadPole struct
	struct ResThreadPool resolver_args =  createResThreadPool(shared_buffer,results_file);


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//Create an array to store the thread IDs of the resolver threads
	pthread_t *tids_resolver = malloc(sizeof(pthread_t) * num_resolvers);

	//Start all of the resolver threads
	for (i = 0; i < num_resolvers; i++) {

		pthread_create(&tids_resolver[i], NULL, resolvers_thread, (void *)&resolver_args);
	}

	//Join all of the requester threads
	for (i = 0; i < num_requesters; i++) {

		if(pthread_join(tids_requester[i], 0)){
			printf("ERROR: Could not join thread %ld\n",tids_requester[i]);
		}
	}



	pthread_mutex_lock(&shared_buffer->mutex_buffer);
	shared_buffer->requesters_done = 1;
	pthread_cond_broadcast(&shared_buffer->buffer_empty);
	pthread_mutex_unlock(&shared_buffer->mutex_buffer);

	//Join all of the resolver threads
	for (i = 0; i < num_resolvers; i++) {

		pthread_join(tids_resolver[i], 0);
	}

	//Close all of the open input files
	for(i = 0; i <  valid_file_tot; i++) {

		fclose(files->files[i].file_descriptor);
	}

	//Close serviced and results files
	fclose(serviced_file.file_descriptor);
	fclose(results_file.file_descriptor);

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



void *requesters_thread(void* files2input){

	struct ReqThreadPool *requester_args = (struct ReqThreadPool *) files2input;
	int counter = 0;

	// requester threads doing work and putting domain names in shared buffer
	requesters_thread_helperFunction(files2input, &counter);


	char * out = output_request2sharedBuffer(counter);

	pthread_mutex_lock(&requester_args->serviced_file.mutex_doc);
	// fputs_unlocked -> fputs
	fputs_unlocked(out, requester_args->serviced_file.file_descriptor);
	pthread_mutex_unlock(&requester_args->serviced_file.mutex_doc);
	printf("%s",out);

	free(out);

	return 0;
}

InputFile * find_unfinished_file(ListOfInputFiles * args){
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


void *requesters_thread_helperFunction(void *files2input, int *num_processed){

	struct ReqThreadPool *requester_args = (struct ReqThreadPool *) files2input;

	ListOfInputFiles *args = requester_args->files;
	SharedBuffer *shared_buffer = requester_args->shared_buffer;

	pthread_mutex_lock(&args->mutex_doc_list);
	//If all the files are processed then exit
	if (args->num_processed == args->total_files) {

		pthread_mutex_unlock(&args->mutex_doc_list);

		return 0;
	}
	// there must be an unfinished file because the mutex is locked
	else {

		// save pointer to file_to_process
		InputFile *file_to_process = find_unfinished_file(args);
		(*num_processed)++;

		// increment current_file by 1 like a circular buffer
		args->current_file = (args->current_file + 1) % args->total_files;
		
		pthread_mutex_unlock(&args->mutex_doc_list);

		while (1) {

			pthread_mutex_lock(&file_to_process->mutex_doc);

			
			char *hostname = (char *) malloc((MAX_NAME_LENGTH) * sizeof(char));
			//
			// fgets_unlocked ->`fgets` WHICH IS MT-SAFE !!! LET ME USE FGETS!!!
			// READ UNTIL EOF
			if (fgets_unlocked(hostname, MAX_NAME_LENGTH, file_to_process->file_descriptor)) {

				pthread_mutex_unlock(&file_to_process->mutex_doc);

				if (strlen(hostname) <= MAX_NAME_LENGTH - 1 && hostname[strlen(hostname) - 1] == '\n') {

					hostname[strlen(hostname) - 1] = '\0';
				}
				/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// Lock Shared memory buffer and do nessary operations
				pthread_mutex_lock(&shared_buffer->mutex_buffer);
			
				while (shared_buffer->full) {

					pthread_cond_wait(&shared_buffer->buffer_full, &shared_buffer->mutex_buffer);
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

				pthread_mutex_unlock(&shared_buffer->mutex_buffer);
				////////////////////////////////////////////////////////////////////////////////////////////////////
			}
			else {
				// free memory space
				free(hostname);
				
				// if file_to_process is not finished
				if (!file_to_process->finished) {
					// Set to finished
					file_to_process->finished = 1;

					pthread_mutex_unlock(&file_to_process->mutex_doc);

					pthread_mutex_lock(&args->mutex_doc_list);

					args->num_processed++;

					pthread_mutex_unlock(&args->mutex_doc_list);
					
					// restart the process and increment num_process by 1
					requesters_thread_helperFunction(files2input, num_processed++);
				}
				else {

					pthread_mutex_unlock(&file_to_process->mutex_doc);
				}

				break;
			}
		}
	}

	return 0;
}

char * output_sharedBuffer2output(char * hostname, char * ip){

	int length_str = snprintf(NULL, 0, "%s, %s\n", hostname, ip );
	
	char * out = malloc(length_str + 1);

	snprintf(out,length_str+1,"%s, %s\n", hostname, ip);
	
	return out;
}
char * output_sharedBuffer2output2(char * hostname){

	int length_str = snprintf(NULL, 0, "%s,\n", hostname);
	
	char * out = malloc(length_str + 1);

	snprintf(out,length_str+1,"%s,\n", hostname);
	
	return out;
}


void *resolvers_thread(void *arguments){

	struct ResThreadPool *resolver_args = (struct ResThreadPool *) arguments;

	SharedBuffer *shared_buffer = resolver_args->shared_buffer;

	while (1) {
		//Acquire buffer lock
		pthread_mutex_lock(&shared_buffer->mutex_buffer);
		//Wait while the shared buffer is empty and the requesters are not done
		while (shared_buffer->empty && !shared_buffer->requesters_done) {
			pthread_cond_wait(&shared_buffer->buffer_empty, &shared_buffer->mutex_buffer);
		}
		//Release the buffer lock if the buffer is empty and requesters are done
		if (shared_buffer->empty && shared_buffer->requesters_done){
			pthread_mutex_unlock(&shared_buffer->mutex_buffer);
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

		pthread_mutex_unlock(&shared_buffer->mutex_buffer);

		char ip[MAX_IP_ADDRESS]; // add max ip length

		if (dnslookup(hostname, ip, MAX_IP_ADDRESS ) == UTIL_SUCCESS) {

			char* out = output_sharedBuffer2output(hostname, ip);

			pthread_mutex_lock(&resolver_args->results_file.mutex_doc);
			fputs_unlocked(out, resolver_args->results_file.file_descriptor); //change fputs_unlocked -> fputs
			pthread_mutex_unlock(&resolver_args->results_file.mutex_doc);

			free(out);

		}
		else {
			char* out = output_sharedBuffer2output2(hostname);

			pthread_mutex_lock(&resolver_args->results_file.mutex_doc);
			fputs_unlocked(out, resolver_args->results_file.file_descriptor); //change fputs_unlocked -> fputs
			pthread_mutex_unlock(&resolver_args->results_file.mutex_doc);

			fprintf(stderr, "ERROR: Invalid hostname \"%s\".\n", hostname);

			free(out);
		}

		free(hostname);
	}

	return 0;
}


	char** file_validity(char ** files2input, int numberOfPossibleFiles, int *total_files){

	int iterator;
	char **files_valid = malloc(sizeof(char *) * numberOfPossibleFiles);
	int index = 0;
	
	// check if it can open file. if yes add it to input_files array and increment valid_files_index. store max index to total_num
	for (iterator = 0; iterator < numberOfPossibleFiles; iterator++) {
		if (access(files2input[iterator], F_OK | R_OK) == -1) {
			fprintf(stderr, "ERROR: Input file \"%s\" does not exist.\n", files2input[iterator]);
		}
		else {
			files_valid[index++] = files2input[iterator];
		}
	}

	*total_files = index;


	return files_valid;
}
