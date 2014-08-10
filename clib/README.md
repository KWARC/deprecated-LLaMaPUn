The LLaMaPUn C library
======================

Installation
------------

You can clone the source from github:
```bash
git clone https://github.com/KWARC/LLaMaPUn.git
```

The following libraries need to be installed:
* libxml2
* libJSON
* uthash
* libiconv

The [`.travis.yml` file](https://github.com/KWARC/LLaMaPUn/blob/master/.travis.yml) in the repository shows an example installation of the
LLaMaPUn C library, including the installation of the above mentioned libraries using `apt-get`.

Since there are some issues issues with older versions of the JSON library,
you might want to run a small script to fix the includes, if necessary:
```bash
sh jsonincludecheck.sh > /dev/null
```

Now we can compile, test, and install the library, using `cmake`:
```bash
mkdir -p build
cd build
cmake ..
make
make test
sudo make install
sudo ldconfig
```


Including and Linking
---------------------

After the installation, you can include e.g. the DNM library writing
```C
#include <llamapun/dnmlib.h>
```
If you use `gcc` or `clang` you can link the LLaMaPUn library by setting the `-lllamapun` flag.


The DNM Library
---------------

When we are dealing with the LaTeXML generated XHTML documents,
we often switch between the DOM view (for structural information etc.)
and the plain text view (for most NLP tools).
The DNM library simplifies this switching by creating a DNM
(Document Narrative Model).

```C
/* Example usage of the DNM library
   Warning: In real applications error checking should be done
            e.g. whether document was loaded, whether iterator actually exists, ...
*/

#include <stdio.h>
#include <stdlib.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <llamapun/dnmlib.h>

int main(void) {
	//load example document
	xmlDocPtr mydoc = xmlReadFile("document.xhtml", NULL, 0);
	//Create DNM, with normalized math tags, and ignoring cite tags
	dnmPtr mydnm = createDNM(mydoc, DNM_NORMALIZE_MATH | DNM_SKIP_CITE);

	//create an iterator over the paragraphs of the document
	dnmIteratorPtr myIterator = getDnmIterator(mydnm, DNM_LEVEL_PARA);

	do {
		//check if paragraph is in a tag marked up as a theorem
		if (dnmIteratorHasAnnotationInherited(myIterator, "ltx_theorem_thm")) {
			//get the plain text of the paragraph
			char *string = getDnmIteratorContent(myIterator);
			printf("Found theorem paragraph:\n%s\n\n", string);
			free(string);
		}
	} while (dnmIteratorNext(myIterator));
	
	//clean up
	free(myIterator);
	freeDNM(mydnm);
	xmlFreeDoc(mydoc);
	xmlCleanupParser();
	
	return 0;
}
```


The Other Libraries
-------------------

The other libraries are very small and easy to use, so I won't include any examples here.

