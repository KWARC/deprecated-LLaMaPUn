#/usr/bin/bash

#On some platforms, json-c is installed to <json-c/json.h>,
#but on other platforms to <json/json.h>.

cat > .testjsoninclude.h <<'EOM'
#include <json-c/json.h>
EOM
if gcc -E .testjsoninclude.h
 then
  echo '#include <json-c/json.h>' > ../llamapun/jsoninclude.h
 else 
  echo '#include <json/json.h>' > ../llamapun/jsoninclude.h
 fi
rm .testjsoninclude.h

echo '#ifdef _json_c_version_h_' >> ../llamapun/jsoninclude.h
#echo '    #define JSON_OBJECT_OBJECT_GET_EX_DEFINED 1' >> jsoninclude.h
echo '  #if JSON_C_MAJOR_VERSION > 0' >> ../llamapun/jsoninclude.h
echo '    #define JSON_OBJECT_OBJECT_GET_EX_DEFINED 1' >> ../llamapun/jsoninclude.h
echo '  #endif' >> ../llamapun/jsoninclude.h
echo '  #if JSON_C_MINOR_VERSION > 9' >> ../llamapun/jsoninclude.h
echo '    #define JSON_OBJECT_OBJECT_GET_EX_DEFINED 1' >> ../llamapun/jsoninclude.h
echo '  #endif' >> ../llamapun/jsoninclude.h
echo '#endif' >> ../llamapun/jsoninclude.h

