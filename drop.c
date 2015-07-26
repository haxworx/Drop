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
// SCPs files to server
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


void FileListFree(File_t *list)
{
	File_t *c = list;

	while (c)	
	{
		File_t *next = c->next;	
		if (next)
		{
			free(c);
		}

		c = next;
	}
}

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

		strlcpy(c->path, path, PATH_MAX);
		c->mode = mode;
	}
}

#define SLASH '/'

File_t * FilesInDirectory(const char *path)
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
		snprintf(path_full, PATH_MAX, "%s%c%s", path, SLASH, dirent->d_name);

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

void MonitorDir(const char *path, const char *cmd, char *arg)
{
	File_t *file_list_one = FilesInDirectory(path);	

	for (;;)
	{
		sleep(3);

		File_t *file_list_two = FilesInDirectory(path);

		CompareFileLists(file_list_one, file_list_two,
			cmd, arg);
		
		FileListFree(file_list_one);	
		file_list_one = file_list_two;
	}
}

#define DIRECTORY "Pictures"

int main(int argc, char **argv)
{
	char *env_home = getenv("HOME");
	if (env_home == NULL)
	{
		Error("Could not get ENV 'HOME'");
	}

	char watch_dir[PATH_MAX] = { 0 };

	snprintf(watch_dir, PATH_MAX, "%s%c%s", env_home, SLASH, DIRECTORY);
	printf("watching: %s\n", watch_dir);

	MonitorDir(watch_dir, "scp", "user@host:");

	return EXIT_SUCCESS;
}
