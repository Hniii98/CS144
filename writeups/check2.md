# Checkpoint 2 Writeup

**Name:** Hniii98  
**Time Spent:** ~6 hours  

---

## Wrap32 类：序列号的环形映射

`Wrap32` 封装了 TCP 中相对序列号 (`seqno`) 与绝对序列号 (`absolute_seqno`) 的双向映射逻辑。  
其核心在于正确处理 **32 位回绕（wrap-around）** 的数学关系。

---

###  核心设计思想

- **`wrap()`**  
  将绝对序列号折叠进 32 位空间。  
  由于 C++ 的无符号运算本身是按 `mod 2³²` 进行的，直接相加即可实现回绕：
  ```cpp
  Wrap32 Wrap32::wrap(uint64_t n, Wrap32 zero_point) {
      return zero_point + static_cast<uint32_t>(n);
  }
  ```
  因此 `(a + b)` 等价于 `(a + b) mod 2³²`，无需显式取模。

- **`unwrap()`**  
  实现反向映射，将环形空间中的 `seqno` 恢复为**最接近参考点 (`checkpoint`) 的绝对序列号**。  
  由于 `wrap` 是周期函数，一个 `seqno` 可以对应多个候选 `absolute_seqno`（相差若干个 2³² 周期）。  
  因此需要依据 `checkpoint` 来判断“当前最合理的原值”。

  `checkpoint`可能和这个求解的绝对序号在同一个2³²周期中，也可能是在上一个周期，或者下一个周期。所以在反向映射的时候，
  需要对3个可能的结果都进行检查。

  **同时，需要保证这个`checkpoint`和待求解的序号的相对距离小于2^31。因为。**
  - 3个可能值之间的距离是2^32
  - 如果保证了`checkpoint`和序号之间距离小于2^31，那么相离最近的那个一定就是正确值
  - 而TCP协议设计的时候，窗口的大小最大就是2^30（加上缩放因子），因此便于确定一个有效的`checkpoint`

---

###  算法要点

1. **计算偏移量**
   ```cpp
   uint64_t offset = raw_value_ - zero_point.raw_value_;
   ```
   - 即使 `raw_value_ < zero_point`，无符号减法依然合法。  
   - 这是因为 C++ 中无符号算术天然执行 `(a - b) mod 2³²`，完美契合 TCP 序列号的回绕语义。(因为模运算中负数的结果为取同余类中的第一个非负代表元，数值上就是加上一个2^32。)

2. **确定粗略基线**
   ```cpp
   uint64_t rough_base = checkpoint & ~0xFFFFFFFFULL;
   ```
   - 通过屏蔽低 32 位确定 `checkpoint` 所在的区间。

3. **生成候选值**
   ```cpp
   uint64_t candidate = rough_base + offset;
   ```
   - 得到当前区间内的候选绝对序列号。

4. **比较相邻区间**
   ```cpp
   for (int shift : {-1, 1}) {
       uint64_t alter = candidate + (shift * (1ULL << 32));
       ...
   }
   ```
   - 检查前后两个相邻区间的候选值（±2³²），  
     选择与 `checkpoint` 差值最小的那个作为最终结果。

---

### 准确定义

> `unwrap()` 的目标是**在模 2³² 的多重映射中，恢复出与当前流上下文（由 checkpoint 决定）最一致的绝对序列号**。  
> 换句话说，它确实是在还原原值，但这种“还原”依赖于 checkpoint 提供的上下文约束。  
> 若没有 checkpoint，单凭 `seqno` 是无法唯一确定原值的。

---

## TCPReceiver：数据接收与状态维护

`TCPReceiver` 模块负责：
- 处理 TCP 报文的接收；
- 通过 `Reassembler` 拼接成连续的数据流；
- 返回接收窗口与 ACK 信息。

---

###  接收流程逻辑

1. **RST 标志处理**
   ```cpp
   if (message.RST) reassembler_.set_error();
   ```
   - 表示连接被异常终止。
   - 不直接调用底层 `ByteStream::set_error()`，而是通过 `Reassembler` 封装传播错误，保持模块边界清晰。

2. **ISN 初始化**
   ```cpp
   if (message.SYN && !isn_.has_value()) isn_ = message.seqno;
   if (!isn_.has_value()) return;
   ```
   - 首次接收 SYN 时记录初始序列号；
   - 若尚未初始化 ISN 就收到其他包，一律丢弃。

3. **序列号转换与插入**
   ```cpp
   uint64_t abs_seq = message.seqno.unwrap(isn_.value(), reassembler_.first_unassembled_index());
   if (message.SYN) abs_seq += 1;
   reassembler_.insert(abs_seq - 1, message.payload, message.FIN);
   ```
   - 使用 `unwrap()` 将 `seqno` 转换为绝对序列号；
   - 跳过 SYN 占用的虚拟字节；
   - 插入到重组器中等待拼接。

4. **发送反馈报文**
   ```cpp
   TCPReceiverMessage TCPReceiver::send() const {
       ...
       message.RST = writer().has_error();
       message.window_size = std::min<uint64_t>(writer().available_capacity(), UINT16_MAX);
       ...
   }
   ```
   - `ackno` 表示下一个期望的字节；
   - 若流已关闭（FIN 已收到），则再加 1；
   - `window_size` 表示当前接收缓冲区剩余空间（截断至 16 位）。



