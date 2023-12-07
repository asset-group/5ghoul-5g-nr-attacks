# CEF-CMake

CMake files for sane usage of [CEF (the Chromium Embedded Framework)](https://bitbucket.org/chromiumembedded/cef-project/overview).

*This project is still a work in progress. Things will change quickly with no regard for backwards compatibility until this note is removed.*

Note that this project is **not** applicable for building CEF itself, but only for using it as a library in another project.

## Rationale

The CEF project provide their own CMake files, but they have several outstanding issues:

* They use old-style CMake (no `target_*` macros).
* They force the use of many potentially unwanted compiler settings (no exceptions, no RTTI, C++11)

Hence projects which don't want to conform to any of this are forced to use their own solutions.

**CEF-CMake** fixes this and provides a `CMakeLists.txt` file to make using CEF easy.

## CEF-CMake features

* Uses modern CMake
* Doesn't force any compiler settings except the minimum required:
	* `/MT` instead of `/MTd` for Visual C when sandbox mode is enabled
* Provides the `cefdll_wrapper` static library target
* Downloads a CEF binary build from [Spotify's CEF automated builds](http://opensource.spotify.com/cefbuilds/index.html)
* Copies CEF binaries and resources next to target executables appropriately

## Usage

* You can have this project as a submodule of yours or somewhere in your directory tree. Doesn't matter.
* In your root `CMakeLists.txt` include `<this_project_dir>/cmake/cef_cmake.cmake`. 
* Add this project's directory as a subdirectory. This defines the static library target `cefdll_wrapper`
* Add `cefdll_wrapper` to the link libraries of your CEF executables  

*Mac-specific instructions to come*

## Example

Another project of mine - [cef-demos](https://github.com/iboB/cef-demos) - includes this one as a submodule and provides some CEF demos, all using CEF-CMake.

## License and copyright

This software is distributed under the MIT Software License.

See accompanying file LICENSE.txt or copy [here](https://opensource.org/licenses/MIT).

Copyright &copy; 2019 [Borislav Stanimirov](http://github.com/iboB)
