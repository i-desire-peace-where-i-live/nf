wc -l configure.ac Makefile.in common.h util.* slice.* map.* json.* \
	entry.* \
	providers/* \
	ipc.* \
	ui/*.c ui/*.h \
	perl.* search.* main.c | awk -F ' ' '{s+=$1} END {print s}'

