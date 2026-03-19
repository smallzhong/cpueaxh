#define NOMINMAX

#include "demo/framework.hpp"

#pragma comment(lib, "cpueaxh.lib")

int main() {
    return cpueaxh_test::run_all_tests() ? 0 : 1;
}
