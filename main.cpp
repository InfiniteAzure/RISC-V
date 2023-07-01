#include <iostream>
#include <algorithm>
#include "processor.h"
#include "Thomaslo.h"

int main() {
    //freopen("../sample.data", "r", stdin);
    //freopen("../ans.out","w",stdout);
    read_all();
    int Clo = 0;
    int rand[3] = {0, 1, 2};
    Tomasulo t;
    while (true) {
        Clo++;
        std::random_shuffle(rand, rand + 3);
        for (int i = 0; i < 3; i++) {
            if (rand[i] == 0) {
                t.commit();
            }
            if (rand[i] == 1) {
                t.calculate();
            }
            if (rand[i] == 2) {
                t.IFetch();
            }
        }
    }
}
