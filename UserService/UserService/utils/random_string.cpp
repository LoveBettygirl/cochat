#include "UserService/utils/random_string.h"
#include <random>
#include <algorithm>

namespace UserService {

std::string randomString(std::string::size_type len)
{
    static std::string alpha_numeric("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    std::string result;
    result.reserve(len);
    std::random_device rd;
    std::mt19937 generator(rd());
    std::string source(alpha_numeric);
    std::string::size_type generated = 0;
    while (generated < len) {
        std::shuffle(source.begin(), source.end(), generator);
        std::string::size_type delta = std::min({len - generated, source.length()});
        result.append(source.substr(0, delta));
        generated += delta;
    }
    return result;
}

}