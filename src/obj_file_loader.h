#include <string>

#include <glog/logging.h>

namespace obj_file_loader {
int run_main(std::string objfile_path, const double *const left_eye_center_x,
             const double *const right_eye_center_x,
             const double *const left_eye_center_y,
             const double *const right_eye_center_y);
}
