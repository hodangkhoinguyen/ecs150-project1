start: 
	gcc -Wall -Wextra -Werror sshell.c -o sshell
	./sshell

clean:
	rm sshell

aa:
	gcc -Wall -Wextra -Werror test.c -o test
	./test
