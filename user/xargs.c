#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define MAX_LINE 50
#define MAXARG 10

int read_line(char *buf, int max_len)
{
    int i = 0;
    char c;

    while(i < max_len - 1) {
    	if (read(0, &c, sizeof(char)) != sizeof(char)) {
	    if (i == 0) {   // EOF
	    	return -1;
	    }
	    printf("line end >> EOF\n");
	    break;
	} 

	if (c == '\n') {
            printf("line end >> enter\n");
	    break;
	}

	buf[i++] = c;
    }

    buf[i] = '\0';
    return i;
}

int split_args(char *str, char **argv, char max_arg)
{
    int argc = 0;
    char *p = str;

    while (*p != '\0' && argc < max_arg - 1) {
    	while (*p == ' ' && *p != '\0') {
	    p ++;
	}

	if (*p == '\0') break;

	argv[argc++] = p;
	while (*p != ' ' && *p != '\0') {
	    p ++;
	}

	if (*p != '\0') {
	    *p ++ = '\0';
	}
    }
    argv[argc] = 0;
    return argc;

}

int main(int argc, char **argv)
{
    char line_buf[MAX_LINE];
    char *cmd_argv[MAXARG];
    int cmd_argc = 0;

    if (argc < 2) {
    	cmd_argv[cmd_argc ++] = "echo";
    } else {
    	for (int i = 1; i < argc; i ++) {
	    cmd_argv[cmd_argc ++] = argv[i];
	}
    }

    while (1) {
    	int line_len = read_line(line_buf, MAX_LINE);

	if (line_len == -1) {
	    printf("main process end >> EOF\n");
	    break;
	}

	if (line_len == 0) {
	    continue;
	}
	
	char *line_argv[MAXARG];
	int line_argc = split_args(line_buf, line_argv, MAXARG - cmd_argc);
	int total_argc = cmd_argc;

	for (int i = 0; i < line_argc; i ++) {
	    cmd_argv[total_argc ++] = line_argv[i];
	}

	cmd_argv[total_argc] = 0;

	int pid = fork();

	if (pid < 0) {
	    fprintf(2, "fork() error!\n");
	    exit(1);
	} else if (pid == 0) {
	    exec(cmd_argv[0], cmd_argv);
	    fprintf(2, "exec() error!\n");
	    exit(1);
	} else {
	    wait(0);
	}
        
	cmd_argv[cmd_argc] = 0;
    }

    exit(0);
}
