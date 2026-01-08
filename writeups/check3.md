# Checkpoint 3 Writeup

**Name:** Hniii98  
**Time Spent:** ~24 hours  

---

## Timer 类：计时器

`Timer` 封装了一个用定时 `alarm` 的计时器。
将定时器的关闭和开启封装成接口对外暴露，同时向外提供检查定时器是否`expired`和是否`running`的接口。

---

###  核心设计思想

- **`set_alarm( uint64_t time_alarm_at_ms )`**  
  接收一个来自用户的输入的数值，代表计时器`alarm`的时刻，单位为ms。

- **`bool is_timer_expired( uint64_t eclapsed_time )`**  
  接收一个来自用户的输入的数值，TCPSender记录的流逝的时间，这是一个相对时间，单位为ms，接口返回当前计时器是否到时。
---

## TCPSener：数据发送与状态维护

`TCPReceiver` 模块负责：
- 处理 TCP 报文的发送；
- 通过`void tick( uint64 t ms since last tick, const TransmitFunction& transmit );`接口维护一个相对时间，并根据情况重发缓存（但是不清理）
- `push( const TransmitFunction& transmit )` 负责发送数据并维护一个缓存
- `receive( const TCPReceiverMessage& msg )` 负责接受Sender的确认号来清理缓存
- `construct_message( string_view peek_of_input )` 接收当前输入流视图来构建需要发送的数据
  
- 维护已发送序列长度和SYN/FIN标志的发送情况

---

###  发送流程逻辑

1. **数据的构建**
	- 根据已发送长度构建`seqno`, 如果输入流`has_error`，segment不考虑标志和payload
	- windows_size去除掉缓冲中的字节数后即为当前segment的容量，在容量内尽可能放多的数据
  
2. **数据的发送**
	- 数据从输入流`input_`到发送端的逻辑只存在于`push( const TransmitFunction& transmit )`
	- 任何有效长度为1的segment都需要放入缓存等待超时后重发，并更新对应状态包括开启计时器
	- 输入流如果`has_error`的话发送一个只带有RST标志的segment，无其他有效数据，不更新任何状态

3. **时间的维护**
	- Sender的时间完全由`tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )`接口来被动维护
  
4. **数据重发**
	- 每当tick接收到一个新的时间后，检查缓存中是否还有需要重发的数据并重发。如果为空，关闭计时器
	- 如果重发的数据是`零窗口探针`，不需要执行back off机制。SYN数据包可能和探针有类似的，需要注意排除
	- 每次重发都应该重新设置`alarm`的时刻

5. **数据的接收**
	- Sender会接受一个Receiver发送的数据包，包含确认号及`window_size`和`RST`标志
	- 清空缓存中所有被`完全确认`的数据包，一旦有数据包被确认，恢复`RTO`和 `retransmitted_times_`
	- 同理，缓存不为空，重新设置`alarm`时刻。否则关闭timer


