# Dale

Dale is a build system for C.

## Building

Dale builds using itself. If you don't have an installation of Dale, use your
favourite C compiler to compile everything in src/ and the relevant file from
src/host/ for your platform.

To allow automatically discovering dependencies on systems that use pkg-config,
place the extras/dalereq binary on your PATH.
