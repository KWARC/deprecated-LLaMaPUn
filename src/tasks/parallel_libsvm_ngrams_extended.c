#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ftw.h>
#include <unistd.h>
#include <math.h>

#include "llamapun/document_loader.h"
#include "llamapun/utils.h"
#include "llamapun/unicode_normalizer.h"

#include <uthash.h>
#include <libxml/parser.h>
#include "mpi.h"



int ITEMS_TO_BE_SENT = 0;
#define FILE_NAME_SIZE 2048

int FILE_COUNTER = 0;

const char const *paragraph_xpath = "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
const char const *relaxed_paragraph_xpath = "//*[local-name()='div' and @class='ltx_para']";

FILE *result_file;

//global data
struct document_frequencies_hash* UNIGRAM_TO_BIN = NULL;
struct document_frequencies_hash* BIGRAM_TO_BIN = NULL;
struct document_frequencies_hash* TRIGRAM_TO_BIN = NULL;
int NUMBER_OF_UNIGRAMS;
int NUMBER_OF_BIGRAMS;
int NUMBER_OF_TRIGRAMS;
size_t NUMBER_OF_FEATURES;

int number_of_paragraphs;
int paragraph_number;

char *BINS;

int is_definition(xmlNode * n) {
	if (!xmlStrEqual(n->name, BAD_CAST "div")) return 0;
	xmlAttr *attr;
	char *class_value;
	int retval = 0;
	for (attr = n->properties; attr != NULL; attr = attr->next) {
		if (xmlStrEqual(attr->name, BAD_CAST "class")) {
			class_value = (char*) xmlNodeGetContent(attr->children);
			if(strstr(class_value, "ltx_theorem_def") != NULL) 
				retval = 1;
			xmlFree(class_value);
			return retval;
		}
	}
	return 0;   // has no class
}

void prepare_preprocessing() {
	number_of_paragraphs = 0;
	paragraph_number = 0;
}

void pre_process_paragraph(char *words[], size_t number_of_words, xmlNodePtr node) {  //extremely inefficient, but we'll need this part later anyway
	number_of_paragraphs++;
}


void process_paragraph(char *words[], size_t number_of_words, xmlNodePtr node) {
	size_t i;
	struct document_frequencies_hash* bin_entry;
	int recorded_something = 0;
	int max_ngram_length = 0;
	char string[500];
	for (i=0; i < number_of_words; i++) {
		max_ngram_length++;
		if (!strcmp(words[i], "endofsentence") || (!isalnum(*words[i]) && *words[i] != '\'')) {
			max_ngram_length = 0;
			continue;
		}
		//store unigram
		HASH_FIND_STR(UNIGRAM_TO_BIN, words[i], bin_entry);
		if (bin_entry != NULL) {
			recorded_something = 1;
			BINS[bin_entry->count] = 1;
		} else {      //Idea: For any ngram we consider, we also consider the (n-1)grams it contains
			max_ngram_length = 0;
			continue;
		}
		//store bigram
		if (max_ngram_length <= 1) continue;
		snprintf(string, sizeof(string), "%s %s", words[i-1], words[i]);
		HASH_FIND_STR(BIGRAM_TO_BIN, string, bin_entry);
		if (bin_entry != NULL) {
			BINS[NUMBER_OF_UNIGRAMS + bin_entry->count] = 1;
		} else {
			max_ngram_length = 1;   //bigram not contained => trigram containing it won't be either
			continue;
		}
		//store trigram
		if (max_ngram_length <=2) continue;
		snprintf(string, sizeof(string), "%s %s %s", words[i-2], words[i-1], words[i]);
		HASH_FIND_STR(TRIGRAM_TO_BIN, string, bin_entry);
		if (bin_entry != NULL) {
			BINS[NUMBER_OF_UNIGRAMS + NUMBER_OF_BIGRAMS + bin_entry->count] = 1;
		}
	}

	if (!recorded_something) return;

	//write things to file
	int label = is_definition(node->parent) ? 1 : -1;
	fprintf(result_file, "%d",label);
	fprintf(result_file, " 0:%ld", number_of_words);
	fprintf(result_file, " 1:%d", number_of_paragraphs);
	fprintf(result_file, " 2:%d", ++paragraph_number);
	for (i=0; i<NUMBER_OF_FEATURES; i++) {
		if (BINS[i]) {
			BINS[i] = 0;   //We'll use the array again
			fprintf(result_file, " %ld:1", i+3);
		}
	}
	fprintf(result_file, "\n");
}

void clean_after_processing() {

}

struct document_frequencies_hash* load_ngrams(char *filename, int *counter) {
	struct document_frequencies_hash *tmp, *bins_hash = NULL;
	*counter = 0;
	FILE *fp = fopen(filename, "r");
	char buffer[500]; //no ngram will be that long
	while  (fgets(buffer, sizeof(buffer), fp)) {
		if (strlen(buffer) > 1) {
			buffer[strlen(buffer)-1] = '\0';  //remove trailing linebreak
			tmp = (struct document_frequencies_hash*)malloc(sizeof(struct document_frequencies_hash));
			tmp->word = strdup(buffer);
			tmp->count = (*counter)++;
			HASH_ADD_KEYPTR( hh, bins_hash, tmp->word, strlen(tmp->word), tmp );
		}
	}
	return bins_hash;
}


//SLAVE process
void run_slave(int myrank) {
	char filename[FILE_NAME_SIZE];
	snprintf(filename, FILE_NAME_SIZE, "/tmp/llamapun_inp_%d.txt", myrank);
	result_file = fopen(filename, "w");
	if (result_file == NULL) {
		fprintf(stderr, "%2d - Couldn't create %s (fatal)\n", myrank, filename);
		exit(1);
	}

	UNIGRAM_TO_BIN = load_ngrams("unigrams.list", &NUMBER_OF_UNIGRAMS);
	BIGRAM_TO_BIN = load_ngrams("bigrams.list", &NUMBER_OF_BIGRAMS);
	TRIGRAM_TO_BIN = load_ngrams("trigrams.list", &NUMBER_OF_TRIGRAMS);

	NUMBER_OF_FEATURES = NUMBER_OF_UNIGRAMS + NUMBER_OF_BIGRAMS + NUMBER_OF_TRIGRAMS;

	BINS = (char *) malloc(NUMBER_OF_FEATURES * sizeof(char));
	size_t i = 0;
	for (;i<NUMBER_OF_FEATURES; i++) BINS[i] = 0;

	printf("Number of features: %ld\n", NUMBER_OF_FEATURES);

	MPI_Status status;
	init_document_loader();

	while (1) {
		MPI_Recv(&filename, FILE_NAME_SIZE, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG == 0) {
			printf("%2d - exiting\n", myrank);
			break;
		} else if (status.MPI_TAG == 1) {
			printf("%2d - %s\n", myrank, filename);
			//do the actual job
			xmlDoc *document = read_document(filename);
			if (document == NULL) {
				fprintf(stderr, "%2d - Couldn't load document %s\n", myrank, filename);
			}
			unicode_normalize_dom(document);
			prepare_preprocessing();
			int b = with_words_at_xpath(pre_process_paragraph, document, paragraph_xpath, stderr, 0, DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
			if (b) {
				with_words_at_xpath(process_paragraph, document, paragraph_xpath, /* logfile = */ stderr,
					WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS | WORDS_MARK_END_OF_SENTENCE,
					DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
			} else {
				with_words_at_xpath(pre_process_paragraph, document, relaxed_paragraph_xpath, stderr, 0, DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
				with_words_at_xpath(process_paragraph, document, relaxed_paragraph_xpath, /* logfile = */ stderr,
					WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS | WORDS_MARK_END_OF_SENTENCE,
					DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
			}

			clean_after_processing();

			xmlFreeDoc(document);

			//request new document
			int i = 0;
			MPI_Send(&i, 1, MPI_INT, /*dest = */ 0, /*tag = nothing special */ 0, MPI_COMM_WORLD);
		} else {
			fprintf(stderr, "%2d - Error: Unkown tag: %d - exiting\n", myrank, status.MPI_TAG);
			break;
		}
	}
	//clean up
	close_document_loader();
	fclose(result_file);
	free_document_frequencies_hash(UNIGRAM_TO_BIN);
	free_document_frequencies_hash(BIGRAM_TO_BIN);
	free_document_frequencies_hash(TRIGRAM_TO_BIN);
	free(BINS);
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
		printf("%2d - file count: %d\n", 0, FILE_COUNTER++);
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
		printf("%2d - file count: %d\n", 0, FILE_COUNTER++);
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
	for (i = 1; i < n; i++) {
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
		sleep(1);  //just to make sure that files are closed properly...
		merge_files(number_of_processes);
		fprintf(stderr, "Finished merging\n");
	} else {
		run_slave(myrank);
	}
	MPI_Finalize();
	return 0;
}


