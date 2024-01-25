#include <iostream>
#include <windows.h>

#include "../lib/controller.hpp"

namespace SMController {
    int main(){
        std::cout << "Hello world!" << std::endl;

        Controller controller{};

        return EXIT_SUCCESS;
    }
}