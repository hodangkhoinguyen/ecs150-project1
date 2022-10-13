start: 
	gcc -Wall -Wextra -Werror sshell.c -o sshell

run:
	gcc -Wall -Wextra -Werror sshell.c -o sshell
	./sshell

clean:
	rm sshell
