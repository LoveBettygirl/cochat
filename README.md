# cochat

基于Linux下C++环境实现的分布式聊天系统，基于[corpc](https://github.com/LoveBettygirl/corpc)框架开发，支持单聊、群聊、添加/删除好友、创建/加入/退出群组的基本功能。

## Feature

  - 系统按业务种类拆分为**4个子服务**：登录/注册服务（`UserService`）、群组服务（`GroupService`）、好友服务（`FriendService`）、消息转发/存储服务（`ChatService`），提升系统**可用性**。
  - 使用一个**长连接服务**（`ProxyService`）作为接入层，管理所有登录的客户端并将请求转发给其他子服务，以完成整个请求过程。
  - 使用MySQL作为系统中所有业务数据的持久化存储，Redis作为系统内热点数据或非持久数据的存储。
  - ~~使用Nginx进行长连接服务的负载均衡，将客户端的连接分散到不同的长连接服务上。（这也许不是个好设计，修改后预期将使用 `corpc` 的负载均衡功能选择一个长连接服务）~~
  - 使用RPC框架（`corpc`）的负载均衡功能选择一个长连接服务，将不同客户端的连接分散到不同的长连接服务上。
  - 实现MySQL/Redis的**数据库连接池**，减少频繁建立数据库连接的开销，提高系统对数据库的访问效率。
  - 使用RocketMQ作为消息队列，将离线消息**存入MySQL**的过程**异步化**，降低数据库压力。
  - 使用MD5对用户登录密码进行**加盐加密存储**，提升登录密码被破解的难度。
  - 自定义客户端和服务端的通信协议，并使用服务端非对称公钥（RSA）加密并传递对称密钥（AES），对称密钥加密通信内容实现客户端和接入层服务的加密通信，提升数据通信的安全性。

## 依赖

- gcc v7.5
- cmake（>=3.0）
- [corpc](https://github.com/LoveBettygirl/corpc)
- MySQL（>=5.7）
- ~~Nginx（>=1.9）~~
- Redis（对应的C++开发库为[hiredis](https://github.com/redis/hiredis)）
- RocketMQ（4.9，需安装C++ SDK）
- [corpc](https://github.com/LoveBettygirl/corpc) 及其所有依赖

## 项目配置

### 配置文件

`UserService`、`FriendService`、`GroupService`、`ChatService`、`ProxyService`子目录下的 `conf` 子目录有yaml格式的配置文件，可自行配置服务的IP和端口，以及要连接的MySQL、Redis、RocketMQ的设置。

### ~~Nginx的配置~~

~~Nginx版本必须是1.9及以后，需要通过源码安装。~~

#### ~~源码安装~~

```
wget -c https://nginx.org/download/nginx-1.12.2.tar.gz
tar –zxvf nginx-1.12.2.tar.gz
cd nginx-1.12.2
./configure --with-stream
make && make install
```

#### ~~文件配置~~

~~在 `/usr/local/nginx/conf/nginx.conf` 中加入以下内容：~~

```
# nginx tcp loadbalance config
stream {
    upstream MyServer {
        # 有几台长连接服务器就加入几个设置，这里是2个
        server 服务器IP1:端口号1 weight=1 max_fails=3 fail_timeout=30s;
        server 服务器IP2:端口号2 weight=1 max_fails=3 fail_timeout=30s;
    }

    server {
        proxy_connect_timeout 1s;
        # proxy_timeout 3s;
        listen 8001;
        proxy_pass MyServer;
        tcp_nodelay on;
    }
}
```

#### ~~启动Nginx~~

```
cd /usr/local/nginx/sbin
./nginx
```

## 项目运行

### 构建并运行子服务（以 `UserService` 为例）

```bash
cd UserService
./clean.sh
./build.sh
cd bin
./UserService ../conf/UserService.yml
```

### 构建并运行长连接服务

```bash
cd ProxyService
./clean.sh
./build.sh
cd bin
./ProxyService ../conf/ProxyService.yml
```

### 构建并运行测试客户端

```bash
cd TestClient
./clean.sh
./build.sh
cd bin
./TestClient -a 127.0.0.1 -p 8001
```

#### 测试客户端命令行选项

- `-p PORT` or `--port=PORT`: 指定要连接的长连接服务的端口号
- `-a IP_ADDRESS` or `--address=IP_ADDRESS`: 指定要连接的长连接服务运行的IP地址
- `-h` or `--help`: 帮助信息
- 若IP地址和端口号有一项未指定，将使用 `corpc` 的服务发现和负载均衡功能选出一个长连接服务的IP地址

## TODO

- 项目可能还存在未知bug，需要修复
- 项目架构的优化
- 业务逻辑的继续完善和优化，客户端的完善，其实业务逻辑的完善依赖于客户端的完善……
