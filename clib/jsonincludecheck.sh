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