//
// Created by Xue Jiarui on 2023/6/25.
//

#ifndef RISC_V_THOMASLO_H
#define RISC_V_THOMASLO_H

#include "command.h"

struct reg_fp {
    unsigned int value = 0;
    int rob = -1;
    bool busy = false;
};

struct reg_t {
    command c{};
    int Qj = -1;
    int Qk = -1;
    int Vj = 0;
    int Vk = 0;
    bool use_j = false;
    bool use_k = false;
    int Dest = -1;
    int Rob_pos = -1;
    bool ready = false;
    int pc_pos;
    bool guess;
    int current_state = 0;
    unsigned int res;
    //1: ALU 2:ready

    void init(unsigned int s) {
        command tmp(s);
        c = tmp;
    }
};

class Rob {
public:
    reg_fp r[32];
    reg_t regs[32];
    int head;
    int size;

    Rob() {
        for (int i = 0; i < 32; ++i) {
            regs[i].Rob_pos = i;
        }
        head = 0;
        size = 0;
    }

    void insert(unsigned int s, int pc_pos_) {
        int pos = (head + size) % 32;
        regs[pos].init(s);
        if (regs[pos].c.command_type != 4 && regs[pos].c.command_type != 5) {
            if (r[regs[pos].c.rs1].busy) {
                if (regs[r[regs[pos].c.rs1].rob].ready) {
                    regs[pos].use_j = true;
                    regs[pos].Vj = regs[r[regs[pos].c.rs1].rob].res;
                } else {
                    regs[pos].Qj = r[regs[pos].c.rs1].rob;
                    regs[pos].use_j = true;
                }
            }
        }
        if (regs[pos].c.command_type == 0 || regs[pos].c.command_type == 2 || regs[pos].c.command_type == 3) {
            if (r[regs[pos].c.rs2].busy) {
                if (regs[r[regs[pos].c.rs2].rob].ready) {
                    regs[pos].use_k = true;
                    regs[pos].Vk = regs[r[regs[pos].c.rs2].rob].res;
                } else {
                    regs[pos].Qk = r[regs[pos].c.rs2].rob;
                    regs[pos].use_k = true;
                }
            }
        }
        if (regs[pos].c.command_type != 2 && regs[pos].c.command_type != 3 && regs[pos].c.rd != 0) {
            r[regs[pos].c.rd].busy = true;
            r[regs[pos].c.rd].rob = pos;
            regs[pos].Dest = regs[pos].c.rd;
        }
        size++;
        regs[pos].pc_pos = pc_pos_;
        regs[pos].current_state = 1;
        if (regs[pos].c.command_type == 2 || regs[pos].c.op_code == 0b0000011) {
            regs[pos].ready = true;
        }
    }

    void pop() {
        head = (head + 1) % 32;
        size--;
    }

    int get_tail() {
        if (size == 0) {
            return -1;
        }
        return (head + size - 1) % 32;
    }

    void clear() {
        head = 0;
        size = 0;
        for (int i = 0;i < 32;++i) {
            r[i].busy = false;
        }
    }
};

class Tomasulo {
public:
    Rob reorder;
    int statistics = 0;
    int writing = 0;
    bool waiting = false;

    void IFetch() {
        if (!waiting) {
            if (reorder.size < 32) {
                reorder.insert(memory[pc], pc);
                command com(memory[pc]);
                if (com.command_type == 5) {
                    pc += com.immediate;
                } else if (com.op_code == 0b1100111) {
                    waiting = true;
                } else if (com.command_type == 3) {
                    if (statistics >= 2) {
                        int s = reorder.get_tail();
                        reorder.regs[s].guess = true;
                        pc = pc + com.immediate;
                    } else {
                        int s = reorder.get_tail();
                        reorder.regs[s].guess = false;
                        pc = pc + 4;
                    }
                } else {
                    pc = pc + 4;
                }
            }
        }
    }

    void broadcast(int pos, unsigned int value) {
        for (int i = 0; i < 32; ++i) {
            if (reorder.regs[i].current_state == 0) {
                if (reorder.regs[i].Qj == i && reorder.regs[i].use_j) {
                    reorder.regs[i].Qj = -1;
                    reorder.regs[i].Vj = value;
                }
                if (reorder.regs[i].Qk == i && reorder.regs[i].use_j) {
                    reorder.regs[i].Qk = -1;
                    reorder.regs[i].Vk = value;
                }
            }
        }
        reorder.regs[pos].ready = true;
        reorder.regs[pos].current_state = 2;
    }

    void calculate() {
        for (int i = reorder.head; i < reorder.head + reorder.size; ++i) {
            int pos = i % 32;
            if (reorder.regs[pos].current_state == 1) {
                if (reorder.regs[pos].c.value == 0x0ff00513) {
                    reorder.regs[pos].current_state = 2;
                    reorder.regs[pos].ready = true;
                    return;
                }
                if (reorder.regs[pos].c.command_type == 0) {
                    R_execute(reorder.regs[pos].c, pos);
                } else if (reorder.regs[pos].c.command_type == 1) {
                    I_execute(reorder.regs[pos].c, pos);
                } else if (reorder.regs[pos].c.command_type == 3) {
                    B_execute(reorder.regs[pos].c, pos);
                } else if (reorder.regs[pos].c.command_type == 4) {
                    U_execute(reorder.regs[pos].c, pos);
                } else if (reorder.regs[pos].c.command_type == 5) {
                    J_execute(reorder.regs[pos].c, pos);
                }
                reorder.regs[pos].current_state = 2;
            }
            broadcast(pos, reorder.regs[pos].res);
        }
    }

    void U_execute(command c, int pos) {
        if (c.op_code == 0b0110111) {
            reorder.regs[pos].res = c.immediate;
        } else if (c.op_code == 0b0010111) {
            reorder.regs[pos].res = reorder.regs[pos].pc_pos + c.immediate;
        }
    }

    void J_execute(command c, int pos) {
        reorder.regs[pos].res = reorder.regs[pos].pc_pos + 4;
    }

    void B_execute(command c, int pos) {
        if (c.funct3 == 0b000 || c.funct3 == 0b001 || c.funct3 == 0b100 || c.funct3 == 0b101) {
            int a, b;
            if (reorder.regs[pos].use_j) {
                a = reorder.regs[pos].Vj;
            } else {
                a = reorder.r[c.rs1].value;
            }
            if (reorder.regs[pos].use_k) {
                b = reorder.regs[pos].Vk;
            } else {
                b = reorder.r[c.rs2].value;
            }
            if (c.funct3 == 0b000) {
                reorder.regs[pos].res = (a == b);
            } else if (c.funct3 == 0b001) {
                reorder.regs[pos].res = (a != b);
            } else if (c.funct3 == 0b100) {
                reorder.regs[pos].res = (a < b);
            } else if (c.funct3 == 0b101) {
                reorder.regs[pos].res = (a >= b);
            }
        } else {
            unsigned int a, b;
            if (reorder.regs[pos].use_j) {
                a = reorder.regs[pos].Vj;
            } else {
                a = reorder.r[c.rs1].value;
            }
            if (reorder.regs[pos].use_k) {
                b = reorder.regs[pos].Vk;
            } else {
                b = reorder.r[c.rs2].value;
            }
            if (c.funct3 == 0b110) {
                reorder.regs[pos].res = (a < b);
            } else if (c.funct3 == 0b111) {
                reorder.regs[pos].res = (a >= b);
            }
        }
    }

    void R_execute(command c, int pos) {
        int a, b;
        unsigned int ua, ub;
        if (reorder.regs[pos].use_j) {
            a = reorder.regs[pos].Vj;
            ua = reorder.regs[pos].Vj;
        } else {
            a = reorder.r[c.rs1].value;
            ua = reorder.r[c.rs1].value;
        }
        if (reorder.regs[pos].use_k) {
            b = reorder.regs[pos].Vk;
            ub = reorder.regs[pos].Vk;
        } else {
            b = reorder.r[c.rs2].value;
            ub = reorder.r[c.rs2].value;
        }
        if (c.funct3 == 0b000) {
            if (c.funct7 == 0b0000000) {
                reorder.regs[pos].res = a + b;
            } else {
                reorder.regs[pos].res = a - b;
            }
        } else if (c.funct3 == 0b001) {
            int left = ub & ((1 << 5) - 1);
            reorder.regs[pos].res = ua << left;
        } else if (c.funct3 == 0b010) {
            reorder.regs[pos].res = (a < b);
        } else if (c.funct3 == 0b011) {
            reorder.regs[pos].res = (ua < ub);
        } else if (c.funct3 == 0b100) {
            reorder.regs[pos].res = ua ^ ub;
        } else if (c.funct3 == 0b101) {
            if (c.funct7 == 0b0000000) {
                int right = ub & ((1 << 5) - 1);
                reorder.regs[pos].res = ua >> right;
            } else {
                int right = ub & ((1 << 5) - 1);
                reorder.regs[pos].res = a >> right;
            }
        } else if (c.funct3 == 0b110) {
            reorder.regs[pos].res = ua | ub;
        } else if (c.funct3 == 0b111) {
            reorder.regs[pos].res = ua & ub;
        }
    }

    void I_execute(command c, int pos) {
        int a;
        unsigned int ua;
        if (reorder.regs[pos].use_j) {
            a = reorder.regs[pos].Vj;
            ua = reorder.regs[pos].Vj;
        } else {
            a = reorder.r[c.rs1].value;
            ua = reorder.r[c.rs1].value;
        }
        if (c.op_code == 0b1100111) {
            reorder.regs[pos].res = reorder.regs[pos].pc_pos + 4;
            pc = a + reorder.regs[pos].c.immediate;
            pc = pc - pc % 2;
            waiting = false;
        } else if (c.op_code == 0b0010011) {
            if (c.funct3 == 0b000) {
                reorder.regs[pos].res = a + c.immediate;
            } else if (c.funct3 == 0b010) {
                reorder.regs[pos].res = (a < c.immediate);
            } else if (c.funct3 == 0b011) {
                reorder.regs[pos].res = (ua < (unsigned int) c.immediate);
            } else if (c.funct3 == 0b100) {
                reorder.regs[pos].res = (unsigned int) (a ^ c.immediate);
            } else if (c.funct3 == 0b110) {
                reorder.regs[pos].res = (unsigned int) (a | c.immediate);
            } else if (c.funct3 == 0b111) {
                reorder.regs[pos].res = (unsigned int) (a & c.immediate);
            } else if (c.funct3 == 0b001) {
                reorder.regs[pos].res = (ua << c.rs2);
            } else if (c.funct3 == 0b101) {
                if (c.funct7 == 0b0000000) {
                    reorder.regs[pos].res = (ua >> c.rs2);
                } else {
                    reorder.regs[pos].res = (a >> c.rs2);
                }
            }
        }
    }

    void memory_execute(command c, int pos) {
        if (c.command_type == 2) {
            int p = reorder.r[c.rs1].value + c.immediate;
            if (c.funct3 == 0b000) {
                unsigned int value = reorder.r[c.rs2].value & (1 << 8) - 1;
                memory[p] = value;
            } else if (c.funct3 == 0b001) {
                unsigned int value = reorder.r[c.rs2].value & (1 << 16) - 1;
                memory[p] = value;
            } else if (c.funct3 == 0b010) {
                unsigned int value = reorder.r[c.rs2].value;
                memory[p] = value;
            }
        } else if (c.command_type == 1) {
            unsigned int p = reorder.r[c.rs1].value + c.immediate;
            unsigned int value = memory[p];
            if (c.funct3 == 0b000) {
                for (int i = 8; i < 32; ++i) {
                    value += (1 << i) * get_digit(7, 7, value);
                }
            } else if (c.funct3 == 0b001) {
                for (int i = 16; i < 32; ++i) {
                    value += (1 << i) * get_digit(7, 7, value);
                }
            } else if (c.funct3 == 0b100) {
                value = value & ((1 << 8) - 1);
            } else if (c.funct3 == 0b101) {
                value = value & ((1 << 16) - 1);
            }
            reorder.r[c.rd].value = value;
        }
    }

    void commit() {
        if (reorder.size != 0) {
            int pos = reorder.head;
            if (writing == 2) {
                writing++;
                return;
            } else if (writing == 3) {
                memory_execute(reorder.regs[pos].c, pos);
                writing = 0;
                if (reorder.r[reorder.regs[pos].c.rd].busy && reorder.r[reorder.regs[pos].c.rd].rob == pos) {
                    reorder.r[reorder.regs[pos].c.rd].busy = false;
                }
                reorder.pop();
                return;
            }
            if (reorder.regs[pos].ready) {
                if (reorder.regs[pos].c.command_type == 2 || (reorder.regs[pos].c.command_type == 1 &&
                                                              reorder.regs[pos].c.op_code == 0b0000011)) {
                    writing = 2;
                } else if (reorder.regs[pos].c.command_type != 3) {
                    if (reorder.regs[pos].c.value == 0x0ff00513) {
                        std::cout << (reorder.r[10].value & 255);
                        exit(0);
                    }
                    if (reorder.r[reorder.regs[pos].c.rd].busy && reorder.r[reorder.regs[pos].c.rd].rob == pos) {
                        reorder.r[reorder.regs[pos].c.rd].busy = false;
                    }
                    if (reorder.regs[pos].c.rd != 0) {
                        reorder.r[reorder.regs[pos].c.rd].value = reorder.regs[pos].res;
                    }
                    reorder.pop();
                    return;
                } else {
                    if (reorder.regs[pos].res && reorder.regs[pos].guess) {
                        if (statistics != 3) {
                            statistics++;
                        }
                        reorder.pop();
                    } else if ((!reorder.regs[pos].res) && (!reorder.regs[pos].guess)) {
                        if (statistics != 0) {
                            statistics--;
                        }
                        reorder.pop();
                    }  else if (reorder.regs[pos].res && (!reorder.regs[pos].guess)) {
                        pc = reorder.regs[pos].pc_pos + reorder.regs[pos].c.immediate;
                        reorder.clear();
                        if (statistics != 3) {
                            statistics++;
                        }
                    } else {
                        pc = reorder.regs[pos].pc_pos + 4;
                        reorder.clear();
                        if (statistics != 0) {
                            statistics--;
                        }
                    }
                    return;
                }
            }
        }
    }

    Tomasulo() = default;
};

#endif //RISC_V_THOMASLO_H
