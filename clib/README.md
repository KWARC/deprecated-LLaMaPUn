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

