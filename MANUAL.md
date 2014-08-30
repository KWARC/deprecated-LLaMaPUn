The LLaMaPUn C library
======================

Content
-------
* `config/` configuration files
* `src/scripts/` utility scripts
* `src/llamapun` library source code
* `src/tasks` individual analysis tasks
* `src/examples` examples of library use
* `test/` test source code and data
* `third_party/` third party source code
* `CMakeLists.txt` CMake build script
* `LICENSE` copy of the license under which this software is distributed
* `Makefile` build makefile
* `README.md` documentation overview about the project
* `MANUAL.md` high-end overview of the LLaMaPUn library

[TODO] The `lib` and `t` directories are yet to be migrated away from the Perl setup.

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
cd src/script && sh jsonincludecheck.sh > /dev/null
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
	//Create DNM, with special tags (like math tags) normalized
	dnmPtr mydnm = create_DNM(xmlDocGetRootElement(mydoc), DNM_NORMALIZE_TAGS);

	//Do some fancy stuff with the DNM
	printf("The plaintext: %s\n", mydnm->plaintext);
	
	//clean up
	free_DNM(mydnm);
	xmlFreeDoc(mydoc);
	xmlCleanupParser();
	
	return 0;
}
```


The Other Libraries
-------------------

[TODO] Documentation is incoming soon.
