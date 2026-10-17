#ifndef GEN_H
#define GEN_H

#include <stddef.h>

typedef struct gen gen_t;
typedef void (*gen_body_fn)(gen_t* g);

gen_t* gen_open(gen_body_fn body);

typedef enum {
        GEN_BYTE,       // GEN_BYTE：每次一个字节（int）
        GEN_CODEPOINT,  // GEN_CODEPOINT：每次一个字符（UTF-8 字符串）
} gen_mode;

gen_t* gen_open_str(const char* s, gen_mode mode);

int gen_yield(gen_t* g, int value);                  // 生成器侧：x = yield value
int gen_yield_str(gen_t* g, const char* s);          // 生成器侧：yield 一个字符串
int gen_next(gen_t* g, int* out);                    // 调用者侧：next(g)
int gen_next_rune(gen_t* g, char* buf, size_t cap);  // 调用者侧：返回字节数，0=结束，-1=buf 太小
int gen_send(gen_t* g, int in, int* out);            // 调用者侧：send(x)

void gen_close(gen_t* g);      // 可选，提前取消
void gen_dispose(gen_t** gp);  // GEN_AUTO 用

#define GEN_AUTO __attribute__((cleanup(gen_dispose)))

#endif  // GEN_H
