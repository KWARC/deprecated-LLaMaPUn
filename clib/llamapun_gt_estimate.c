#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <uthash.h>
//#include <json-c/json.h>
#include "jsoninclude.h"


//'Pure' Good-Turing smoothing doesn't work for the more frequent words
//so for now we'll use the following threshold.
//TODO: Apply a decent smoothing instead
#define THRESHOLD 5


struct freq_of_freq {
	long long key;
	long long count;
	UT_hash_handle hh;
};

json_object* get_gt_estimate(json_object *table) {
	/*
	  returns a new table with the corresponding Good-Turing estimates

	  assumes that table has integers as values, representing the
	  number of occurences of the corresponding key
	*/

	struct freq_of_freq * myhash = NULL;
	struct freq_of_freq * tmp;

	long long total_count = 0;
	long long int_val;

	//count frequencies
	{
		json_object_object_foreach(table, key, val) {
			int_val = json_object_get_int(val);
			total_count += int_val;
			HASH_FIND_INT(myhash, &int_val, tmp);
			if (tmp == NULL) {
				tmp = (struct freq_of_freq*) malloc(sizeof(struct freq_of_freq));
				tmp -> key = int_val;
				tmp -> count = 1;	//we've just found the first occurence
				HASH_ADD_INT(myhash, key, tmp);
			} else {
				tmp->count++;
			}
		}
	}

	json_object *response = json_object_new_object();

	double f_f, f_fp1;
	long long int_valp1;

	//create response
	{
		json_object_object_foreach(table, key, val) {
			int_val = json_object_get_int(val);
			if (int_val <= THRESHOLD) {
				HASH_FIND_INT(myhash, &int_val, tmp);
				f_f = tmp->count;

				int_valp1 = int_val+1;
				HASH_FIND_INT(myhash, &int_valp1, tmp);
				if (tmp == NULL) {
					f_fp1 = 0.0;
					fprintf(stderr, "Good-Turing estimate: Error: THRESHOLD too high - result won't make sense\n");
				} else {
					f_fp1 = tmp->count;
				}
				
				json_object_object_add(response, key, json_object_new_double((int_val+1)*(f_fp1/f_f)/total_count));
			} else {  //No decent smoothing implemented yet (it can be done better when seeing the actual input data)
				json_object_object_add(response, key, json_object_new_double(int_val/((double)total_count)));
			}
		}
	}


	//free memory
	struct freq_of_freq * tmp2;
	HASH_ITER(hh, myhash, tmp, tmp2) {
		HASH_DEL(myhash, tmp);
		free(tmp);
	}

	return response;
}