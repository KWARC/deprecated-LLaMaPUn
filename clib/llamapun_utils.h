/* CorTeX utility functions and constants */

char* cortex_stringify_response(json_object* response, size_t *size);

json_object* cortex_response_json(char *annotations, char *message, int status);
