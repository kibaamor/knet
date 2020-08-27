# **knet**

[English Version](./README.md)

一个跨平台的无锁且支持定时器（如：心跳）的C++11网络库。

[![Travis CI](https://img.shields.io/travis/kibaamor/knet/master?label=Linux&style=flat-square)](https://travis-ci.org/KibaAmor/knet)
[![Travis CI](https://img.shields.io/travis/kibaamor/knet/master?label=OSX&style=flat-square)](https://travis-ci.org/KibaAmor/knet)
[![AppVeyor](https://img.shields.io/appveyor/build/kibaamor/knet/master?label=Windows&style=flat-square)](https://ci.appveyor.com/project/KibaAmor/knet)
[![Coverity](https://img.shields.io/coverity/scan/20462?label=Coverity&style=flat-square)](https://scan.coverity.com/projects/kibaamor-knet)
[![License](https://img.shields.io/github/license/kibaamor/knet?label=License&style=flat-square)](./LICENSE)
[![Standard](https://img.shields.io/badge/C++-11-blue.svg?style=flat-square)](https://github.com/kibaamor/knet)

## 目录

- [**knet**](#knet)
  - [目录](#目录)
  - [特点](#特点)
  - [编译环境](#编译环境)
  - [如何使用](#如何使用)
    - [编译](#编译)
    - [测试](#测试)
  - [核心概念](#核心概念)
  - [示例程序](#示例程序)
    - [echo服务器和客户端](#echo服务器和客户端)
      - [通信协议](#通信协议)
      - [echo服务器](#echo服务器)
    - [echo客户端](#echo客户端)
    - [代码质量](#代码质量)

--------

## 特点

- 支持Window、Linux、MacOS、FreeBSD和OpenBSD
- 同时支持同步、异步两种使用方式
- 易于扩展，支持网络粘包
- 代码少，无第三方库依赖
- 无锁设计，每个连接的创建销毁逻辑都在同一个固定的线程中处理
- 支持定时器，如心跳发送与检测

## 编译环境

- CMake 3.15 及以上
- [Clang 3.8](http://clang.llvm.org/cxx_status.html)及以上(如果你使用Clang编译)
- Visual Studio 2015及以上(Windows)
- [Gcc 4.9](https://gcc.gnu.org/gcc-5/changes.html#libstdcxx)及以上(Linux)
- [Xcode 9.4](https://stackoverflow.com/questions/28094794/why-does-apple-clang-disallow-c11-thread-local-when-official-clang-supports) 及以上(MacOS)

## 如何使用

Windows和Linux下都可以使用下面的命令

### 编译

```bash
# 进入源码根目录
cd knet

# 生成工程
cmake . -B build

# 编译Relase
cmake --build build --config Release
```

### 测试

```bash
# 进入源码根目录下的build目录
cd build

# 运行测试
ctest -C Release
```

## 核心概念

knet的核心概念是：`socket的生产-消费`。

本质来说：*客户端连接服务器*和*服务器接受客户端的连接*都是创建可以读写socket。而创建socket后，客户端和服务器对socket的操作都是一样的发送消息和接受消息。那么这个过程就可以看作是一个生成socket和消费socket的过程。如下图：

```text
   producer                       consumer
┌───────────┐                  ┌──────────────┐   
│ connector │    ——————————>   │    worker    │            
│           │      socket      │              │   
│ acceptor  │    ——————————>   │ async_worker │       
└───────────┘                  └──────────────┘
```

- [connector](./src/kconnector.cpp) 连接器，在客户端连接服务器时使用
- [acceptor](./src/kacceptor.cpp) 接收器，在服务器想要接受客户端连接时使用
- [worker](./src/kworker.cpp) socket的同步处理逻辑
- [async_worker](./src/kworker.cpp) socket的异步处理逻辑

## 示例程序

### echo服务器和客户端

[example目录](./examples/)下提供了使用knet实现的[echo服务器](./examples/server)和[echo客户端](./examples/client).

#### 通信协议

网络包前4个字节为整个网络包(包头+数据)的大小，紧跟在后面的为网络包数据。

#### echo服务器

echo服务器将接收到的数据原封不动的发回给客户端。
同时还设置了定时器检查是否在指定的时间内收到了客户端的消息，如果未在指定时间收到客户端消息，将关闭与客户端的连接。

服务器提供了两种：

- 同步方式的echo服务端[examples/server/sync_server.cpp](./examples/server/sync_server.cpp)
- 异步方式的echo服务端[examples/server/async_server.cpp](./examples/server/async_server.cpp)

### echo客户端

客户端启动后主动连接服务器（如果连接不上服务器，则会自动重连服务器），连接成功后，会主动给服务器发送`不完整的`网络包。
在收到服务器返回的网络包时，会对数据封包进行校验，失败时会断开连接。

客户端也提供了两种：

- 同步方式的echo客户端[examples/client/sync_client.cpp](./examples/client/sync_client.cpp)
- 异步方式的echo客户端[examples/client/async_client.cpp](./examples/client/async_client.cpp)

### 代码质量

[![代码质量状态](https://codescene.io/projects/7651/status.svg)](https://codescene.io/projects/7651/jobs/latest-successful/results)
