#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieve(int left_read_fd) 
{
    int prime;

    if (read(left_read_fd, &prime, sizeof(int)) != sizeof(int)) {
    	close(left_read_fd);
	exit(1);
    }

    fprintf(1, "prime %d\n", prime);

    int right_pipe[2];
    if (pipe(right_pipe) < 0) {
    	fprintf(2, "pipe failed\n");
	close(left_read_fd);
	exit(1);
    }

    int pid = fork();
    if (pid < 0) {
    	fprintf(2, "fork failed\n");
	close(left_read_fd);
	close(right_pipe[0]);
	close(right_pipe[1]);
	exit(1);
    }

    if (pid == 0) {
    	close(right_pipe[1]);
	sieve(right_pipe[0]);
    } else {
    	close(right_pipe[0]);

	int num;
	while (read(left_read_fd, &num, sizeof(int)) == sizeof(int)) {
	    if (num % prime != 0) {
	    	write(right_pipe[1], &num, sizeof(int));
	    }
	}

	close(left_read_fd);
	close(right_pipe[1]);
	wait(0);
    }

    exit(0);
}


int main(void) 
{
    int pipefd[2];
    if (pipe(pipefd) < 0) {
    	fprintf(2, "pipe failed\n");
	exit(1);
    }

    int pid = fork();
    if (pid < 0) {
    	fprintf(2, "fork failed\n");
	close(pipefd[0]);
	close(pipefd[1]);
	exit(1);
    }

    if (pid == 0) {
    	close(pipefd[1]);
	sieve(pipefd[0]);
    } else {
    	close(pipefd[0]);

	for (int i = 2; i <= 35; i ++) {
	    write(pipefd[1], &i, sizeof(int));
	}

	close(pipefd[1]);
	while (wait(0) > 0);
    }
    
    exit(0);
}
