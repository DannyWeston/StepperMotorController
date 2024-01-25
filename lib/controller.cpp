#include <iostream>

#include "controller.hpp"

namespace SMController {
    // Constructor
    Controller::Controller(){
        std::cout << "Making object" << std::endl;
    }

    // Destructor
    Controller::~Controller(){
        std::cout << "Destroying object" << std::endl;
        // Do something
    }   
}