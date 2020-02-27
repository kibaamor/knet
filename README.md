# knet

[![Linux Build Status](https://img.shields.io/travis/kibaamor/knet?label=Linux%20Build%20Status&style=flat-square)](https://travis-ci.org/KibaAmor/knet)
[![Windows Build Status](https://img.shields.io/appveyor/build/kibaamor/knet?label=Windows%20Build%20Status&style=flat-square)](https://ci.appveyor.com/project/KibaAmor/knet)

A cross platform lock-free and timer-supported(hearbeat etc.) C++11 network library.

一个跨平台的无锁且支持定时器（如：心跳）的C++11网络库。

## 特点

* 支持Window和Linux
* 同时支持同步、异步两种使用方式
* 易于扩展，支持网络粘包
* 代码少，无第三方库依赖
* 无锁设计，每个连接的创建销毁逻辑都在同一个固定的线程中处理
* 支持定时器，如心跳发送与检测

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

### echo服务器和客户端

[example目录](./examples/)下提供了使用knet实现的[echo服务器](./examples/server)和[echo客户端](./examples/client).

#### 通信协议

网络包前4个字节为整个网络包(包头+数据)的大小，紧跟在后面的为网络包数据。

#### echo服务器

echo服务器将接收到的数据原封不动的发回给客户端。
同时还设置了定时器检查是否在指定的时间内收到了客户端的消息，如果未在指定时间收到客户端消息，将关闭与客户端的连接。

服务器提供了两种：
* 同步方式的echo服务端[examples/server/sync_server.cpp](./examples/server/sync_server.cpp)
* 异步异步的echo服务端[examples/server/async_server.cpp](./examples/server/async_server.cpp)

### echo客户端

客户端启动后主动连接服务器（如果连接不上服务器，则会自动重连服务器），连接成功后，会主动给服务器发送网络包。
在收到服务器返回的网络包时，会对数据进行校验，失败时会断开连接。

客户端也提供了两种：

* 同步方式的echo客户端[examples/client/sync_client.cpp](./examples/client/sync_client.cpp)
* 异步异步的echo客户端[examples/client/async_client.cpp](./examples/client/async_client.cpp)
