import os

print("Written for Python 3, requires simplifed HTML documents (../tasks/simplify_documents)")

indexfile = open(input("Index file: "), "r")

WORDS = []

for line in indexfile:
	if line:
		WORDS += [[int(line.split(" ")[0]), 0, 0, line.split(" ")[1][:-1]]]     #id, occurences in definitions, occurences outside, string representation

indexfile.close()

print("%d words loaded" % len(WORDS))

COUNTER = 0
for root, dirs, names in os.walk(input("Directory: ")):
	for f in names:
		print("%d - %s" % (COUNTER, f))
		COUNTER += 1
		with open(os.path.join(root, f), "r") as fp:
			for line in fp:
				if line:
					l = line.split(" ")
					if l[0] == "-1":     #it's a definition
						for e in l[1:]:
							if e:   #old simplify_documents inserted one double space per line
								WORDS[int(e)][2] += 1
					else:
						for e in l[1:]:
							if e:
								WORDS[int(e)][1] += 1

with open(input("Output: "), "w") as fp:
	for word in WORDS:
		fp.write("%d %d %d %s\n" % tuple(word))

print("DONE :-)")