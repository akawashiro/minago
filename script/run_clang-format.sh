#! /bin/bash

set -eux -o pipefail

# Run clang-format on all files in the repository.
cd "$(git rev-parse --show-toplevel)"
find . -name '*.h' -o -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.hpp' | xargs clang-format-15 -i
git diff --exit-code
