Checkpoint 1 Writeup
====================

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

I was surprised by or edified to learn that: [describe]

Report from the hands-on component of the lab checkpoint: [include
information from 2.1(4), and report on your experience in 2.2]

Describe Reassembler structure and design. [Describe data structures and
approach taken. Describe alternative designs considered or tested.
Describe benefits and weaknesses of your design compared with
alternatives -- perhaps in terms of simplicity/complexity, risk of
bugs, asymptotic performance, empirical performance, required
implementation time and difficulty, and other factors. Include any
measurements if applicable.]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I'm not sure about: [describe]


## 函数实现


### void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )

函数的语义是接受data和对应的first_index插入ByteStream中，如果数据在Reassembler的接收窗口内但是前面的数据存在gap，就需要缓存。

Reaseembler的接收窗口为 
$$[first_unassemblered_index,  first\_unassemblered\_index+availale\_capacity)$$

当完整接受到最后一个substring，Reassembler还要负责关闭ByteStream。


整体的流程如下：
- 1. 确定输入数据和接收窗口的重叠部分，如果没有重叠直接退出。
- 2. 重叠部分前有gap，缓存数据，跳到4.；缓存部分没有gap，写入数据，跳到 3.
- 3. 缓存中可写入的数据应写尽写
- 4. 检查是否需要关闭ByteStraem



### void Reassembler::buffer_it(uint64_t index, std::string data) 

buffer\_it 函数是缓存的唯一入口，为了保证保存internal_buffer_的简单和易用，其保证了两个不变量 
-   1.缓存中所有的pair所代表的interval不能重叠
-   2.缓存中所有的pair的first index应该始终大于等于first_unassemblered_index()
  

这样设计的原因是：使得缓存拉取函数drain()的时候，只需要简单查找而不需要左右判断切分。
为了保持这一不变量，每当push入新的数据后，写入窗口变化，对应的缓存就需要通过normalize_buffer()进行整理。


buffer\_it函数主要的设计就是：对于每一个缓存的pair对代表的interval，不按照和即将缓存的interval根据不同的重叠方式去判断，这种方式很繁琐也很容易出错。取而代之的是将所有缓存的interval都同时判断左重叠和右重叠。
如果是与缓存的interval重叠在右侧，那么
```c++
 if (s < L) { // prefix
      data = it->second.substr(0, L - s) + data;
      L = s;
    }
```
如果是与缓冲的interval重叠在左侧，那么
```c++
 if (e > R) { // suffix
      data += it->second.substr(R - s);
      R = e;
    }
```

### void Reassembler::buffer_it(uint64_t index, std::string data) 

由于buffer\_it保持了缓存的不变量，所以拉取缓存的时候就很简单了，找到对应需要的起始index，然后按照容量写入。
如果完整写入一个缓存的interval，直接删除这个缓存，否则保留对应剩余的slice重新写入缓存。


### void Reassembler::normalize_buffer()

normalize_buffer用于写入窗口变化时保证缓存的不变量。具体思路就是
- 1.遍历所有可能重叠的节点
- 2.如果是完全重叠，直接删除；部分重叠，保留未重叠部分，重新写入缓存。
- 3.遇到部分重叠，退出循环

因为有且只有一个interval会与当前已push的数据存在部分重叠，所以可以用此作为循环退出的条件。