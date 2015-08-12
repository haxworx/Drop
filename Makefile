SOURCE=drop.c
INSTALL_DIR=/usr/local/bin
CFLAGS=-std=c99 -Wall -Werror -pedantic 
EXEC=drop
OPTIONS=
HAIKU=-lnetwork
WINDOWS=-DWINDOWS -static

default:
	gcc $(CFLAGS) $(OPTIONS) $(WINDOWS) $(SOURCE) -o $(EXEC)

haiku: 
	gcc $(CFLAGS) $(OPTIONS) $(HAIKU) $(SOURCE) -o $(EXEC)

clean:
	rm $(EXEC)

install:
	cp drop $(INSTALL_DIR)
	chmod 755 $(INSTALL_DIR)/$(EXEC)
	echo "done!"
