export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
curl "https://raw.githubusercontent.com/valentinvanelslande/ctrlib/master/ctrlib.hpp" -o "include/ctrlib.hpp"
make -j4
