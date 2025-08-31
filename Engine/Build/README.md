# Build system

This custom build system is a work in progress.

## TODO

This is the list of all of the functionnality needed in the build system

- Makefile generation (with a proper API to allow for other outputs in the future)
- Targets
  - Editor
  - Debug
  - Shipping
- Thirdparty lib management
  - GLFW building
    - For Windows, MacOS & X11 it is fairly straightforward, just compile the files
    - For Wayland however, there needs to be protocol generation with wayland-scanner
      as well as dependency management (wayland-client, ...)
