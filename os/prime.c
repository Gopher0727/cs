#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_N 1000

void get_prime(int p1[2]) {
        close(p1[1]);

        int     n;
        ssize_t ret = read(p1[0], &n, sizeof(n));

        if (ret == 0) {
                close(p1[0]);
                exit(0);
        }
        if (ret != sizeof(n)) {
                fprintf(stderr, "read failed or incomplete\n");
                exit(1);
        }

        printf("prime %d\n", n);
        fflush(stdout);

        int p2[2];
        if (pipe(p2) < 0) {
                perror("pipe");
                exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) {
                perror("fork");
                exit(1);
        }

        if (pid == 0) {
                close(p1[0]);
                get_prime(p2);
        } else {
                close(p2[0]);

                int m;
                while ((ret = read(p1[0], &m, sizeof(m))) > 0) {
                        if (ret != sizeof(m)) {
                                fprintf(stderr, "incomplete integer\n");
                                exit(1);
                        }

                        if (m % n != 0) {
                                if (write(p2[1], &m, sizeof(m)) != sizeof(m)) {
                                        perror("write");
                                        exit(1);
                                }
                        }
                }

                if (ret < 0) {
                        perror("read");
                        exit(1);
                }

                close(p1[0]);
                close(p2[1]);

                if (waitpid(pid, NULL, 0) < 0) {
                        perror("waitpid");
                        exit(1);
                }
        }

        exit(0);
}

int main(int argc, char* argv[]) {
        int limit = 35;

        if (argc > 2) {
                fprintf(stderr, "Usage: %s [2-%d]\n", argv[0], MAX_N);
                return 1;
        }

        if (argc == 2) {
                char* end;
                long  n = strtol(argv[1], &end, 10);

                if (*end != '\0' || end == argv[1] || n < 2 || n > MAX_N) {
                        fprintf(stderr, "Range must be 2-%d\n", MAX_N);
                        return 1;
                }

                limit = (int)n;
        }

        int p[2];
        if (pipe(p) < 0) {
                perror("pipe");
                return 1;
        }

        pid_t pid = fork();
        if (pid < 0) {
                perror("fork");
                return 1;
        }

        if (pid == 0) {
                get_prime(p);
        } else {
                close(p[0]);

                for (int i = 2; i <= limit; i++) {
                        if (write(p[1], &i, sizeof(i)) != sizeof(i)) {
                                perror("write");
                                return 1;
                        }
                }

                close(p[1]);

                if (waitpid(pid, NULL, 0) < 0) {
                        perror("waitpid");
                        return 1;
                }
        }

        return 0;
}
