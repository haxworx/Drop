/* 
   Copyright (c) 2015, Al Poole <netstar@gmail.com> All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer. 2. Redistributions in 
   binary form must reproduce the above copyright notice, this list of
   conditions and the following disclaimer in the documentation and/or other
   materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

 */

/* Thanks to Sam Watkins, Jason Pierce and Chris Rahm for being good friends
   over the years!
*/

#define _BSD_SOURCE 0x0001
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
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define h_addr h_addr_list[0]
/* Let's do a HTTP POST to a web-server and retrieve a URL. */

#ifndef WINDOWS
#define SLASH '/'
#else
#define SLASH '\\'
#endif

#ifndef strlcpy
size_t strlcpy(char *d, char const *s, size_t n)
{
	return snprintf(d, n, "%s", s);
}

size_t strlcat(char *d, char const *s, size_t n)
{
	return snprintf(d, n, "%s%s", d, s);
}
#endif

bool debugging = false;

char *directory = NULL;
const char *username = NULL;
const char *password = NULL;



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


char *PathStrip(char *path)
{
	char *t = strrchr(path, '\\');
	if (t)
	{
		t++;
		return t;
	}

	t = strrchr(path, '/');
	if (t)
	{
		t++;
		return t;
	}

	return path;
}

int Connect(char *hostname, int port)
{
	int sock;
	struct hostent *host;
	struct sockaddr_in host_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		Error("socket()");
	}

	host = gethostbyname(hostname);
	if (host == NULL)
	{
		Error("gethostbyname()");
	}

	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(port);
	host_addr.sin_addr = *((struct in_addr *)host->h_addr);
	memset(&host_addr.sin_zero, 0, 8);

	int status = connect(sock, (struct sockaddr *)&host_addr,
			     sizeof(struct sockaddr));

	if (status == 0)
	{
		return sock;
	}

	return 0;
}

#define REMOTE_URI "/drop.cgi"
#define REMOTE_HOST "haxlab.org"
#define REMOTE_PORT 80

ssize_t Write(int sock, char *buf, int len)
{
	if (debugging)
	{
		printf("%s", buf);
	}
	return write(sock, buf, len);
}


bool RemoteFileDel(char *file)
{
	char path[PATH_MAX] = { 0 };

	snprintf(path, sizeof(path), "%s%c%s", directory, SLASH, file);
	if (debugging)
	{
		printf("the path is %s\n", path);
	}

	int sock = Connect(REMOTE_HOST, REMOTE_PORT);
	if (!sock)
	{
		Error("Could not Connect()");
	}

	int content_length = 0;
	char *file_from_path = PathStrip(path);

	char post[8192] = { 0 };
	char *fmt =
		"POST %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Content-Length: %d\r\n"
		"Username: %s\r\n"
		"Password: %s\r\n" "Filename: %s\r\n" "Action: DEL\r\n\r\n";

	snprintf(post, sizeof(post), fmt, REMOTE_URI, REMOTE_HOST,
		 content_length, username, password, file_from_path);

	Write(sock, post, strlen(post));

	close(sock);

	return true;
}

bool RemoteFileAdd(char *file)
{
	char path[PATH_MAX] = { 0 };

	snprintf(path, sizeof(path), "%s%c%s", directory, SLASH, file);
	if (debugging)
	{
		printf("the path is %s\n", path);
	}

	int sock = Connect(REMOTE_HOST, REMOTE_PORT);
	if (!sock)
	{
		Error("Could not Connect()");
	}

	struct stat fstats;
	if (strlen(path) == 0)
	{
		return false;
	}

	if (stat(path, &fstats) < 0)
	{
		return false;
	}

	FILE *f = fopen(path, "rb");
	if (f == NULL)
	{
		Error("Unable to open filename %s, this should not happen!",
		      path);
	}

#define CHUNK 1024

	char buffer[CHUNK + 1] = { 0 };

	int content_length = fstats.st_size;

	char *file_from_path = PathStrip(path);

	char post[8192] = { 0 };
	char *fmt =
		"POST %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Content-Length: %d\r\n"
		"Username: %s\r\n"
		"Password: %s\r\n" "Filename: %s\r\n" "Action: ADD\r\n\r\n";

	snprintf(post, sizeof(post), fmt, REMOTE_URI, REMOTE_HOST,
		 content_length, username, password, file_from_path);

	Write(sock, post, strlen(post));

	int total = 0;

	int size = content_length;

	while (size)
	{
		while (1)
		{
			int count = fread(buffer, 1, CHUNK, f);
			int bytes = write(sock, buffer, count);
			if (bytes == 0)
			{
				break;
			}
			else
			{
				size -= bytes;
				total += bytes;
			}
		}
	}

	close(sock);
	fclose(f);

	return true;
}

typedef struct File_t File_t;
struct File_t
{
	char path[PATH_MAX];
	unsigned int mode;
	ssize_t size;
	unsigned int mtime;
	File_t *next;
};

typedef struct Mod_t Mod_t;
struct Mod_t
{
	char filename[PATH_MAX];
	int type;
};

// haircut anyone???
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

void WindowsSanifyPath(char *path)
{
	char *p = path;

	while (*p)
	{
		if (*p == '\\')
		{
			*p = '/';
		}
		
		p++;
	}
}


void FileListFree(File_t * list)
{
	File_t *c = list;

	while (c)
	{
		File_t *next = c->next;
		free(c);

		c = next;
	}
}


void FileListAdd(File_t * list, char *path, ssize_t size, unsigned int mode,
		 unsigned int mtime)
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

		c = c->next;
		c->next = NULL;

		char *p = PathStrip(path);
		
		WindowsSanifyPath(p);
		
		strlcpy(c->path, p, PATH_MAX);
		c->mode = mode;
		c->size = size;
		c->mtime = mtime;
	}
}

#define CHANGE_ADD 0x01
#define CHANGE_MOD 0x02
#define CHANGE_DEL 0x03

File_t *FileExists(File_t * list, char *filename)
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

int ActOnFileDel(File_t * first, File_t * second)
{
	File_t *f = first;
	int isChanged = 0;
	
	while (f)
	{
		File_t *exists = FileExists(second, f->path);
		if (!exists)
		{
			printf("del file %s\n", f->path);
			RemoteFileDel(f->path);
			isChanged++;
		}

		f = f->next;
	}

	return isChanged;
}

int ActOnFileMod(File_t * first, File_t * second)
{
	File_t *f = second;
	int isChanged = 0;
	
	while (f)
	{
		File_t *exists = FileExists(first, f->path);
		if (exists)
		{
			if (f->mtime != exists->mtime)
			{
				printf("mod file %s\n", f->path);
				RemoteFileAdd(f->path);
				isChanged++;
			}
		}

		f = f->next;
	}
	
	return isChanged;
}

int ActOnFileAdd(File_t * first, File_t * second)
{
	File_t *f = second;
	int isChanged = 0;
	
	while (f)
	{
		File_t *exists = FileExists(first, f->path);
		if (!exists)
		{
			printf("add file %s\n", f->path);
			RemoteFileAdd(f->path);
			isChanged++;
		}

		f = f->next;
	}
	
	return isChanged;
}


#include <errno.h>

File_t *FilesInDirectory(const char *path)
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
		if (!strncmp(dirent->d_name, ".", 1))
		{
			continue;
		}

		char path_full[PATH_MAX] = { 0 };
		snprintf(path_full, PATH_MAX, "%s%c%s", path, SLASH,
			 dirent->d_name);

		struct stat fs;
		stat(path_full, &fs);

		if (S_ISDIR(fs.st_mode))
		{
			continue;
		}
		else
		{
			FileListAdd(list, path_full, fs.st_size, fs.st_mode,
				    fs.st_mtime);
		}
	}

	closedir(d);

	return list;
}

#define STATE_FILE_FORMAT "%s\t%u\t%u\t%u"
#define DROP_CONFIG_DIRECTORY ".drop"
#define DROP_CONFIG_FILE "drop.cfg"
#define DROP_STATE_FILE "state"

void SaveFileState(File_t * list)
{
	char state_file_path[PATH_MAX] = { 0 };

	snprintf(state_file_path, PATH_MAX, "%s%c%s", DROP_CONFIG_DIRECTORY,
		 SLASH, DROP_STATE_FILE);

	FILE *f = fopen(state_file_path, "w");
	if (f == NULL)
	{
		Error("SaveFileState: fopen");
	}

	File_t *c = list->next;
	while (c)
	{
		// we could refactor this...just include stat struct rather
		// than individual members...ahhh...f*ck it!
		fprintf(f, STATE_FILE_FORMAT, c->path, (unsigned int)c->size,
			(unsigned int)c->mode, (unsigned int)c->mtime);
		fprintf(f, "\n");

		c = c->next;
	}

	fclose(f);
}

bool GetStateFileValues(char *text, char *buf, int *size, int *mode, int *ctime)
{
	const char delim[] = "\t\0";
	
	char *ptr = strtok(text, delim);
	if (ptr)
	{
		snprintf(buf, PATH_MAX, "%s", ptr);
		ptr = strtok(NULL, delim);
		if (ptr)
		{
			*size = atoi(ptr);
			ptr = strtok(NULL, delim);
			if (ptr)
			{
				*mode = atoi(ptr);
				ptr = strtok(NULL, delim);
				if (ptr)
				{
					*ctime = atoi(ptr);
					return true;
				
				}
			}
		}
	}
	
	return false;	
}

File_t *ListFromStateFile(const char *state_file_path)
{
	FILE *f = fopen(state_file_path, "r");
	if (f == NULL)
	{
		Error("ListFromStateFile: %s", state_file_path);
	}

	char path[PATH_MAX] = { 0 };
	
	File_t *list = calloc(1, sizeof(File_t));
	if (list == NULL)
	{
		Error("ListFromStateFile: calloc");
	}

	char line[1024] = { 0 };
	while ((fgets(line, sizeof(line), f)) != NULL)
	{
		Trim(line);
		
		/*int result = sscanf(line, STATE_FILE_FORMAT, path, &s, &m, &t);
		if (result == 4)
		{
			
			FileListAdd(list, path, s, m, t);
		}
		*/
		int mtime, size, mode;
		int status = GetStateFileValues(line, path, &size, &mode, &mtime);
		if (status)
		{
			FileListAdd(list, path, size, mode, mtime);
		}
	}

	fclose(f);

	return list;
}

#define COMMAND_MAX 2048

typedef struct config_t config_t;
struct config_t
{
	char directory[PATH_MAX];
	char remote_directory[PATH_MAX];
	char ssh_string[COMMAND_MAX];
};

void CompareFileLists(File_t * first, File_t * second)
{
	bool store_state = false;
	int modifications = 0;
	
	modifications += ActOnFileAdd(first, second);
	modifications += ActOnFileDel(first, second);
	modifications += ActOnFileMod(first, second);
	
	if (modifications)
	{
		printf("total of %d changes\n\n", modifications);
		store_state = true;
	}
	
	if (store_state)
	{
		SaveFileState(second);
	}
}

// Am I going to do a poo Jez???
void Prepare(void)
{
	struct stat fstats;

	if (stat(DROP_CONFIG_DIRECTORY, &fstats) < 0)
	{
		mkdir(DROP_CONFIG_DIRECTORY, 0777);
	}
}

File_t *FirstRun(char *path)
{
	char state_file_path[PATH_MAX] = { 0 };

	snprintf(state_file_path, PATH_MAX, "%s%c%s", DROP_CONFIG_DIRECTORY,
		 SLASH, DROP_STATE_FILE);

	struct stat fstats;

	File_t *list = NULL;

	if (stat(state_file_path, &fstats) < 0)
	{
		if (debugging)
		{
			printf("this is the first run\n");
		}

		WindowsSanifyPath(path);
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

void MonitorPath(char *path)
{
	File_t *file_list_one = FirstRun(path);	// FilesInDirectory(path); 
	printf("watching: %s\n", path);
	printf("syncing: http://%s/%s\n", REMOTE_HOST, username);

	for (;;)
	{
		sleep(changes_interval);

		if (debugging)
		{
			puts("PING!");
		}

		File_t *file_list_two = FilesInDirectory(path);

		CompareFileLists(file_list_one, file_list_two);

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
		*e = '\n';	// don't break text
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

	snprintf(config_file_path, PATH_MAX, "%s%c%s", DROP_CONFIG_DIRECTORY,
		 SLASH, DROP_CONFIG_FILE);

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

	int result =
		ConfigValue(map, CONFIG_DIRECTORY, config->directory,
			    PATH_MAX);
	if (!result)
	{
		// Could check config on missing option basis...
	}

	result = ConfigValue(map, CONFIG_REMOTE, config->remote_directory,
			     PATH_MAX);
	if (!result)
	{
		// Could check config on missing option basis...
	}

	result = ConfigValue(map, CONFIG_SSH, config->ssh_string,
			     sizeof(config->ssh_string));
	if (!result)
	{
		// Could check config on missing option basis...
	}

	// Check the configuration file generically 
	ConfigCheck(*config);

	return config;
}

void About(void)
{
	printf("Copyright (c) 2015, Al Poole <netstar@gmail.com> All rights reserved.\n");
}

void AboutRemoteURL(void)
{
	printf("Remote URL http://%s/%s\n", REMOTE_HOST, username);
}

void Version(void)
{
	About();
	printf("Drop version 0.0.2a\n");
}

void Usage(void)
{
	Version();
	Error("Drop didn't start");
}

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		Usage();
	}

	// weird hack...
	stdout = stderr;

	directory = argv[1];
	username = argv[2];
	password = argv[3];

	Prepare();

	Version();

	MonitorPath(directory);

	return EXIT_SUCCESS;
}
