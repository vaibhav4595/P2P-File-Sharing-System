all: shell.c get_file_list.h
	gcc -lssl -lcrypto -g -Wall -Wunused-variable -Wreturn-type -o shell shell.c 
