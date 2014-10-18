#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>

#include "llamapun/document_loader.h"
#include "llamapun/utils.h"

#include <libxml/parser.h>
#include "mpi.h"


int ITEMS_TO_BE_SENT = 0;
#define FILE_NAME_SIZE 2048

const char const *paragraph_xpath = "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
const char const *relaxed_paragraph_xpath = "//*[local-name()='div' and @class='ltx_para']";

FILE *result_file;


void process_paragraph(char *words[], size_t number, xmlNodePtr node) {
	
}



//SLAVE process
void run_slave(int myrank) {
	char filename[FILE_NAME_SIZE];
	snprintf(filename, sizeof(filename), "/tmp/llamapun_inp_%d.txt", myrank);
	result_file = fopen(filename, "w");
	if (result_file == NULL) {
		fprintf(stderr, "%2d - Couldn't create %s (fatal)\n", myrank, filename);
		exit(1);
	}
	MPI_Status status;
	init_document_loader();

	while (1) {
		MPI_Recv(&filename, FILE_NAME_SIZE, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG == 0) {
			printf("%2d - exiting\n", myrank);
			break;
		} else if (status.MPI_TAG == 1) {
			//do the actual job
			xmlDoc *document = read_document(filename);
			if (document == NULL) {
				fprintf(stderr, "%2d - Couldn't load document %s\n", myrank, filename);
			}
			
			int b = with_words_at_xpath(process_paragraph, document, paragraph_xpath, /* logfile = */ stderr,
				WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS, /* | WORDS_MARK_END_OF_SENTENCE, */
				DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
			if (!b) {
				b = with_words_at_xpath(process_paragraph, document, relaxed_paragraph_xpath, /* logfile = */ stderr,
					WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS,
					DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
			}

			xmlFreeDoc(document);

			//request new document
			MPI_Send(0, 1, MPI_INT, /*dest = */ 0, /*tag = nothing special */ 0, MPI_COMM_WORLD);

		} else {
			fprintf(stderr, "%2d - Error: Unkown tag: %d - exiting\n", myrank, status.MPI_TAG);
			break;
		}
	}
	//clean up
	close_document_loader();
	fclose(result_file);
}




void kill_slaves() {
	char random_message[FILE_NAME_SIZE];
	MPI_Comm_size(MPI_COMM_WORLD, &ITEMS_TO_BE_SENT);
	while (--ITEMS_TO_BE_SENT) {
		MPI_Status status;
		int value;
		MPI_Recv(&value, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG == 0) {
			//nothing special
		} else {
			fprintf(stderr, "ERROR: UNKOWN TAG %d SENT TO MASTER - exiting\n", status.MPI_TAG);
			exit(1);
		}
		printf("%2d - killing %2d\n", 0, status.MPI_SOURCE);
		MPI_Send(random_message, FILE_NAME_SIZE, MPI_CHAR, /*receiver = */ status.MPI_SOURCE, /*worktag = die */ 0, MPI_COMM_WORLD);
	}
}




//MASTER process, distributing the documents:
int parse(const char *filename, const struct stat *s, int type) {
	if (type != FTW_F) return 0; //Not a file
	UNUSED(s);
	char filename_maxlength[FILE_NAME_SIZE];
	if (FILE_NAME_SIZE <= strlen(filename)) {   		//if filename is too long for chunk size
		fprintf(stderr, "%2d - Error: file name too long (%s)\nskipping file\n", 0, filename);
		return 0;
	}

	snprintf(filename_maxlength, FILE_NAME_SIZE, "%s", filename);   //it's easier to send chunks of fixed size

	if (ITEMS_TO_BE_SENT) {
		MPI_Send(filename_maxlength, FILE_NAME_SIZE, MPI_CHAR, /*receiver = */ ITEMS_TO_BE_SENT--, /*worktag = */ 1, MPI_COMM_WORLD);
	} else {   //if everyone has a job, wait, until first one is done
		MPI_Status status;
		int value;
		MPI_Recv(&value, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG == 0) {
			//nothing special
		} else {
			fprintf(stderr, "ERROR: UNKOWN TAG %d SENT TO MASTER - exiting\n", status.MPI_TAG);
			exit(1);
		}
		if (value != 0) {
			fprintf(stderr, "THERE MIGHT BE SOME PROBLEMS...");
		}
		//send new job
		MPI_Send(filename_maxlength, FILE_NAME_SIZE, MPI_CHAR, /*receiver = */ status.MPI_SOURCE, /*worktag = */ 1, MPI_COMM_WORLD);
	}

	return 0;
}

void merge_files(int n) {
	printf("MERGING RESULTS\n");
	FILE *outfile = fopen("libsvm_input.txt", "w");
	if (outfile == NULL) {
		fprintf(stderr, "Fatal: Can't create libsvm_input.txt\n");
		exit(1);
	}
	char filename[FILE_NAME_SIZE];
	char buffer[65536];
	int i;
	for (i = 1; i <= n; i++) {
		snprintf(filename, sizeof(filename), "/tmp/llamapun_inp_%d.txt", i);
		printf("Merging %s\n", filename);
		FILE *infile = fopen(filename, "r");
		if (infile == NULL) {
			fprintf(stderr, "Error: Cannot open %s (skipping)\n", filename);
			continue;
		}
		//append file content
		while (fgets(buffer, sizeof(buffer), infile)) {
			fprintf(outfile, "%s", buffer);
		}
		fclose(infile);
	}
	fclose(outfile);
}


int main(int argc, char *argv[]) {
	int myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	printf("%2d - started\n", myrank);

	if (myrank == 0) {   //Master, i.e. distributor of tasks
		MPI_Comm_size(MPI_COMM_WORLD, &ITEMS_TO_BE_SENT);
		int number_of_processes = ITEMS_TO_BE_SENT;
		ITEMS_TO_BE_SENT--;    //number of processes that are supposed to receive a task
		if(argc == 1)
			ftw(".", parse, 1);
		else
			ftw(argv[1], parse, 1);
		kill_slaves();
		merge_files(number_of_processes);
	} else {
		run_slave(myrank);
	}
	return 0;
}


