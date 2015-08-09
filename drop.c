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

/*
Lord Bogotron says,

"This maniac uses OpenBSD for development. OpenBSD provides additional string
manipulation functions not offered by the standard C library. I'm sure we'll
port it over for you later on."

*/
#define _XOPEN_SOURCE 99999
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
/* Let's do a HTTP POST to a web-server and retrieve a URL.
*/

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
    host_addr.sin_addr = *((struct in_addr *) host->h_addr);
    memset(&host_addr.sin_zero, 0, 8);
    
    int status = connect(sock, (struct sockaddr *) &host_addr,
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

const char *boundary = "--------------------56739374637362";

ssize_t Write(int sock, char *buf, int len)
{
    printf("%s", buf);
    return write(sock, buf, len);
}

void Content_Disposition(int sock, char *name, char *value, int len, char *filename)
{
    char content_disposition[1024] = { 0 };
    char content[8192] = { 0 };
    snprintf(content_disposition, sizeof(content_disposition), "Content-Disposition: form-data; name=\"%s\"\r\n\r\n", name);
    snprintf(content, sizeof(content),  "%s\r\n%s\r\n", value, boundary);
    
    Write(sock, content_disposition, strlen(content_disposition));
    Write(sock, content, strlen(content));
}

bool HTTP_Post_File(char *path)
{
    int sock = Connect(REMOTE_HOST, REMOTE_PORT);
    if (! sock)
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
        Error("Unable to open filename %s, this should not happen!", path);
    }
#define CHUNK 1024

    char buffer[CHUNK + 1] = { 0 };
    
    int size = fstats.st_size;
    int total = 0;
    
    char method[1024] = { 0 };
    snprintf(method, sizeof(method), "POST %s HTTP/1.1\r\n", REMOTE_URI);
    Write(sock, method, strlen(method));
    
    char host[1024] = { 0 };
    snprintf(host, sizeof(host),"Host: %s\r\n", REMOTE_HOST);
    Write(sock, host, strlen(host));
    
    int length = size + 8;
    
    char content_length[1024] = { 0 };
    snprintf(content_length, sizeof(content_length), "Content-Length: %d\r\n", length);
    Write(sock, content_length, strlen(content_length));
    
    char content_type[1024] = { 0 };
    snprintf(content_type, sizeof(content_type), "Content-Type: multipart/form-data; boundary=%s\r\n", boundary);
    Write(sock, content_type, strlen(content_type));
    
    
    char split[1024] = { 0 };
    snprintf(split, sizeof(split), "%s\r\n",boundary);
    Write(sock, split, strlen(split));
    
    Content_Disposition(sock, "username", "aldo", 0, NULL);
    Content_Disposition(sock, "password", "aldo", 0, NULL);
    char buf[8192] = { 0 };
    
    snprintf(buf, sizeof(buf), "Content-Disposition: form-data; name=\"filename\"; filename=\"a.sex\"\r\n");
    Write(sock, buf, strlen(buf));
    
    snprintf(buf, sizeof(buf), "Content-Type: multipart/mixed\r\n\r\n");
    Write(sock, buf, strlen(buf));
    
    while (size)
    {
        while (1)
        {
            int count = fread(buffer, 1, CHUNK, f);
            int bytes = Write(sock, buffer, count);
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
    
    
    snprintf(split, sizeof(split), "\r\n%s\r\n\r\n",boundary);
    Write(sock, split, strlen(split));
    
    printf("http post done %s\n\n", path);
    
    return true;
}
typedef struct File_t File_t;
struct File_t {
	char path[PATH_MAX];
	unsigned int mode;
	ssize_t size;
	unsigned int ctime;
	File_t *next;
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

bool ActOnFileDel(File_t *first, File_t *second)
{
	File_t *f = first; 
	bool isChanged = false;

	while (f)
	{
		File_t *exists = FileExists(second, f->path);
		if (!exists)
		{
			printf("del file %s\n", f->path);
			isChanged = true;
		}

		f = f->next;	
	}

	return isChanged;
}

bool ActOnFileMod(File_t *first, File_t *second)
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
                HTTP_Post_File(c->path);
				isChanged = true;
			}
		}

		c = c->next;	
	}

	return isChanged;
}

bool ActOnFileAdd(File_t *first, File_t *second)
{
	File_t *f = second;
	bool isChanged = false;
	
	while (f)
	{
		File_t *exists = FileExists(first, f->path);
		if (!exists)
		{
			printf("add file %s\n", f->path);
            HTTP_Post_File(f->path);
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
		// we could refactor this...just include stat struct rather than individual members...ahhh...f*ck it!
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

void CompareFileLists(File_t *first, File_t *second)
{
	bool store_state = false;
	bool modifications = false;	

	modifications = ActOnFileAdd(first, second);
	if (modifications)
	{
		store_state = true;
	}

	modifications = ActOnFileDel(first, second);
	if (modifications)
	{
		store_state = true;	
	}

	modifications = ActOnFileMod(first, second);
	if (modifications)
	{
		store_state = true;
	}

	// this s/is/was bullshit jeremy!
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

void MonitorPath(const char *path)
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


void Usage(void)
{
	Error("drop <DIRECTORY>");
}
// I think I'm going to blow my beans!

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		Usage();
	}

	// weird hack...
	stdout = stderr;

	char *directory = argv[1];
	
	Prepare();

	MonitorPath(directory);

	return EXIT_SUCCESS;
}


