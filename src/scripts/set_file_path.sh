#/usr/bin/bash

#determines the path of the repository root file and writes it into a header file

pwd | sed -e 's/^\(.*\)\/src\/scripts$/#define LLAMAPUN_ROOT_PATH \"\1\/\"/' > ../llamapun/local_paths.h
pwd | sed -e 's/^\(.*\)\/src\/scripts$/#define LIBTEXT_LANGUAGES_PATH \"\1\/third-party\/libtextcat-2.2\/langclass\/LM\/\"/' >> ../llamapun/local_paths.h
