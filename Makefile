SOURCE=drop.c
INSTALL_DIR=/usr/local/bin
CFLAGS=-std=c99 -Wall -Werror -pedantic
EXEC=drop

default:
	gcc $(CFLAGS) $(SOURCE) -o $(EXEC)
clean:
	rm $(EXEC)

install:
	cp drop $(INSTALL_DIR)
	chmod 755 $(INSTALL_DIR)/$(EXEC)
	echo "done!"
