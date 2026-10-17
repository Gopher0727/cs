#include <stdio.h>

#include "gen.h"

static const char TEXT[] = "héllo 世界";

int main(void) {
        GEN_AUTO gen_t* g1 = gen_open_str(TEXT, GEN_BYTE);
        GEN_AUTO gen_t* g2 = gen_open_str(TEXT, GEN_CODEPOINT);

        int ch;
        while (gen_next(g1, &ch)) {
                printf("%02x ", ch);
        }
        printf("\n");

        char buf[8];
        while (gen_next_rune(g2, buf, sizeof(buf)) > 0) {
                printf("%s ", buf);
        }
        printf("\n");

        return 0;
}
