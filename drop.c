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

bool debugging = false;

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
	ssize_t size;
	unsigned int ctime;
	File_t *next;
};

void Trim(char *string)
{
        char *s = string;

        while (*s)
        {
                if (*s == '\r' || *s == '\n')
                {
                        *s = '\0';
                        return;
                }
                s++;
        }
}

void FileListFree(File_t *list)
{
	File_t *c = list;

	while (c)	
	{
		File_t *next = c->next;	
		free(c);

		c = next;
	}
}

void FileListAdd(File_t *list, char *path, ssize_t size, unsigned int mode, unsigned int ctime)
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
		c->size = size;
		c->ctime = ctime;
	}
}

#ifndef WINDOWS
#define SLASH '/'
#else
#define SLASH '\\'
#endif

#include <errno.h>

File_t * FilesInDirectory(const char *path)
{
	DIR *d = NULL;
	struct dirent *dirent = NULL;
	d = opendir(path);
	if (d == NULL)
	{
		printf("here %s %s\n", path, strerror(errno));
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

		struct stat fs;
		stat(path_full, &fs);
		
		if (S_ISDIR(fs.st_mode))
		{
			continue;
		}
		else
		{
			FileListAdd(list, path_full, fs.st_size, fs.st_mode, fs.st_ctime);
		}
	}
	
	closedir(d);

	return list;
}

typedef struct command_t command_t;
struct command_t {
	char *cmd; 
	char args[8192];
};

void execute(const char *path, command_t command)
{
	char ebuf[8192] = { 0 };
	snprintf(ebuf, sizeof(ebuf), "%s '%s' %s >/dev/null 2>&1",
					 command.cmd, path, command.args);
	system(ebuf);
}

File_t * FileExists(File_t *list, char *filename)
{
	File_t *f = list;

	while (f)
	{
		if (!strcmp(f->path, filename))
		{
			return f;
		}
		f = f->next;
	}

	return NULL;
}

bool ActOnFileDel(File_t *first, File_t *second, command_t command)
{
	File_t *f = first; 
	bool isChanged = false;

	while (f)
	{
		File_t *exists = FileExists(second, f->path);
		if (!exists)
		{
			printf("del file %s\n", f->path);
			//execute(f->path, command);
		}

		f = f->next;	
	}

	return isChanged;
}

bool ActOnFileMod(File_t *first, File_t *second, command_t command)
{
	File_t *c = second;
	bool isChanged = false;

	while (c)
	{
		File_t *exists = FileExists(first, c->path);
		if (exists)
		{
			if (c->size != exists->size)
			{
				printf("mod file %s\n", c->path);
				isChanged = true;
				//execute(c->path, command);
			}
		}

		c = c->next;	
	}

	return isChanged;
}

bool ActOnFileAdd(File_t *first, File_t *second, command_t command)
{
	File_t *f = second;
	bool isChanged = false;
	
	while (f)
	{
		File_t *exists = FileExists(first, f->path);
		if (!exists)
		{
			printf("add file %s\n", f->path);
			execute(f->path, command);
			isChanged = true;
		}	
	
		f = f->next;
	}

	return isChanged;
}

#define STATE_FILE_FORMAT "%s %u %u %u"
#define DROP_CONFIG_DIRECTORY ".drop"
#define DROP_CONFIG_FILE "drop.cfg"
#define DROP_STATE_FILE "state"

void SaveFileState(File_t *list)
{
	char state_file_path[PATH_MAX] = { 0 };

        snprintf(state_file_path, PATH_MAX, "%s%c%s", DROP_CONFIG_DIRECTORY, SLASH, DROP_STATE_FILE);

	FILE *f = fopen(state_file_path, "w");
	if (f == NULL)
	{
		Error("SaveFileState: fopen");
	}

	File_t *c = list->next;
	while (c)
	{
		fprintf(f, STATE_FILE_FORMAT, c->path, (unsigned int) c->size, (unsigned int)c->mode, (unsigned int) c->ctime);  
		fprintf(f, "\n");

		c = c->next;
	}

	fclose(f);
}

#define COMMAND_MAX 2048

typedef struct config_t config_t;
struct config_t {
	char directory[PATH_MAX];
	char remote_directory[PATH_MAX];
	char ssh_string[COMMAND_MAX];
};

void CompareFileLists(config_t config, File_t *first, File_t *second)
{
	command_t command;
	command.cmd = "scp";	

	snprintf(command.args, sizeof(command.args), "%s:%s", 
		config.ssh_string, config.remote_directory);	

	bool modifications = false;	
	modifications = ActOnFileAdd(first, second, command);
	modifications = ActOnFileDel(first, second, command);
	modifications = ActOnFileMod(first, second, command);

	// this is bullshit jeremy!
	if (1 || modifications)
	{
		SaveFileState(second);
	}
}


void Prepare(void)
{
	struct stat fstats;

	if (stat(DROP_CONFIG_DIRECTORY, &fstats) < 0)
	{
		mkdir(DROP_CONFIG_DIRECTORY, 0777);
	}

	if (! S_ISDIR(fstats.st_mode))
	{
		Error("%s is not a directory", DROP_CONFIG_DIRECTORY);
	}
}


File_t *ListFromStateFile(const char *state_file_path)
{
	FILE *f = fopen(state_file_path, "r");
	if (f == NULL)
	{
		Error("ListFromStateFile: %s", state_file_path);
	}

	char path[PATH_MAX] = { 0 };
	unsigned int s, m, t;

	File_t *list = calloc(1, sizeof(File_t));
	if (list == NULL)
	{
		Error("ListFromStateFile: calloc");
	}

	char line[1024] = { 0 };
	while((fgets(line, sizeof(line), f)) != NULL) 
	{
		Trim(line);
		int result = sscanf(line, STATE_FILE_FORMAT, path, &s, &m, &t);
		if (result == 4)
		{
			FileListAdd(list, path, s, m, t);
		}				
		memset(line, 0, sizeof(line));
	}

	fclose(f);

	return list;
}

File_t *FirstRun(const char *path)
{
	char state_file_path[PATH_MAX] = { 0 };

	snprintf(state_file_path, PATH_MAX, "%s%c%s", DROP_CONFIG_DIRECTORY, SLASH, DROP_STATE_FILE);

	struct stat fstats;
	
	File_t *list = NULL;
	
	if (stat(state_file_path, &fstats) < 0)
	{
		if (debugging)
		{
			printf("this is the first run\n");
		}

		list = FilesInDirectory(path);	
	} 
	else
	{
		list = ListFromStateFile(state_file_path);
	}

	
	return list;
}

// time between scans of path in MonitorPath
unsigned int changes_interval = 3;

void MonitorPath(const char *path, config_t config)
{
	File_t *file_list_one = FirstRun(path); // FilesInDirectory(path);	
	printf("watching: %s\n", path);

	for (;;)
	{
		sleep(changes_interval);

		if (debugging)
		{
			puts("PING!");
		}

		File_t *file_list_two = FilesInDirectory(path);

		CompareFileLists(config, file_list_one, file_list_two);
		
		FileListFree(file_list_one);	
		file_list_one = file_list_two;
	}
}

int ConfigValue(char *text, char *name, char *destination, ssize_t len)
{
	char *i = NULL;

	i = strstr(text, name);
	if (i)
	{
		i += strlen(name) + 1; 
		char *e = strchr(i, '\n');
		*e = '\0';
		char *value = strdup(i);
		*e = '\n'; // don't break text
		
		strlcpy(destination, value, len);  
		
		free(value);
	
		return 1;
	}

	return 0;
}

#define CONFIG_DIRECTORY "DIRECTORY"
#define CONFIG_REMOTE "REMOTE_DIRECTORY"
#define CONFIG_SSH "SSH_LOGIN"

void ConfigCheck(config_t config)
{
	bool isError = false;

	if (strlen(config.directory) == 0)
	{
		isError = true;
	}

	if (strlen(config.remote_directory) == 0)
	{
		isError = true;
	}

	if (strlen(config.ssh_string) == 0)	
	{
		isError = true;
	}

	if (isError)
	{
		Error("Broken config file %s", DROP_CONFIG_FILE);
	}
}

config_t *ConfigLoad(void)
{
	config_t *config = calloc(1, sizeof(config_t));

	char config_file_path[PATH_MAX] = { 0 };

	snprintf(config_file_path, PATH_MAX, "%s%c%s", DROP_CONFIG_DIRECTORY, SLASH, DROP_CONFIG_FILE);

	FILE *f = fopen(config_file_path, "r");
	if (f == NULL)
	{
		Error("Could not open %s", config_file_path);
	}

	char line[1024] = { 0 };
#define BUFSIZE 8192
#define CONFIG_MAX_LINES 20
	char map[BUFSIZE * CONFIG_MAX_LINES] = { 0 };

	int line_count = 0;	
	while ((fgets(line, sizeof(line), f)) != NULL)
	{
		if (line_count >= CONFIG_MAX_LINES)
		{
			Error("Unexpected content in %s", config_file_path);	
		}

		strlcat(map, line, sizeof(map));
		++line_count;
	}
	
	fclose(f);

	int result = ConfigValue(map, CONFIG_DIRECTORY, config->directory, PATH_MAX);
	if (!result)
	{
		// Could check config on missing option basis...
	}

	result = ConfigValue(map, CONFIG_REMOTE, config->remote_directory, PATH_MAX);
	if (!result)
	{
		// Could check config on missing option basis...
	}
	
	result = ConfigValue(map, CONFIG_SSH, config->ssh_string, sizeof(config->ssh_string));
	if (!result)
	{
		// Could check config on missing option basis...
	}

	// Check the configuration file generically	
	ConfigCheck(*config);

	return config;
}

// Lord Bogotron is reborn!!!


int main(int argc, char **argv)
{
	Prepare();

	config_t *config = ConfigLoad();	

	MonitorPath(config->directory, *config);

	return EXIT_SUCCESS;
}


