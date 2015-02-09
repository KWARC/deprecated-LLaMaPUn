#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>

#include <textcat.h>

#include "mpi.h"
//#include "mpc.h"

#include "llamapun/language_detection.h"
#include "llamapun/utils.h"
#include "llamapun/dnmlib.h"

int ITEMS_TO_BE_SEND = 0;
#define FILE_NAME_SIZE 2048
#define RETURN_MESSAGE_SIZE 4096
FILE *logfile;


int parse(const char *filename, const struct stat *s, int type) {
	if (type != FTW_F) return 0; //Not a file
	UNUSED(s);
	char filename_maxlength[FILE_NAME_SIZE];
	if (FILE_NAME_SIZE <= strlen(filename)) {   		//if filename is too long for chunk size
		fprintf(logfile, "Error: file name too long (%s)\nskipping file\n", filename);
		fprintf(stderr, "%2d - Error: file name too long (%s)\nskipping file\n", 0, filename);
		return 0;
	}

	snprintf(filename_maxlength, FILE_NAME_SIZE, "%s", filename);   //it's easier to send chunks of fixed size

	if (ITEMS_TO_BE_SEND) {
		MPI_Send(filename_maxlength, FILE_NAME_SIZE, MPI_CHAR, /*receiver = */ ITEMS_TO_BE_SEND--, /*worktag = */ 1, MPI_COMM_WORLD);
	} else {   //if everyone has a job, wait, until first one is done
		MPI_Status status;
		char message[RETURN_MESSAGE_SIZE];
		MPI_Recv(message, RETURN_MESSAGE_SIZE, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG == 0) {
			//nothing special
		} else if (status.MPI_TAG == 1) {
			//print message to logfile
			fprintf(logfile, "%s", message);
		} else {
			fprintf(stderr, "ERROR: UNKOWN TAG %d SENT TO MASTER - exiting\n", status.MPI_TAG);
			exit(1);
		}
		//send new job
		MPI_Send(filename_maxlength, FILE_NAME_SIZE, MPI_CHAR, /*receiver = */ status.MPI_SOURCE, /*worktag = */ 1, MPI_COMM_WORLD);
	}

	return 0;
}

void run_slave(int myrank) {
	void *my_tc = llamapun_textcat_Init();
	char filename[FILE_NAME_SIZE];
	char message[RETURN_MESSAGE_SIZE];
	MPI_Status status;
	while (1) {
		MPI_Recv(&filename, FILE_NAME_SIZE, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG == 0) {
			printf("%2d - exiting\n", myrank);
			break;
		} else if (status.MPI_TAG == 1) {
			//do the actual job

			//printf("%2d - %s\n", myrank, filename);

			xmlDoc *doc = read_document(filename);
			if (doc == NULL) {
				snprintf(message, RETURN_MESSAGE_SIZE, "Couldn't load document %s\n", filename);
				MPI_Send(message, RETURN_MESSAGE_SIZE, MPI_CHAR, /*dest = */ 0, /*tag = message */ 1, MPI_COMM_WORLD);
			}
			
			dnmPtr dnm = create_DNM(xmlDocGetRootElement(doc), DNM_SKIP_TAGS);

			if (dnm == NULL) {
				fprintf(stderr, "%2d - Couldn't create DNM - exiting\n", myrank);
				exit(1);
			}
			char *result = textcat_Classify(my_tc, dnm->plaintext, dnm->size_plaintext);
			if (strncmp(result, "[english]", strlen("[english]"))) {  //isn't primarily english
				snprintf(message, RETURN_MESSAGE_SIZE, "%s\t%s\n", filename, result);
				MPI_Send(message, RETURN_MESSAGE_SIZE, MPI_CHAR, /*dest = */ 0, /*tag = message */ 1, MPI_COMM_WORLD);
			}
			else {
				snprintf(message, RETURN_MESSAGE_SIZE, "%s\tenglish\n", filename);
				printf("%2d - %s", myrank, message);
				MPI_Send(message, RETURN_MESSAGE_SIZE, MPI_CHAR, /*dest = */ 0, /*tag = nothing special */ 0, MPI_COMM_WORLD);
			}

			//clean up
			free_DNM(dnm);
			xmlFreeDoc(doc);
		} else {
			fprintf(stderr, "%2d - Error: Unkown tag: %d - exiting\n", myrank, status.MPI_TAG);
			break;
		}
	}
	//clean up
	textcat_Done(my_tc);
	xmlCleanupParser();
}


void kill_slaves() {
	char random_message[FILE_NAME_SIZE];
	MPI_Comm_size(MPI_COMM_WORLD, &ITEMS_TO_BE_SEND);
	while (--ITEMS_TO_BE_SEND) {
		MPI_Status status;
		char message[RETURN_MESSAGE_SIZE];
		MPI_Recv(message, RETURN_MESSAGE_SIZE, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG == 0) {
			//nothing special
		} else if (status.MPI_TAG == 1) {
			//print message to logfile
			fprintf(logfile, "%s", message);
		} else {
			fprintf(stderr, "ERROR: UNKOWN TAG %d SENT TO MASTER - exiting\n", status.MPI_TAG);
			exit(1);
		}
		printf("%2d - killing %2d\n", 0, status.MPI_SOURCE);
		MPI_Send(random_message, FILE_NAME_SIZE, MPI_CHAR, /*receiver = */ status.MPI_SOURCE, /*worktag = die */ 0, MPI_COMM_WORLD);
	}
}

int main(int argc, char *argv[]) {
	int myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	printf("%2d - started\n", myrank);

	if (myrank == 0) {   //Master, i.e. distributor of tasks
		logfile = fopen("logfile.txt", "w");
		if (!logfile) {fprintf(stderr, "Can't open logfile, exiting\n"); exit(1);}
		MPI_Comm_size(MPI_COMM_WORLD, &ITEMS_TO_BE_SEND);
		ITEMS_TO_BE_SEND--;    //number of processes that are supposed to receive a task
		if(argc == 1)
			ftw(".", parse, 1);
		else
			ftw(argv[1], parse, 1);
		kill_slaves();
		fclose(logfile);
	} else {
		run_slave(myrank);
	}
	return 0;
}