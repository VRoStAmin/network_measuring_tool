all:
	gcc -Wall -Wextra -pthread -o netmeasure main.c agrs.c client.c server.c tcp.c udp.c -lm

clean:
	rm -f netmeasure
