#include <json-c/json.h>
#ifdef _json_c_version_h_
  #if JSON_C_MAJOR_VERSION > 0
    #define JSON_OBJECT_OBJECT_GET_EX_DEFINED 1
  #endif
  #if JSON_C_MINOR_VERSION > 9
    #define JSON_OBJECT_OBJECT_GET_EX_DEFINED 1
  #endif
#endif
