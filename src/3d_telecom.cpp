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
            boost::program_options::value<std::pair<int, int>>()->default_value(
                {1280, 720}),
            "Resolution of camera (1280 720 in default)")(
            "not-show-image", "do not show images from the camera");

        boost::program_options::variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);
        notify(vm);

        std::string objfile = "LibertStatue.obj";
        std::pair<int, int> resolution = {1280, 720};

        if (vm.count("help")) {
            std::cout << desc << '\n';
            return 0;
        }
        if (vm.count("obj")) {
            objfile = vm["obj"].as<std::string>();
        }
        if (vm.count("resolution")) {
            resolution = vm["resolution"].as<std::pair<int, int>>();
        }

        std::thread th1(obj_file_loader::run_main(objfile));
        std::thread th2(eye_like::run_main(resolution));
        th1.join();
        th2.join();
        return 0;
    } catch (const boost::program_options::error &ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
