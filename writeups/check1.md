# Checkpoint 0 Writeup

**Name:** Hniii98  
**Time Spent:** ~16 hours  

## 手动发送

ip_raw需要我们去手动发送一个internet datagram，首先就是构造数据，仿造Week 1 day 2的lecture notes构造出一个模拟的icmp数据包，然后通过`sento()`发送到对应地`Address`。这里我用回环地址来模拟接收方。

```bash

启动ip_raw:
hniii98@ysyx:~/Project/CS144/CS144$ sudo ./build/apps/ip_raw

监听回环口:
hniii98@ysyx:~/Project/CS144/CS144$ sudo tcpdump -n -i lo  'proto 5'
tcpdump: verbose output suppressed, use -v[v]... for full protocol decode
listening on lo, link-type EN10MB (Ethernet), snapshot length 262144 bytes

接着在启动ip_raw的窗口键入消息：Hello world!

监听回环口的窗口监听到对应的消息：
tcpdump: verbose output suppressed, use -v[v]... for full protocol decode
listening on lo, link-type EN10MB (Ethernet), snapshot length 262144 bytes
17:16:28.056438 IP 127.0.0.1 > 127.0.0.1:  ip-proto-5 14
```

## 函数实现


### void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )

函数的语义是接受data和对应的first_index插入ByteStream中，如果数据在Reassembler的接收窗口内但是前面的数据存在gap，就需要缓存。

Reaseembler的接收窗口为 
$$[first\_unassemblered\_index,  first\_unassemblered\_index+availale\_capacity)$$

当完整接受到最后一个substring，Reassembler还要负责关闭ByteStream。


整体的流程如下：
- 1. 确定输入数据和接收窗口的重叠部分，如果没有重叠直接退出。
- 2. 重叠部分前有gap，缓存数据，跳到4.；缓存部分没有gap，写入数据，跳到 3.
- 3. 缓存中可写入的数据应写尽写
- 4. 检查是否需要关闭ByteStraem

### void Reassembler::buffer_it(uint64_t index, std::string data) 

buffer\_it 函数是缓存的唯一入口。缓存队列为了保证简单和易用，其保证了两个不变量 
-   1.缓存中所有的pair所代表的interval不能重叠
-   2.缓存中所有的pair的first index应该始终大于等于first_unassemblered_index()
  

这样设计的原因是：使得缓存拉取函数drain()的时候，只需要简单查找而不需要左右判断切分。
为了保持这一不变量，每当push入新的数据后，写入窗口变化，对应的缓存就需要通过normalize_buffer()进行整理。


buffer\_it函数主要的设计就是：对于缓存中所有的interval，不去按照重叠方式的不同分别处理。而是采用：如果interval和需要缓存
的data存在重叠，把interval多余的前缀和后缀合并到data，然后直接移除这个interval所在的pair。

这样处理后，与data完全重叠的部分被保留在了data中，而多余的部分也通过合并保存到data，同时移除了原有的interval来保持缓存不
会重复。

### void drain ()

由于缓存队列保持了缓存的不变量，所以拉取缓存的时候就很简单了，找到对应需要的起始index，然后按照容量写入。
如果完整写入一个缓存的interval，直接删除这个缓存，否则保留对应剩余的slice重新写入缓存。


### void Reassembler::normalize_buffer()

normalize_buffer用于insert()调用后，可能导致写入窗口的变化，需要清楚已经过期的缓存。

具体思路就是
- 1.遍历所有可能重叠的节点
- 2.如果是完全重叠，直接删除；部分重叠，保留未重叠部分，重新写入缓存。
- 3.遇到部分重叠，退出循环

总的来说，normalize_buffer()就是，保证缓存中的数据不是`过期的`，也是就是大于等于`first_unassembled_index`。这样
方便写入窗口变化后，drain()函数直接拉取就可以。