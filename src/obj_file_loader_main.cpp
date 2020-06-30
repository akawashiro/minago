#include "obj_file_loader.h"

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, true);

    double lx = 0, ly = 0, rx = 0, ry = 0;
    obj_file_loader::run_main("LibertStatue/LibertStatue.obj", &lx, &rx, &ly,
                              &ry);
}
