#include "eyeLike.h"
#include "obj_file_loader.h"

#include <iostream>
#include <thread>

#include <boost/program_options.hpp>

int main(int argc, const char **argv) {
    try {
        boost::program_options::options_description desc{"Options"};
        desc.add_options()("help,h", "Help screen")(
            "obj",
            boost::program_options::value<std::string>()->default_value(
                "LibertStatue.obj"),
            "Path to the .obj file")(
            "resolution",
            boost::program_options::value<std::vector<int>>()->multitoken(),
            "Resolution of camera (1280 720 in default)")(
            "not-show-image", "do not show images from the camera");

        boost::program_options::variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);
        notify(vm);

        std::string objfile = "LibertStatue.obj";
        std::pair<int, int> resolution = {1280, 720};
        bool enable_image = true;

        if (vm.count("help")) {
            std::cout << desc << '\n';
            return 0;
        }
        if (vm.count("obj")) {
            objfile = vm["obj"].as<std::string>();
        }
        if (vm.count("resolution")) {
            auto v = vm["resolution"].as<std::vector<int>>();
            if (v.size() == 2) {
                resolution = {v[0], v[1]};
            } else {
                std::cout << "You must give two integers to resolution "
                             "option.";
                return 1;
            }
        }
        if (vm.count("not-show-image")) {
            enable_image = false;
        }

        std::thread th1(
            obj_file_loader::run_main, objfile, &eye_like::left_eye_center_x,
            &eye_like::right_eye_center_x, &eye_like::left_eye_center_y,
            &eye_like::right_eye_center_y);
        std::thread th2(eye_like::run_main, resolution, enable_image);
        th1.join();
        th2.join();
        return 0;
    } catch (const boost::program_options::error &ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
