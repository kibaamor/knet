# knet

简单易用的C++11网络库

## 亮点

* 支持Window和Linux
* 同时支持同步、异步两种使用方式
* 易于扩展，支持网络粘包
* 代码少，无库依赖

## 如何编译

```bash
cd knet
mkdir build
cd build
cmake ..
make -j6
```

## 示例程序

* 同步方式的echo客户端[examples/client/sync_client.cpp](./examples/client/sync_client.cpp)
* 异步异步的echo客户端[examples/client/async_client.cpp](./examples/client/async_client.cpp)
* 同步方式的echo服务端[examples/server/sync_server.cpp](./examples/server/sync_server.cpp)
* 异步异步的echo服务端[examples/server/async_server.cpp](./examples/server/async_server.cpp)
