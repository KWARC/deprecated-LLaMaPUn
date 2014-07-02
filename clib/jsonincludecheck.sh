#/usr/bin/bash

#On some platforms, json-c is installed to <json-c/json.h>,
#but on other platforms to <json/json.h>.

cat > .testjsoninclude.h <<'EOM'
#include <json-c/json.h>
EOM
if gcc -E .testjsoninclude.h
 then
  echo '#include <json-c/json.h>' > jsoninclude.h
 else 
  echo '#include <json/json.h>' > jsoninclude.h
 fi
rm .testjsoninclude.h
