SOURCE=drop.c
INSTALL_DIR=/usr/local/bin
CFLAGS=-std=c99 -Wall -Werror -pedantic 
EXEC=drop
OPTIONS=
HAIKU=-lnetwork
WINDOWS=-DWINDOWS -static

default:
	gcc $(CFLAGS) $(OPTIONS) $(WINDOWS) $(SOURCE) -o $(EXEC)

clean:
	rm $(EXEC)
