
#ifndef LOG_MISC_UTILS_
#define LOG_MISC_UTILS_

#include "libs/termcolor.hpp"

#define LOG1(a) (std::cout << (a) << std::endl)
#define LOG2(a, b) (std::cout << (a) << (b) << std::endl)
#define LOG3(a, b, c) (std::cout << (a) << (b) << (c) << std::endl)
#define LOG4(a, b, c, d) (std::cout << (a) << (b) << (c) << (d) << std::endl)
#define LOG5(a, b, c, d, e) (std::cout << (a) << (b) << (c) << (d) << (e) << std::endl)
#define LOG6(a, b, c, d, e, f) (std::cout << (a) << (b) << (c) << (d) << (e) << (f) << std::endl)

#define LOGC(a) (std::cout << termcolor::cyan << (a) << termcolor::reset << std::endl)
#define LOG2C(a, b) (std::cout << termcolor::cyan << (a) << (b) << termcolor::reset << std::endl)
#define LOG3C(a, b, c) (std::cout << termcolor::cyan << (a) << (b) << (c) << termcolor::reset << std::endl)
#define LOG4C(a, b, c, d) (std::cout << termcolor::cyan << (a) << (b) << (c) << (d) << termcolor::reset << std::endl)
#define LOG5C(a, b, c, d, e) (std::cout << termcolor::cyan << (a) << (b) << (c) << (d) << (e) << termcolor::reset << std::endl)

#define LOGM(a) (std::cout << termcolor::magenta << (a) << termcolor::reset << std::endl)
#define LOG2M(a, b) (std::cout << termcolor::magenta << (a) << (b) << termcolor::reset << std::endl)
#define LOG3M(a, b, c) (std::cout << termcolor::magenta << (a) << (b) << (c) << termcolor::reset << std::endl)
#define LOG4M(a, b, c, d) (std::cout << termcolor::magenta << (a) << (b) << (c) << (d) << termcolor::reset << std::endl)
#define LOG5M(a, b, c, d, e) (std::cout << termcolor::magenta << (a) << (b) << (c) << (d) << (e) << termcolor::reset << std::endl)

#define LOGG(a) (std::cout << termcolor::green << (a) << termcolor::reset << std::endl)
#define LOG2G(a, b) (std::cout << termcolor::green << (a) << (b) << termcolor::reset << std::endl)
#define LOG3G(a, b, c) (std::cout << termcolor::green << (a) << (b) << (c) << termcolor::reset << std::endl)
#define LOG4G(a, b, c, d) (std::cout << termcolor::green << (a) << (b) << (c) << (d) << termcolor::reset << std::endl)
#define LOG5G(a, b, c, d, e) (std::cout << termcolor::green << (a) << (b) << (c) << (d) << (e) << termcolor::reset << std::endl)

#define LOGR(a) (std::cout << termcolor::red << (a) << termcolor::reset << std::endl)
#define LOG2R(a, b) (std::cout << termcolor::red << (a) << (b) << termcolor::reset << std::endl)
#define LOG3R(a, b, c) (std::cout << termcolor::red << (a) << (b) << (c) << termcolor::reset << std::endl)
#define LOG4R(a, b, c, d) (std::cout << termcolor::red << (a) << (b) << (c) << (d) << termcolor::reset << std::endl)
#define LOG5R(a, b, c, d, e) (std::cout << termcolor::red << (a) << (b) << (c) << (d) << (e) << termcolor::reset << std::endl)

#define LOGY(a) (std::cout << termcolor::yellow << (a) << termcolor::reset << std::endl)
#define LOG2Y(a, b) (std::cout << termcolor::yellow << (a) << (b) << termcolor::reset << std::endl)
#define LOG3Y(a, b, c) (std::cout << termcolor::yellow << (a) << (b) << (c) << termcolor::reset << std::endl)
#define LOG4Y(a, b, c, d) (std::cout << termcolor::yellow << (a) << (b) << (c) << (d) << termcolor::reset << std::endl)
#define LOG5Y(a, b, c, d, e) (std::cout << termcolor::yellow << (a) << (b) << (c) << (d) << (e) << termcolor::reset << std::endl)

#endif