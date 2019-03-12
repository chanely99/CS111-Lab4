#NAME: Chanel Young
#EMAIL: chanelyoung99@gmail.com
#UID: 604914166

default: 
	gcc -g -Wall -Wextra -lmraa -lm -UDUMMY lab4b.c -o lab4b
clean: 
	rm -rf lab4b lab4b-604914166.tar.gz
dist: 
	tar -cvzf lab4b-604914166.tar.gz lab4b.c README Makefile
