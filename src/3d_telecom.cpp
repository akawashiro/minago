#include "eyeLike.h"
#include "obj_file_loader.h"

#include <thread>

int main(int argc, const char **argv) {
    std::thread th(obj_file_loader::run_main);
    int r = eye_like::run_main();
    th.join();
    return r;
}
