# DirkEngine

Welcome to the **DirkEngine** project! an opensource game engine to help me and any contributors learn about
the fascinating technologies that go into game development. This project is just a hobby so don't expect LTS
or any kind of production ready system. If in the future this project grows however, I would be happy to turn
this into a real production thing to make games and anything else you would want to do with it.

Any contribution/feedback is greatly appreciated!

## <!-- line break separator -->

> [!IMPORTANT]
> Don't forget to check out the [contribution guidelines](/.github/CONTRIBUTING.md) and the
> [code of conduct](/.github/CODE_OF_CONDUCT.md) of this project.


## Build and run the engine

> [!TIP]
> The project is not fully in engine state so this will change a lot soon!
> See #22 for progress on that.

> [!IMPORTANT]
> I have only tested this project on my own machine which runs [NixOS](https://nixos.org) with
> [this configuration](https://github.com/PiquelChips/dotfiles). If you would like to test and
> adapt it for other **Linux distributions** or even **Windows** (ewww), feel free!

### Prerequisits

1. A `C++` compiler such as [**Clang**](https://clang.llvm.org/), [**gcc**](https://gcc.gnu.org/) or the **MSVC Toolchain**.
2. The [**CMake**](https://cmake.org/) build tool and its dependencies.
3. The [**Vulkan SDK**](https://vulkan.lunarg.com/) that we use for the renderer backend. You can follow [this](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html) tutorial for SDK instalation.

### Building the project

The **DirkEngine** uses the [**CMake**](https://cmake.org/) build system. If you
are using **Visual Studio** or any other `C++` IDE, you should already have
CMake integration, in which case you should look into the usage in your specific
editor.

For everybody else, you can use the CLI version. Either run the CMake build and run targets
directly or use the integrated `Makefile` to run the commands and build the configuration for
you.

## Roadmap

You can checkout the dedicated [**Github Project**](https://github.com/users/PiquelChips/projects/3) to understand the
current direction of development.

## License

This project is licensed under the [**GPLv3**](LICENSE.md).
