# Checkpoint 5 Writeup

**Name:** Hniii98  
**Time Spent:** ~10 hours  


## NetworkInterface 类

### 核心功能

NetworkInterface 是网络协议栈中的**网络接口层**，主要负责：

1. **IP-MAC 地址映射管理**：维护 IP 地址到以太网 MAC 地址的映射缓存
2. **ARP 协议处理**：发送 ARP 请求，接收 ARP 回复，学习网络中的主机映射
3. **数据报转发**：将 IPv4 数据报封装成以太网帧发送，或从以太网帧中解析出数据报
4. **队列管理**：缓存等待映射的数据报，避免频繁发送 ARP 请求

### 关键设计

#### 1. 映射缓存结构

```cpp
std::map<Address, MACEntry> cached_mapping_

struct MACEntry {
    EthernetAddress MAC;      // MAC 地址
    ms expired_at;            // 过期时间（30秒）
};
```

- 使用 `std::map` 存储 IP-MAC 映射，自动排序
- 每个映射有 30 秒过期时间（`MAPPING_ALIVE = 30000ms`）
- 定期清理过期映射，避免缓存膨胀

#### 2. 等待队列设计

```cpp
std::map<Address, std::vector<DgramEntry>> datagrams_waiting_mapping_

struct DgramEntry {
    InternetDatagram dgram;   // 等待发送的数据报
    ms expired_at;            // 过期时间（5秒）
};
```

- 当目的 IP 的 MAC 未知时，数据报进入等待队列
- 每个目的 IP 对应一个队列，避免队头阻塞
- 队列有过期时间（5秒），超时后丢弃，避免无限等待

#### 3. ARP 请求冷却机制

```cpp
std::map<Address, ms> arp_available_at_
```

- 记录每个目的 IP 上次发送 ARP 请求的时间
- 5 秒内（`ARP_SENDING_FROZEN = 5000ms`）不重复发送 ARP 请求
- 防止 ARP 请求风暴，减少网络负载

### 核心方法实现

#### 1. `send_datagram()` - 发送数据报

**逻辑流程**：
1. 检查目的 IP 的 MAC 是否已在缓存中
   - 缓存命中：直接封装成以太网帧发送
   - 缓存未命中：
     - 将数据报加入等待队列
     - 检查 ARP 冷却时间，如果已过则发送 ARP 请求
     - ARP 请求广播到全网（`ETHERNET_BROADCAST`）



#### 2. `recv_frame()` - 接收以太网帧

**逻辑流程**：
1. 过滤非本机帧（目的地址不是本机 MAC 或广播地址）
2. 根据帧类型处理：
   - **IPv4 帧**：解析成数据报，加入接收队列
   - **ARP 帧**：
     - 学习发送方的 IP-MAC 映射（双向学习）
     - 如果有等待队列，立即发送缓存的数据报
     - 如果是 ARP 请求且目标是自己，回复 ARP 响应



#### 3. `tick()` 

**作用**：
- 更新接口运行时间
- 清理过期的映射缓存
- 清理过期的等待队列数据报


