# Checkpoint 0 Writeup

**Name:** Hniii98  
**Time Spent:** ~12 hours  
 
## 代理转发

通过proxychain代理请求，得到对应回复报文：
```C++
hniii98@ysyx:~/Project/CS144/CS144/build$ proxychains ./apps/webget cs144.keithw.org /hello
[proxychains] config file found: /etc/proxychains.conf
[proxychains] preloading /usr/lib/x86_64-linux-gnu/libproxychains.so.4
[proxychains] DLL init: proxychains-ng 4.17
[proxychains] Strict chain  ...  127.0.0.1:7897  ...  cs144.keithw.org:80  ...  OK
HTTP/1.1 200 OK
Date: Thu, 11 Dec 2025 08:58:11 GMT
Server: Apache
Last-Modified: Thu, 13 Dec 2018 15:45:29 GMT
ETag: "e-57ce93446cb64"
Accept-Ranges: bytes
Content-Length: 14
Connection: close
Content-Type: text/plain

Hello, CS144!
```
### 发送时的调用链
`weget`首先解析命令行的输入获得 1. 主机名 2. 服务名。根据这两个变量去调用`getaddrinfo`接口获得包含了`IP地址`和`端口号`的`sockaddr`。接着调用socket.write()接口，内部的实现调用了系统的
`writev`接口将request一次性传入内核的缓冲区中。进入TCP的发送队列。

### 接收时的调用链
接收时，调用系统的`read()`获取TCP接收缓冲区的内容直到socket设置了eof。eof的状态由`read()`调用来管理的，`read()`默认是阻塞socket，当返回值为0时候代表链接已关闭同时设置socket的`eof`为true。如果是返回值小于0，代表错误socket直接抛出异常。

## writer 和 reader 设计
Writer和Reader的实现是通过 继承+显式 downcast 来实现对同一内存对象的两种视图。因为Writer和Reader没有添加额外的成员变量，所以内存布局上完全等价于ByteStream。

在没有虚函数的情况下，能不能调用某个接口只取决于指针的静态类型。而Writer和Reader都通过static_assert保证了对象布局的一直，所以都能正确访问到对应的成员。
  
普通成员函数（不涉及到虚指针）并不会改变对象大小，byte_stream_helper.cc中也通过static_assert确保了这一点。

