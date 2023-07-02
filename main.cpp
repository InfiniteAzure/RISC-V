#include <iostream>
#include "processor.h"
#include "Thomaslo.h"

int main() {
    freopen("../sample.data", "r", stdin);
    //freopen("../ans.out","w",stdout);
    read_all();
    int Clo = 0;
    Tomasulo t;
    while (true) {
        Clo++;
        t.IFetch();
        t.calculate();
        t.commit();
    }
}