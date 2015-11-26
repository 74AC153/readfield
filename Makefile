readfield: readfield.c
	gcc -g3 -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=2 -o readfield readfield.c

clean:
	rm readfield
