wc -l common.h slice.* source.* entry.* util.* dir.* mozilla.* syncer.* editor.* window.c main.c | awk -F ' ' '{s+=$1} END {print s}'

