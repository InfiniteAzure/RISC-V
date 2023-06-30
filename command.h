//
// Created by Xue Jiarui on 2023/6/25.
//

#ifndef RISC_V_COMMAND_H
#define RISC_V_COMMAND_H

class command{
public:
    unsigned int value;
    int command_type;
    int op_code;
    int rs1;
    int rs2;
    int rd;
    int shamt;
    int funct3;
    int funct7;
    int immediate;

    command() = default;

    command(unsigned int s) {
        unsigned int deal;
        value = s;
        command_type = get_command_type(s);
        op_code = get_digit(6,0,s);
        rd = get_digit(11,7,s);
        rs1 = get_digit(19,15,s);
        rs2 = get_digit(24,20,s);
        funct3 = get_digit(14,12,s);
        funct7 = get_digit(31,25,s);
        if (command_type == 1) {
            deal = get_digit(31,20,s);
            for (int i = 12;i < 32;++i) {
                deal += get_digit(31,31,s) << i;
            }
        } else if (command_type == 2) {
            deal = get_digit(11,7,s) + get_digit(31,25,s) * (1 << 5);
            for (int i = 12;i < 32;++i) {
                deal += get_digit(31,31,s) << i;
            }
        } else if (command_type == 3) {
            deal = get_digit(11,8,s) * (1 << 1) + get_digit(30,25,s) * (1 << 5)
                    + get_digit(7,7,s) * (1 << 11) + get_digit(31,31,s) * (1 << 12);
            for (int i = 13;i < 32;++i) {
                deal += get_digit(31,31,s) << i;
            }
        } else if (command_type == 4) {
            deal = get_digit(31,12,s) * (1 << 12);
        } else if (command_type == 5) {
            deal = get_digit(30,21,s) * (1 << 1) + get_digit(20,20,s) * (1 << 11)
                    +get_digit(19,12,s) * (1 << 12) + get_digit(31,31,s) * (1 << 20);
            for (int i = 21;i < 32;++i) {
                deal += get_digit(31,31,s) << i;
            }
        }
        immediate = (int) deal;
    }
};

#endif //RISC_V_COMMAND_H
