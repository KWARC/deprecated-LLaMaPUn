#/usr/bin/bash

#determines the path of the repository root file and writes it into a header file

pwd | sed -e 's/^\(.*\)\/src\/scripts$/#define ROOT_PATH \"\1\"\n/' > ../llamapun/file_paths.h

