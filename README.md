Find the actual Readme at: https://github.com/endless-sky/endless-sky/

## Changes compared to base (in order of magnitude):

- custom graphics backend(metal/vulkan, see source/risingleaf_shared/ for my stuff)
- overhauled build system(using fetch content instead of vcpkg and no system libs, you might experience longer build times)
- removed `using namespace std;` it always annoyed me
- removed SSE from a vec2 class (Point)
- removed libmad in favor of miniaudio as libmad is very outdated
- removed minizip in favor of single header miniz (broken)
- uses openal-soft by default
- various style changes (.clang-format has not been applied everywhere yet)

## Licensing

Endless Sky is a free, open source game. The [source code](https://github.com/endless-sky/endless-sky/) is available under the GPL v3 license, and all the artwork is either public domain or released under a variety of Creative Commons (and similarly permissive) licenses. (To determine the copyright status of any of the artwork, consult the [copyright file](https://github.com/endless-sky/endless-sky/blob/master/copyright).)
