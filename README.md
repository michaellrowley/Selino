# Selino
<p align="center">
<img src="./logo.png" width=150>
</p>


<span align="center">

[![Lua - Documentation](https://img.shields.io/badge/Lua_API-Documentation-blue?style=for-the-badge)](https://proxy-application.gitbook.io/ "Go to project documentation")

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/6f240593371b40e6a254af93da5816be)](https://app.codacy.com/gh/michaellrowley/Selino/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

</span>

Selino is a cross-platform, **scriptable** proxy with support for ``CONNECT``-type connections on SOCKS4(/a) and SOCKS5(/h).

**Current Version: 0.1.1**

## Usage
Running this application is as simple as launching its binary followed by some (*mostly* optional) standard command-line arguments;
- ``-s`` / ``--script``: A script to receive app-specific callbacks throughout the application's execution.
- ``-p`` / ``--port``: The port that the application should listen on for incoming connections.
- ``-t`` / ``--timeout``: The application's inactivity timeout during two-way 'transparent' forwarding. This can be specified as ``60,000`` (defaults to milliseconds), or with a suffix such as ``s``/``ms``/``m`` or ``seconds``/``milliseconds``/``minutes``.
- ``-elc`` / ``--enable-local-connections``: A flag (meaning that no additional value is required) to permit the proxy to deliver requests from clients to internal (private or localhost) addresses without any filtering.
> selino -s log_data.lua -p 8080 -p 1080 -p  -t 30ms

Lua scripts play a major role in this project's structure, allowing users to modularly expand the proxy's features.

## Example
> $ selino -s [testing/logging.lua](./testing/logging.lua) -p 8080 -p 1080 -t 30ms

Separate terminal:
> $ curl -x socks5://localhost:8080 http://info.cern.ch

<details>
<summary>
Output from original terminal
</summary>

```bash
Initialization function called in logging.lua
Received a connection on local '127.0.0.1:8080' from '127.0.0.1:62173'
Packet received:
	Protocol: 'SOCKS5'
	Packet Index: '0'
	Local Endpoint: '127.0.0.1:8080'
	Remote Endpoint: '127.0.0.1'
	Remote Port: '62173'
	Raw Packet: '\x05\x02\x00\x01'
Packet received:
	Protocol: 'SOCKS5'
	Packet Index: '1'
	Local Endpoint: '127.0.0.1:8080'
	Remote Endpoint: '127.0.0.1'
	Remote Port: '62173'
	Raw Packet: '\x05\x01\x00\x01\xbc\xb8\x15\x6c\x00\x50\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
Received a SOCKS5 request:
	Incoming Port: 'nil'
	Incoming Endpoint: '127.0.0.1:62173'
	Command: 'CONNECT'
	Destination: '188.184.21.108:80'
Forwarding '76' bytes ('127.0.0.1:62173' ==> '188.184.21.108:80':
	Raw Data: '\x47\x45\x54\x20\x2f\x20 ... \x2a\x0d\x0a\x0d\x0a')
Forwarding '878' bytes ('188.184.21.108:80' ==> '127.0.0.1:62173':
	Raw Data: '\x48\x54\x54\x50\x2f\x31 ... \x74\x6d\x6c\x3e\x0a')
Tunnel ('127.0.0.1:62173' <==> '188.184.21.108:80') has been closed
Terminated a connection on local '127.0.0.1:8080' from '127.0.0.1:62173'
```
</details>

## Compilation
Compiling this project requires inclusion of some external libraries:
- [Sol2](https://github.com/ThePhD/sol2)(v3) for implementation of Lua scripting
- [Lua](https://www.lua.org/download.html) (tested using ``5.4``) for Sol to use as a backend for the language
- [Boost](https://www.boost.org/) for cross-platform networking

The code itself is written in C++20 and therefore attempts to use anything below that version will require some source-level adjustments in order to compile.

This project has successfully been compiled on the latest versions of GCC, Clang, and MSVC. There is a CMake file included in the ``src/`` directory to aid in generating a makefile for building the code (you may need to define ``SOL_PATH`` and/or ``BOOST_INCLUDE_DIR`` for CMake to run smoothly).
<details>
	<summary>Compilation using CMake example:</summary>
This example assumes a file hierarchy as follows:

```
    |- sol2
    |----- sol
    |---------- config.hpp
    |---------- sol.hpp
    |---------- forward.hpp
    |- boost_1_8x
    |----- include
    |---------- boost
    |--------------- asio.hpp
    |--------------- ...
    |- selino
    |----- src
    |---------- CMakeLists.txt
    |---------- ...
```

```sh
git clone https://github.com/michaellrowley/Selino/
cd Selino
cmake src -DSOL_PATH="../../sol2/" -DBOOST_INCLUDE_DIR="../../boost_1_8x/include/"
cmake src -DSOL_PATH="../../sol2/" -DBOOST_INCLUDE_DIR="../../boost_1_8x/include/"
make
```
You'll need to replace the include paths with your own, and on some platforms there will be no need to specify the boost library include path at all (as long as the ``libboost-dev`` package is installed).

***Note:*** We run cmake twice to remediate a slight Lua include bug within the CMake file, where the first run fails to create proper linking commands and the second - presumably thanks to the CMake cache file - is able to satisfy that requirement.

</details>
