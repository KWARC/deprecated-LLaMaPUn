#!/usr/bin/bash

import os
import subprocess
import sys


#as beryl doesn't support python3:
if sys.version_info[0] < 3:
	input = raw_input



def getReadableFileName(msg):
	s = input(msg)
	if not os.path.isfile(s):
		print("Error: File doesn't exist")
		return getReadableFileName(msg)
	elif not os.access(s, os.R_OK):
		print("Error: File isn't readable")
		return getReadableFileName(msg)
	else:
		return s


print("Script for running SVM experiments")
print("\nNote: This is a simple tool, we assume the user knows what is going on,")
print("so we're not checking for everything that might go wrong")
print("(especially, older results can be overwritten)\n")

libsvmDir = input("LibSVM directory (e.g. \"/arXMLiv/experiments/libsvm-318\"):\n")
trainDataFileName = getReadableFileName("File name for training data: ")
testDataFileName = getReadableFileName("File name for test data: ")
subsetSize = int(input("Size of subset (e.g. 50000): "))
assert subsetSize > 0

def printAndCall(string):
	print(string)
	return subprocess.call(string, shell=True)

#STEP 1
print("Validating data")
if printAndCall("python " + os.path.join(libsvmDir, "tools/checkdata.py") + " " + trainDataFileName):
	print(trainDataFileName + " doesn't appear to be a valid input file.")
	sys.exit(1)
elif printAndCall("python " + os.path.join(libsvmDir, "tools/checkdata.py") + " " + testDataFileName):
	print(testDataFileName + " doesn't appear to be a valid input file.")
	sys.exit(1)


def commonPrefix(s1, s2):
	"""Returns the maximal common prefix of s1 and s2"""
	prefix = ""
	for i in range(len(s1)):
		if i >= len(s2) or s1[i] != s2[i]:
			break
		else:
			prefix += s1[i]
	return prefix

#STEP 2
print("Scaling the data (train set)")
#let's try to use meaningful names for files we created
scalingFileName = commonPrefix(trainDataFileName, testDataFileName) + "_scaling.txt"
trainScaledFileName = trainDataFileName + ".scaled"
if printAndCall(os.path.join(libsvmDir, "./svm-scale") + " -l 0 -s " + scalingFileName + " " + \
	               trainDataFileName + " > " + trainScaledFileName):
	print("An error occured while we were trying to scale the training set (fatal, exiting)")
	sys.exit(1)

print("Apply scaling to the test set")
testScaledFileName = testDataFileName + ".scaled"
if printAndCall(os.path.join(libsvmDir, "./svm-scale") + " -l 0 -r " + scalingFileName + " " + \
	               testDataFileName + " > " + testScaledFileName):
	print("An error occured while scaling the test set (fatal, exiting)")
	sys.exit(1)

#STEP 3
print("Generating a subset of the train data for parameter estimation")
subsetFileName = trainScaledFileName + ".subset"
if printAndCall("python " + os.path.join(libsvmDir, "tools/subset.py") + " -s 1 " + \
	               trainScaledFileName + " " + str(subsetSize) + " " + subsetFileName):
	print("An error occured while generating the subset (fatal, exiting)")
	sys.exit(1)


