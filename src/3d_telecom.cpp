#include "eyeLike.h"
#include "obj_file_loader.h"

#include <thread>

int main(int argc, const char **argv) {
    std::thread th1(obj_file_loader::run_main);
    std::thread th2(eye_like::run_main);
    th1.join();
    th2.join();
    return 0;
}
