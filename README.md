# **knet**

[中文版](./README_zh.md)

A cross platform lock-free and timer-supported C++11 network library.

[![Travis CI](https://img.shields.io/travis/kibaamor/knet/master?label=Linux&style=flat-square)](https://www.travis-ci.com/github/KibaAmor/knet)
[![Travis CI](https://img.shields.io/travis/kibaamor/knet/master?label=OSX&style=flat-square)](https://www.travis-ci.com/github/KibaAmor/knet)
[![AppVeyor](https://img.shields.io/appveyor/build/kibaamor/knet/master?label=Windows&style=flat-square)](https://ci.appveyor.com/project/KibaAmor/knet)
[![Coverity](https://img.shields.io/coverity/scan/20462?label=Coverity&style=flat-square)](https://scan.coverity.com/projects/kibaamor-knet)
[![License](https://img.shields.io/github/license/kibaamor/knet?label=License&style=flat-square)](./LICENSE)
[![Standard](https://img.shields.io/badge/C++-11-blue.svg?style=flat-square)](https://github.com/kibaamor/knet)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FKibaAmor%2Fknet.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FKibaAmor%2Fknet?ref=badge_shield)

## Table of Contents

- [**knet**](#knet)
  - [Table of Contents](#table-of-contents)
  - [Highlights](#highlights)
  - [Environment](#environment)
  - [How To Use](#how-to-use)
    - [vcpkg](#vcpkg)
    - [Build from source](#build-from-source)
  - [Core Concept](#core-concept)
  - [Examples](#examples)
    - [Echo Server and Client](#echo-server-and-client)
      - [Protocol](#protocol)
      - [Echo Server](#echo-server)
    - [Echo Client](#echo-client)
    - [Code Quality](#code-quality)
  - [License](#license)

--------

## Highlights

- Support Windows, Linux, MacOS, FreeBSD, OpenBSD
- Support Synchronous and Asynchronous processing connections
- Support network package defragment
- Less code and no third party dependency
- Lockfree, connection work on same fixed thread
- Timer support, such as: hearbeat check etc.

## Environment

- CMake 3.15 or higher
- [Clang 3.8](http://clang.llvm.org/cxx_status.html) or higher (If you build with Clang)
- Visual Studio 2015 or higher(Windows)
- [G++ 5](https://gcc.gnu.org/gcc-5/changes.html#libstdcxx) or higher(Linux)
- [Xcode 9.4](https://stackoverflow.com/questions/28094794/why-does-apple-clang-disallow-c11-thread-local-when-official-clang-supports) or higher(MacOS)

## How To Use

### [vcpkg](https://github.com/microsoft/vcpkg)

```bash
vcpkg install knet
```

### Build from source

```bash
# clone source code
git clone https://github.com/KibaAmor/knet.git # or https://gitee.com/kibaamor/knet.git

# enter source code root directory
cd knet

# generate project
cmake . -B build

# build Release
cmake --build build --config Release

# run tests
(cd build && ctest --output-on-failure)

# install
(cd build && sudo make install)
```

## Core Concept

the core concept of knet is: `produce socket-consume socket`.

In fact, both *connecting to server* and *accepting connection from client* are creating readable and writable socket. After the socket is created, both the client and the server do the same to send and accept messages. This process can then be seen as a process of generating sockets and consuming sockets. Here's the diagram:

```text
   producer                       consumer
┌───────────┐                  ┌──────────────┐
│ connector │    ——————————>   │    worker    │
│           │      socket      │              │
│ acceptor  │    ——————————>   │ async_worker │
└───────────┘                  └──────────────┘
```

- [connector](./src/kconnector.cpp) connect to server
- [acceptor](./src/kacceptor.cpp) accept connection from client
- [worker](./src/kworker.cpp) synchronous processing connections
- [async_worker](./src/kworker.cpp) asynchronous processing connections

## Examples

### Echo Server and Client

[example](./examples/) provides [echo server](./examples/server) and [echo client](./examples/client).

#### Protocol

```txt
┌─────────────────────────────┬──────┐
│ total package size(4 bytes) │ data │
└─────────────────────────────┴──────┘
```

4 bytes package header and data follow.

#### Echo Server

The echo server sends the received data back to the client intact.
A timer is also set to check whether a client message is received within the specified time, and the connection to the client is closed if the client message is not received within the specified time.

The server provides two types:

- Synchronous Echo Server[examples/server/sync_server.cpp](./examples/server/sync_server.cpp)
- Asynchronous Echo Server[examples/server/async_server.cpp](./examples/server/async_server.cpp)

### Echo Client

The client actively connects to the server after starting (and automatically reconnects the server if the connection is not available), and when the connection is successful, it actively sends an `incomplete` network package to the server.
When the network package returned by the server is received, the data envelope is verified and disconnected if it fails.

The client provides two types:

- Synchronous Echo Client[examples/client/sync_client.cpp](./examples/client/sync_client.cpp)
- Asynchronous Echo Client[examples/client/async_client.cpp](./examples/client/async_client.cpp)

### Code Quality

[![Code quality status](https://codescene.io/projects/7651/status.svg)](https://codescene.io/projects/7651/jobs/latest-successful/results)

## License

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FKibaAmor%2Fknet.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FKibaAmor%2Fknet?ref=badge_large)
