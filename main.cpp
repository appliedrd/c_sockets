#include <iostream>

extern "C" {
#include "time_server.h"
};
int main() {
    std::cout << "Hello, World!" << std::endl;
    get_time();
    return 0;
}
