#include <iostream>

#include "thread_pool.hpp"

using namespace std;

int main() {
    tardis::thread_pool tp;
    tp.add_task([](){
        cout << "hello world!" << endl;
    });
    while (true) {}
    return 0;
}
