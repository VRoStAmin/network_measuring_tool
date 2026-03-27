all:
	gcc -Wall -Wextra -o netmeasure main.c agrs.c client.c server.c tcp.c

clean:
	rm -f netmeasure
