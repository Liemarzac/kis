//
//  main.cpp
//  kisTest
//
//  Created by Sylvain Gibouret on 20/01/2024.
//

#include <iostream>

#include "UnitCpp/UnitCpp.h"

int main(int argc, const char * argv[]) {
    return UnitCpp::TestRegister::test_register().run_tests();
}

