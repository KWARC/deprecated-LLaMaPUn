if valgrind --leak-check=full $1 2>&1 >/dev/null | grep -P " [123456789][,\d]* blocks"; then
	echo "$1 failed"
	exit 1
else
	echo "$1 passed"
	exit 0
fi
