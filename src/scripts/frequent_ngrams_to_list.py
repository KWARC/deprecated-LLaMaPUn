#!/usr/bin/python

"""
This script is supposed to be used to reduce the large json files with ngram frequencies to a list of the most frequent ngrams.
"""

import json

inFileName = input("Filename: ")
threshold = int(input("Minimal frequency: "))


with open(inFileName, "r") as fp:
	dictionary = json.loads(fp.read())

outFileName = input("Output file: ")

fp = open(outFileName, "w")

for (ngram, freq) in dictionary.items():
	if freq > threshold:
		fp.write(ngram+"\n")

fp.close()

