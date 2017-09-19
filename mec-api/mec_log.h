#ifndef MEC_LOG_H
#define MEC_LOG_H

// this will be replaced with a logging method, that can be replaced by a platform
// probably based on 'printf' type formating

// define to null to omit logging
#include <iostream>

#define LOG_0(x) std::cerr << x << std::endl
// #define LOG_0(x) 

#define LOG_1(x) std::cout << x << std::endl
// #define LOG_1(x) 

// #define LOG_2(x) std::cout << x << std::endl
#define LOG_2(x) 

// #define LOG_3(x) std::cout << x << std::endl
#define LOG_3(x) 



#endif //MEC_LOG_H