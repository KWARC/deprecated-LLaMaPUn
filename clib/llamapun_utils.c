// Some everyday C includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// JSON
#include <json-c/json.h>

char* cortex_stringify_response(json_object* response, size_t *size) {
  const char* string_response = json_object_to_json_string(response);
  *size = strlen(string_response);
  return (char *)string_response; }

json_object* cortex_response_json(char *annotations, char *message, int status) {
  json_object *response = json_object_new_object();
  json_object *json_log = json_object_new_string(message);
  json_object *json_status = json_object_new_int(status);
  json_object *json_annotations = json_object_new_string(annotations);

  json_object_object_add(response,"annotations", json_annotations);
  json_object_object_add(response,"log", json_log);
  json_object_object_add(response,"status", json_status);
  return response; }
