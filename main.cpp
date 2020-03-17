#include <iostream>
#include "thread_pool.hpp"

using namespace std;

int main() {
    tardis::thread_pool tp;
    tp.add_task([](){
        cout << "hello world!" << endl;
    });
    auto r = tp.async([](int a, int b){
        this_thread::sleep_for(chrono::seconds(2));
        return a + b;
    }, 2, 33);
    cout << r.get() << endl;
    
    while (true) {}
    return 0;
}
