#some small executables for testing
add_executable(evaluate_tokenization evaluate_tokenization.c)
target_link_libraries(evaluate_tokenization utils document_loader ${LIBXML2_LIBRARIES})