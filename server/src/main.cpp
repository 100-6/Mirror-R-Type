#include <iostream>
#include "Test.hpp"

int main(void)
{
    std::cout << "Server starting" << std::endl;
    engine::test t;
    t.hello();
    return 0;
}