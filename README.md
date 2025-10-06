# DirkEngine

Welcome to the **DirkEngine** project! an opensource game engine to help me and any contributors learn about
the fascinating technologies that go into game development. This project is just a hobby so don't expect LTS
or any kind of production ready system (this could change in the future if this repository gets enough traction).
Any contribution/feedback is greatly appreciated!

## <!-- line separator -->

> [!IMPORTANT]
> Don't forget to check out the [contribution guidelines](/.github/CONTRIBUTING.md) and the
> [code of conduct](/.github/CODE_OF_CONDUCT.md) of this project.

## Build

> [!IMPORTANT]
> The **DirkEngine** uses a custom build system. This build system is only available on Linux.
> Feel free to add compatibility with another platform.

### Prerequisits

1. The [**gcc**](https://gcc.gnu.org/) compiler toolchain.
2. The [**Go**](https://go.dev/) compiler.
3. GUN [**Make**](https://www.gnu.org/software/make/).
4. The [**Vulkan SDK**](https://vulkan.lunarg.com/) that we use for the renderer backend. You can follow [this](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html) tutorial for SDK instalation.
   The SDK should be in the `VULKAN_SDK` environmment variable.

### Building the project

Run `make build` to build or `make` to build and run.
Binaries will be located in the `Binaries` directory.

## Roadmap

You can checkout the dedicated [**Github Project**](https://github.com/users/PiquelChips/projects/3) to understand the
current direction of development.

## Contact

If you would like to contact me privately for reasons related to the **DirkEngine** (such as any security issues), send me an email at [**dirkengine@piquel.fr**](mailto:dirkengine@piquel.fr).

## License

This project is licensed under the [**GPLv3**](LICENSE) license.
