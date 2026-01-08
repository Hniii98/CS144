# Checkpoint 6 Writeup

**Name:** Hniii98  
**Time Spent:** ~6 hours  

## Router 类

### 核心功能

Router 实现了一个**多接口 IP 路由器**，核心功能是**根据路由表将 IPv4 数据报转发到正确的下一跳或直连网络**。它是网络层的核心设备，连接多个网络并负责跨网络的数据包路由。

### 关键设计

#### 1. 路由表结构

```cpp
struct RouteEntry {
    std::optional<Address> next_hop;  // 下一跳 IP（空表示直连）
    uint32_t route_prefix;            // 路由前缀
    uint32_t prefix_mask;             // 子网掩码
    size_t interface_num;             // 出接口索引
    
    static uint32_t to_mask( uint8_t prefix_length );
};

std::vector<RouteEntry> routing_table_;
```

- 使用 `std::vector` 存储路由表项
- 每个表项包含：目的网络、子网掩码、下一跳、出接口
- 子网掩码通过 `to_mask()` 函数从前缀长度计算
  - 例如：`/24` → 掩码 `255.255.255.0`（前 24 位为 1）

#### 2. 最长前缀匹配算法

```cpp
RouteEntry* find_longest_prefix_match( uint32_t destination )
{
  RouteEntry* best = nullptr;
  for ( auto& entry : routing_table_ ) {
    if ( ( destination & entry.prefix_mask ) == ( entry.route_prefix & entry.prefix_mask ) ) {
      if ( !best || entry.prefix_mask > best->prefix_mask ) {
        best = &entry;
      }
    }
  }
  return best;
}
```

**算法逻辑**：
- 遍历所有路由表项
- 检查目的 IP 是否匹配路由前缀：`(目的IP & 掩码) == (前缀 & 掩码)`
- 选择子网掩码最长（前缀长度最大）的匹配项
- 如果没有匹配，返回 `nullptr`（丢弃数据报）
```

#### 3. 多接口管理

```cpp
std::vector<std::shared_ptr<NetworkInterface>> interfaces_;
```

- 路由器可以连接多个网络（多个接口）
- 每个接口是一个 `NetworkInterface` 对象
- 接口索引从 0 开始，对应路由表中的 `interface_num`

### 核心方法实现

#### 1. `add_route()` - 添加路由

```cpp
void add_route( uint32_t route_prefix, uint8_t prefix_length, 
                optional<Address> next_hop, size_t interface_num )
```

**功能**：向路由表添加一条路由规则
- 计算子网掩码：`prefix_mask = to_mask(prefix_length)`
- 创建 `RouteEntry` 并加入 `routing_table_`

#### 2. `process_datagram()` - 处理数据报

**处理流程**：
1. **TTL 检查**：TTL <= 1 时丢弃，防止数据报无限循环
2. **路由查找**：调用最长前缀匹配找到出接口和下一跳
3. **TTL 减 1**：每经过一个路由器，TTL 减 1
4. **校验和更新**：TTL 改变后必须重新计算 IP 头部校验和
5. **转发**：
   - **直连**：`next_hop` 为空，目的 IP 就是最终目标
   - **转发**：`next_hop` 有值，将数据报发送到下一跳路由器

#### 3. `route()` - 路由主循环

```cpp
void route()
{
  for ( auto it : interfaces_ ) {
    do_interface_transfer( it );
  }
}
```

**功能**：轮询所有接口，处理接收到的数据报
- 遍历所有网络接口
- 对每个接口调用 `do_interface_transfer()`

#### 4. `do_interface_transfer()` - 接口数据传输

**功能**：处理单个接口接收到的所有数据报

- 获取接口的接收队列 `datagrams_received()`
- 循环处理队列中的每个数据报
- 处理完后立即从队列中弹出（路由器不缓存数据报）

