#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <unistd.h>
#include <math.h>

#include "llamapun/document_loader.h"
#include "llamapun/utils.h"
#include "llamapun/unicode_normalizer.h"

#include <uthash.h>
#include <libxml/parser.h>
#include "mpi.h"


struct document_frequencies_hash* frequencies_hash_to_bins(struct score_hash *scores);

int ITEMS_TO_BE_SENT = 0;
#define FILE_NAME_SIZE 2048

int FILE_COUNTER = 0;

const char const *paragraph_xpath = "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
const char const *relaxed_paragraph_xpath = "//*[local-name()='div' and @class='ltx_para']";

FILE *result_file;

//global data
struct score_hash *idf;
struct document_frequencies_hash* word_to_bin = NULL;

//document-wise data
struct document_frequencies_hash *DF = NULL;
struct document_frequencies_hash *DF_prime = NULL;    //count while parsing to figure out whether terms occur 1st/2nd/3rd time
int doc_max_count;
double tfc_doc_normalization_divisor;


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

}

void pre_process_paragraph(char *words[], size_t number, xmlNodePtr node) {
	UNUSED(node);
	size_t i = 0;
	while (i < number)
		record_word(&DF,words[i++]);
}

void in_between_processing() {
	//determine doc_max_count
	struct document_frequencies_hash *word_entry;
	struct score_hash *idf_entry;
	doc_max_count = 0;
	tfc_doc_normalization_divisor = 0.0;
	for(word_entry=DF; word_entry != NULL; word_entry = word_entry->hh.next) {
		if (doc_max_count < word_entry->count) {
			doc_max_count = word_entry->count;
		}
		HASH_FIND_STR(idf, word_entry->word, idf_entry);
		if (idf_entry) {
			tfc_doc_normalization_divisor += (word_entry->count * idf_entry->score) * (word_entry->count * idf_entry->score);
		} else {
			//set idf to 13
			tfc_doc_normalization_divisor += (word_entry->count * 13) * (word_entry->count * 13);
		}
	}
	tfc_doc_normalization_divisor = sqrt(tfc_doc_normalization_divisor);
}

void process_paragraph(char *words[], size_t number, xmlNodePtr node) {
	double bins[20000] = { 0 };
	size_t i=0;
	int recorded_bin = 0;
	for (i=0; i<20000; i++) bins[i]=0;
	struct document_frequencies_hash *word_entry;
	unsigned int paragraph_word_count = 0;


	int counters_threshold_a[3];
	int counters_threshold_b[3];
	int counters_threshold_c[3];
	int counters_threshold_d[3];
	for (i = 0; i < 3; i++) {
		counters_threshold_a[i] = 0;
		counters_threshold_b[i] = 0;
		counters_threshold_c[i] = 0;
		counters_threshold_d[i] = 0;
	}

	for (i=0; i<number; i++) {
		paragraph_word_count++;
		struct document_frequencies_hash* bin_entry;
		record_word(&DF_prime, words[i]);

		HASH_FIND_STR(DF, words[i], word_entry);
		//double word_tf = 0.5 + (0.5 * word_entry->count)/doc_max_count;
		double word_tf = word_entry->count;   //applying both normalizations doesn't work
		// Lookup the IDF:
		struct score_hash* idf_entry;
		HASH_FIND_STR(idf, words[i], idf_entry);
		double word_idf = 13;
		if(idf_entry!=NULL) {
			word_idf = idf_entry->score;
		}
		else {
			// Hardcoding anything missing as a term (corpus-wise)
			// that's what log2(8800 / 1) computes to anyway:
			word_idf = 13;
		}

		HASH_FIND_STR(word_to_bin, words[i], bin_entry);
		if (bin_entry != NULL) {
			int bin = bin_entry->count;
			// We're using bins, numbered by the word stem and valued with the TFxIDF scores
			// Compute the TF:
			recorded_bin = 1;
			bins[bin] += (word_tf * word_idf) / tfc_doc_normalization_divisor;

			HASH_FIND_STR(DF_prime, words[i], word_entry);
			if (word_entry->count < 4) {   //word is recorded already
				if (bins[bin] > 0.4) counters_threshold_d[word_entry->count - 1]++;
				if (bins[bin] > 0.12) counters_threshold_c[word_entry->count - 1]++;
				if (word_idf > 4.0) counters_threshold_b[word_entry->count - 1]++;
				if (word_idf > 7.5) counters_threshold_a[word_entry->count - 1]++;
			}
		}

	}

	if (recorded_bin) {// Some paras have no content words, skip.

		// We have the paragraph vector, now map it into a TF/IDF vector and write down a labeled training instance:
		int label = is_definition(node->parent) ? 1 : -1;
		fprintf(result_file, "%d",label);

		for (i=0; i<3; i++) {
			fprintf(result_file, " %ld:%d", 4*i, counters_threshold_a[i]);
			fprintf(result_file, " %ld:%d", 4*i+1, counters_threshold_b[i]);
			fprintf(result_file, " %ld:%d", 4*i+2, counters_threshold_c[i]);
			fprintf(result_file, " %ld:%d", 4*i+3, counters_threshold_d[i]);
		}

		fprintf(result_file, " %d:%d", 13, paragraph_word_count);

		for (i=0; i<20000; i++) {
			if(bins[i] > 0.0005) {
				//if (i%5 == 0) printf("%3.4f\n", 100*bins[i]);
				// We also need to normalize on the basis of paragraph length
				fprintf(result_file, " %ld:%f", 13+i,bins[i]);
			}
		}
		fprintf(result_file,"\n");
	}
}

void clean_after_processing() {
	free_document_frequencies_hash(DF);
	DF = NULL;
	free_document_frequencies_hash(DF_prime);
	DF_prime = NULL;
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

	/* Load pre-computed corpus IDF scores */
	json_object* idf_json = read_json("idf.json");
	idf = json_to_score_hash(idf_json);
	json_object_put(idf_json);
	/* Map common words to bin positions */
	word_to_bin = frequencies_hash_to_bins(idf);

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
			int b = with_words_at_xpath(pre_process_paragraph, document, paragraph_xpath, /* logfile = */ stderr,
				WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS, /* | WORDS_MARK_END_OF_SENTENCE, */
				DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
			if (!b) {
				with_words_at_xpath(pre_process_paragraph, document, relaxed_paragraph_xpath, /* logfile = */ stderr,
					WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS,
					DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
				in_between_processing();
				with_words_at_xpath(process_paragraph, document, relaxed_paragraph_xpath, /* logfile = */ stderr,
					WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS,
					DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
			} else {
				in_between_processing();
				with_words_at_xpath(process_paragraph, document, paragraph_xpath, /* logfile = */ stderr,
					WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS, /* | WORDS_MARK_END_OF_SENTENCE, */
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
	free_score_hash(idf);
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



struct document_frequencies_hash* frequencies_hash_to_bins(struct score_hash *scores) {
	struct score_hash *s;
	struct document_frequencies_hash *tmp, *bins_hash = NULL;
	int bin_index = 0;
	HASH_SORT(scores, ascending_score_sort);
	for(s=scores; s != NULL; s = s->hh.next) {
		if (s->score >= 10.5) { break; } // Filter >4 frequent words
		tmp = (struct document_frequencies_hash*)malloc(sizeof(struct document_frequencies_hash));
		tmp->word = strdup(s->word);
		tmp->count = ++bin_index;
		HASH_ADD_KEYPTR( hh, bins_hash, tmp->word, strlen(tmp->word), tmp );
	}
	fprintf(stderr, "Total bins: %d\n",bin_index);
	return bins_hash;
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


