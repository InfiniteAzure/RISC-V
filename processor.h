//
// Created by Xue Jiarui on 2023/6/21.
//

#ifndef RISC_V_PROCESSOR_H
#define RISC_V_PROCESSOR_H

#include "memory.h"

unsigned int sixteen_to_ten(std::string s) {
    int ans = 0;
    for (int i = 0;i < s.length();++i) {
        if (s[i] <= '9' && s[i] >= '0') {
            ans += (s[i] - '0') * (1 << ((s.length() - 1 - i) * 4));
        } else {
            ans += (s[i] - 'A' + 10) * (1 << ((s.length() - 1 - i) * 4));
        }
    }
    return ans;
}

void read_all() {
    unsigned int pos;
    char a;
    while (std::cin >> a) {
        std::string s;
        std::cin >> s;
        if (a == '@') {
            pos = sixteen_to_ten(s);
        } else {
            std::string ms[4];
            ms[0] += a;
            ms[0] += s;
            std::cin >> ms[1] >> ms[2] >> ms[3];
            memory[pos] = sixteen_to_ten(ms[0]) + (1 << 8) * sixteen_to_ten(ms[1])
                    + (1 << 16) * sixteen_to_ten(ms[2]) + (1 << 24) * sixteen_to_ten(ms[3]);
            pos += 4;
        }
    }
}

unsigned int get_digit(int finish,int start,unsigned int s) {
    unsigned int ans = s;
    ans = ans >> start;
    ans = ans & ((1 << (finish - start + 1)) - 1);
    return ans;
}

int get_command_type(unsigned int s) {
    unsigned int judge = get_digit(6,0,s);
    if (judge == 0b0110011) {
        return 0;
    }
    if (judge == 0b0010011 || judge == 0b0000011 || judge == 0b1100111) {
        return 1;
    }
    if (judge == 0b0100011) {
        return 2;
    }
    if (judge == 0b1100011) {
        return 3;
    }
    if (judge == 0b0010111 || judge == 0b0110111) {
        return 4;
    }
    return 5;
}

#endif //RISC_V_PROCESSOR_H
