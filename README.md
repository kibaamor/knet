# knet

[![Build status](https://ci.appveyor.com/api/projects/status/blxl6hpmf3vuag6m?svg=true)](https://ci.appveyor.com/project/KibaAmor/knet)
[![Build Status](https://travis-ci.org/KibaAmor/knet.svg?branch=master)](https://travis-ci.org/KibaAmor/knet)

简单易用的C++11网络库

## 特点

* 支持Window和Linux
* 同时支持同步、异步两种使用方式
* 易于扩展，支持网络粘包
* 代码少，无第三方库依赖
* 无锁设计，每个socket都在一个固定的线程处理

## 编译环境

* CMake 3.1及以上
* Visual Studio 2015及以上(Windows)
* Gcc 4.9及以上(Linux)

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

## 示例程序

* 同步方式的echo客户端[examples/client/sync_client.cpp](./examples/client/sync_client.cpp)
* 异步异步的echo客户端[examples/client/async_client.cpp](./examples/client/async_client.cpp)
* 同步方式的echo服务端[examples/server/sync_server.cpp](./examples/server/sync_server.cpp)
* 异步异步的echo服务端[examples/server/async_server.cpp](./examples/server/async_server.cpp)
