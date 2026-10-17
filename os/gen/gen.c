#include "gen.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// 每个值 = tag + 长度 + 负载
enum { TAG_INT = 1, TAG_STR = 2 };

struct gen {
        int         up[2];    // 生成器 -> 调用者
        int         down[2];  // 调用者 -> 生成器
        pid_t       pid;
        int         started;  // 第一次 next 不送值
        int         closed;
        const char* str;  // gen_open_str 用
        int         mode;
        gen_t*      prev;  // 存活链表，atexit 兜底用
        gen_t*      next;
};

static gen_t* g_live;

static void gen_unlink(gen_t* g) {
        if (g->prev != NULL) {
                g->prev->next = g->next;
        } else {
                g_live = g->next;
        }
        if (g->next != NULL) {
                g->next->prev = g->prev;
        }
        g->prev = NULL;
        g->next = NULL;
}

// 进程退出时兜底收掉没写 GEN_AUTO 的生成器
static void gen_reap_all(void) {
        while (g_live != NULL) {
                gen_t* g = g_live;
                gen_close(g);
                free(g);
        }
}

static int write_int(int fd, int value) {
        ssize_t n;
        do {
                n = write(fd, &value, sizeof(value));
        } while (n < 0 && errno == EINTR);

        return n == (ssize_t)sizeof(value);
}

static int read_int(int fd, int* out) {
        ssize_t n;
        do {
                n = read(fd, out, sizeof(*out));
        } while (n < 0 && errno == EINTR);

        return n == (ssize_t)sizeof(*out);
}

static int write_all(int fd, const void* p, size_t n) {
        const char* c = p;

        while (n > 0) {
                ssize_t w = write(fd, c, n);
                if (w < 0) {
                        if (errno == EINTR) {
                                continue;
                        }
                        return 0;
                }
                c += w;
                n -= (size_t)w;
        }
        return 1;
}

static int read_all(int fd, void* p, size_t n) {
        char* c = p;

        while (n > 0) {
                ssize_t r = read(fd, c, n);
                if (r < 0) {
                        if (errno == EINTR) {
                                continue;
                        }
                        return 0;
                }
                if (r == 0) {
                        return 0;
                }
                c += r;
                n -= (size_t)r;
        }
        return 1;
}

// 类型不匹配时把负载读掉，别把管道留在半截消息上
static void drain(int fd, size_t n) {
        char buf[128];

        while (n > 0) {
                size_t chunk = n < sizeof(buf) ? n : sizeof(buf);
                if (!read_all(fd, buf, chunk)) {
                        return;
                }
                n -= chunk;
        }
}

static gen_t* gen_new(void) {
        gen_t* g = calloc(1, sizeof(*g));
        if (g == NULL) {
                perror("calloc");
                exit(1);
        }
        if (pipe(g->up) < 0 || pipe(g->down) < 0) {
                perror("pipe");
                exit(1);
        }

        signal(SIGPIPE, SIG_IGN);  // 生成器先死时写管道只返回 EPIPE，别杀进程
        return g;
}

static gen_t* gen_start(gen_t* g, gen_body_fn body) {
        static int registered;

        g->pid = fork();
        if (g->pid < 0) {
                perror("fork");
                exit(1);
        }
        if (g->pid == 0) {
                close(g->up[0]);
                close(g->down[1]);

                body(g);

                close(g->up[1]);
                close(g->down[0]);
                _exit(0);
        }

        close(g->up[1]);
        close(g->down[0]);

        if (!registered) {
                atexit(gen_reap_all);
                registered = 1;
        }
        g->prev = NULL;
        g->next = g_live;
        if (g_live != NULL) {
                g_live->prev = g;
        }
        g_live = g;

        return g;
}

gen_t* gen_open(gen_body_fn body) {
        return gen_start(gen_new(), body);
}

int gen_yield(gen_t* g, int value) {
        int sent;
        if (!write_int(g->up[1], TAG_INT) || !write_int(g->up[1], (int)sizeof(value)) || !write_int(g->up[1], value)) {
                _exit(0);  // 调用者提前收工
        }
        if (!read_int(g->down[0], &sent)) {
                _exit(0);
        }
        return sent;
}

static int yield_bytes(gen_t* g, const char* p, size_t len) {
        int sent;
        if (!write_int(g->up[1], TAG_STR) || !write_int(g->up[1], (int)len) || !write_all(g->up[1], p, len)) {
                _exit(0);
        }
        if (!read_int(g->down[0], &sent)) {
                _exit(0);
        }
        return sent;
}

int gen_yield_str(gen_t* g, const char* s) {
        return yield_bytes(g, s, strlen(s));
}

int gen_send(gen_t* g, int in, int* out) {
        int tag;
        int len;
        int v;

        if (g == NULL || g->closed) {
                return 0;
        }
        if (g->started) {
                if (!write_int(g->down[1], in)) {
                        gen_close(g);
                        return 0;
                }
        }
        g->started = 1;

        if (!read_int(g->up[0], &tag) || !read_int(g->up[0], &len)) {
                gen_close(g);
                return 0;
        }
        if (tag != TAG_INT || len != (int)sizeof(v)) {
                drain(g->up[0], len > 0 ? (size_t)len : 0);
                gen_close(g);
                return 0;
        }
        if (!read_int(g->up[0], &v)) {
                gen_close(g);
                return 0;
        }

        *out = v;
        return 1;
}

int gen_next(gen_t* g, int* out) {
        return gen_send(g, 0, out);
}

int gen_next_rune(gen_t* g, char* buf, size_t cap) {
        int ack = 0;
        int tag;
        int len;

        if (g == NULL || g->closed) {
                return 0;
        }
        if (g->started) {
                if (!write_int(g->down[1], ack)) {
                        gen_close(g);
                        return 0;
                }
        }
        g->started = 1;

        if (!read_int(g->up[0], &tag) || !read_int(g->up[0], &len)) {
                gen_close(g);
                return 0;
        }
        if (tag != TAG_STR || len < 0) {
                drain(g->up[0], len > 0 ? (size_t)len : 0);
                gen_close(g);
                return 0;
        }
        if ((size_t)len + 1 > cap) {
                drain(g->up[0], (size_t)len);
                gen_close(g);
                return -1;
        }
        if (!read_all(g->up[0], buf, (size_t)len)) {
                gen_close(g);
                return 0;
        }

        buf[len] = '\0';
        return len;
}

void gen_close(gen_t* g) {
        if (g == NULL || g->closed) {
                return;
        }
        g->closed = 1;

        // 两个方向都关：卡在 read 的看到 EOF，卡在 write 的看到 EPIPE
        close(g->down[1]);
        close(g->up[0]);
        g->down[1] = -1;
        g->up[0]   = -1;
        if (g->pid > 0) {
                while (waitpid(g->pid, NULL, 0) < 0 && errno == EINTR) {
                }
                g->pid = -1;
        }

        gen_unlink(g);
}

void gen_dispose(gen_t** gp) {
        if (gp == NULL || *gp == NULL) {
                return;
        }
        gen_close(*gp);
        free(*gp);
        *gp = NULL;
}

// 解一个 UTF-8 码点，*len 是消耗的字节数
static unsigned int utf8_next(const unsigned char* s, int* len) {
        unsigned int cp;
        int          n;

        if (s[0] < 0x80) {
                *len = 1;
                return s[0];
        }
        if ((s[0] & 0xE0) == 0xC0) {
                cp = s[0] & 0x1F;
                n  = 2;
        } else if ((s[0] & 0xF0) == 0xE0) {
                cp = s[0] & 0x0F;
                n  = 3;
        } else if ((s[0] & 0xF8) == 0xF0) {
                cp = s[0] & 0x07;
                n  = 4;
        } else {
                *len = 1;
                return 0xFFFD;
        }

        for (int i = 1; i < n; i++) {
                if ((s[i] & 0xC0) != 0x80) {
                        *len = i;
                        return 0xFFFD;
                }
                cp = (cp << 6) | (s[i] & 0x3F);
        }

        *len = n;
        return cp;
}

static void str_body(gen_t* g) {
        const unsigned char* s = (const unsigned char*)g->str;

        if (g->mode == GEN_BYTE) {
                for (; *s != 0; s++) {
                        gen_yield(g, *s);
                }
                return;
        }

        // 每个字符直接交出它的 UTF-8 字节，调用者拿到的就是 "世"
        for (int i = 0; s[i] != 0;) {
                int len;

                utf8_next(s + i, &len);
                if (len == 1 && s[i] >= 0x80) {
                        yield_bytes(g, "\xEF\xBF\xBD", 3);  // 坏字节 -> U+FFFD
                } else {
                        yield_bytes(g, (const char*)s + i, (size_t)len);
                }
                i += len;
        }
}

gen_t* gen_open_str(const char* s, gen_mode mode) {
        gen_t* g = gen_new();
        g->str   = (s != NULL) ? s : "";
        g->mode  = (int)mode;

        return gen_start(g, str_body);
}
