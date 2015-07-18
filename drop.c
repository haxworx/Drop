/*
Copyright (c) 2015, Al Poole <netstar@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// Watch the directory e.g. $HOME/Pictures
// then if there's a new one use scp or whatever 
// and copy it to the server...
// I'm using it like a drop box, it runs in bg
// and whenever I find a good image online I drag it to $HOME/Pictures
// then it's saved!!!

// Maybe you want to use another agent or I advise to set no pass on your
// SSH keys maybe!

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>

#include <unistd.h>

void Error(char *fmt, ...)
{
        char message[8192] = { 0 };
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(message, 8192, fmt, ap);
        fprintf(stderr, "Error: %s\n", message);
        va_end(ap);

        exit(EXIT_FAILURE);
}

typedef struct File_t File_t;
struct File_t {
	char path[PATH_MAX];
	unsigned int mode;
	File_t *next;
};

void FileListAdd(File_t *list, char *path, unsigned int mode)
{
	File_t *c = list;
	
	while (c->next)
	{
		c = c->next;
	}

	if (c->next == NULL)
	{
		c->next = calloc(1, sizeof(File_t));
		if (c->next == NULL)
		{
			Error("calloc()");
		}
		
		c = c->next; c->next = NULL;

		strncpy(c->path, path, PATH_MAX);
		c->mode = mode;
	}
}

File_t * CheckDirFiles(const char *path)
{
	DIR *d = NULL;
	struct dirent *dirent = NULL;

	
	d = opendir(path);
	if (d == NULL)
	{
		Error("opendir()");
	}

	File_t *list = calloc(1, sizeof(File_t));
	if (list == NULL)
	{
		Error("calloc()");
	}

	while ((dirent = readdir(d)) != NULL)
	{
		if (! strncmp(dirent->d_name, ".", 1))
		{
			continue;
		}
		
		char path_full[PATH_MAX] = { 0 };
		snprintf(path_full, PATH_MAX, "%s/%s", path, dirent->d_name);

		struct stat fstats;
		stat(path_full, &fstats);
		
		if (S_ISDIR(fstats.st_mode))
		{
			continue;
		}
		else
		{
			FileListAdd(list, path_full, fstats.st_mode);
		}
	}
	
	closedir(d);

	return list;
}

void CompareFileLists(File_t *first, File_t *second, const char *cmd, char *arg)
{
	File_t *f = second;

	while (f)
	{
		File_t *n = first;
		bool Found = false;
		while (n)
		{
			if (!strcmp(f->path, n->path))
			{
				Found = true;
			}

			n = n->next;
		}
		if (! Found)
		{
			char execute[8192] = { 0 };
			snprintf(execute, 8192, "%s %s %s", cmd, f->path, arg);
			system(execute);
			printf("new file %s\n", f->path);
			printf("executing: %s\n", execute);
		}

		f = f->next;	
	}

	

}
void ChangeInDirectory(const char *path, const char *cmd, char *arg)
{
	File_t *file_list_zero = NULL;
	File_t *file_list_one = NULL; 

	for (;;)
	{
		file_list_zero = CheckDirFiles(path);	
		sleep(3);
		file_list_one  = CheckDirFiles(path);
		CompareFileLists(file_list_zero, file_list_one,
			cmd, arg);
		free(file_list_zero);
		free(file_list_one); 
	}

}
int main(int argc, char **argv)
{
	ChangeInDirectory("/home/al/Pictures", "scp", "user@host:");

	return EXIT_SUCCESS;
}
