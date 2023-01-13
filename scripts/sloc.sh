wc -l configure.ac Makefile.in common.h util.* slice.* map.* json.* \
	source.* entry.* \
	dir.* mozilla.* keep.* \
	cfg.* ipc.* server.* \
	ui/listbox.* ui/editor.* ui/window.c cmd.* \
	perl.* search.* main.c | awk -F ' ' '{s+=$1} END {print s}'

