SOURCE=drop.c
CFLAGS=-std=c99 -Wall -Werror -pedantic
EXEC=drop

default:
	gcc $(CFLAGS) $(SOURCE) -o $(EXEC)
clean:
	rm $(EXEC)
