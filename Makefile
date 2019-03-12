#NAME: Chanel Young
#EMAIL: chanelyoung99@gmail.com
#UID: 604914166

default: 
	gcc -Wall -Wextra -g -o lab4b lab4b.c
clean: 
	rm -rf lab4b lab4b-604914166.tar.gz
dis: 
	tar -cvzf lab3a-604914166.tar.gz lab4b.c README Makefile
