# dynalo: dynamic loading of shared libraries

dynalo is a header-only library that provides a cross platform API for:

* Loading and unloading a shared library (`.so` in Linux and `.dll` in Windows)
* Getting a pointer to exported functions in the loaded shared library

## API Summary

Everything is inside the `dynalo` namespace which is defined in the `dynalo/dynalo.hpp` header.

* Load a shared library

    ```cpp
    native::handle open(const std::string& dyn_lib_path);
    ```

* Unload a shared library

    ```cpp
    void close(native::handle lib_handle);
    ```

* Look up a function in the shared library and return a pointer to it

    ```cpp
    template <typename FunctionSignature>
    FunctionSignature* get_function(native::handle lib_handle, const std::string& func_name);
    ```

* Wrapper class (excerpt)

    ```cpp
    class library
    {
    public:
        /// Loads a shared library using dynalo::open
        explicit library(const std::string& dyn_lib_path);

        /// Unloads the shared library using dynalo::close
        ~library();

        /// Returns a pointer to the @p func_name function using dynalo::get_function
        template <typename FunctionSignature>
        FunctionSignature* get_function(const std::string& func_name);
    };
    ```

* Generate a valid shared library file name
    * On Linux: Convert `awesome` to `libawesome.so`
    * On Windows: Convert `awesome` to `awesome.dll`

    ```cpp
    std::string to_native_name(const std::string& lib_name);
    ```

## Example

This is a simple example of a shared library that exports some functions
and a program that dynamically loads the library then calls its exported functions.

See the [`test`](test) folder for details on how to compile.

### Shared Library

* `shared.hpp`

    ```cpp
    // symbol_helper.hpp: helper macros that define the boiler plate for exporting functions
    #define DYNALO_EXPORT_SYMBOLS
    #include <dynalo/symbol_helper.hpp>

    #include <cstdint>

    DYNALO_EXPORT int32_t DYNALO_CALL add_integers(const int32_t a, const int32_t b);
    DYNALO_EXPORT void DYNALO_CALL print_message(const char* message);
    ```

* `shared.cpp`

    ```cpp
    #include "shared.hpp"

    #include <iostream>

    DYNALO_EXPORT int32_t DYNALO_CALL add_integers(const int32_t a, const int32_t b)
    {
        return a + b;
    }

    DYNALO_EXPORT void DYNALO_CALL print_message(const char* str)
    {
        std::cout << "Hello [" << str << "]" << std::endl;
    }
    ```

### Dynamic Loading

* `loader.cpp`

    ```cpp
    #include <dynalo/dynalo.hpp>

    #include <cstdint>
    #include <sstream>

    // usage: loader "path/to/lib/folder"
    int main(int argc, char* argv[])
    {
        dynalo::library lib(std::string(argv[1]) + "/" + dynalo::to_native_name("shared"));

        auto add_integers  = lib.get_function<int32_t(const int32_t, const int32_t)>("add_integers");
        auto print_message = lib.get_function<void(const char*)>("print_message");

        std::ostringstream oss;
        oss << "it works: " << add_integers(1, 2);

        print_message(oss.str().c_str());  // prints: Hello [it works: 3]
    }
    ```

## Dependencies

* C++11 compiler: Mainly for the `using` and `auto` keywords. Open an issue or submit a PR if you need support for C++98)
* Linux: `libdl` (link with `-ldl`)
* Windows: `kernel32.lib`

## Installation

### Using CMake

```sh
# create and cd to the build folder
mkdir build
cd build

# configure
cmake [-DCMAKE_INSTALL_PREFIX="path/to/install/dir"] [-DCMAKE_BUILD_TYPE=(Debug|Release|...)] ..

# install
cmake --build . [--config (Debug|Release|...)] --target install
```

### Manual Installation

dynalo is a header-only library. Therefore, you can simply copy the content of the [`include`](include) folder into your project. (just remember to link with the proper libs as mentioned in "Dependencies")
