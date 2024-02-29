#!/bin/bash

clang-format -i `find ./  -name *.cc`
clang-format -i `find ./  -name *.cpp`
clang-format -i `find ./  -name *.h`
clang-format -i `find ./  -name *.hh`
# cloc --git `git branch --show-current`
