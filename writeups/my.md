## Ref 类

`Ref`的主要功能在封装一个数据的`持有`或者`借用`。不论哪种状态，内部的常量指针都指向实际对象。而仅有在`拥有`状态下，obj_包装实际的对象，否则值为std::nullopt.

- Ref的默认构造函数构造一个`拥有`T类型对象的包裹，T对象的值为默认值，通过C++20的约束关键字保证T类型存在默认构造。

- 拷贝赋值和移动赋值的返回值都是Ref& , 这是为了支持类似 `Ref<int> a = b = c = Ref<int>{}`这种链式调用;

- Ref::borrow()函数为了创建一个`借用`当前所包裹对象的包裹，这里设计了一个标签类`uninitialized_t`，目的是为了和默认构造函数隔离，因为**默认构造函数创建的是`借用`状态。而接收标签类的构造函数函数体内没有任何实际行为，因此obj_仍然保持默认值std::nullopt来代表借用。**


## Address 类

`Address`的主要功能是封装多种协议簇，但是在CS144中，只用到IPv4、IPv6其实暂时没有涉及。

- Address类封装一个一个Raw类的对象，而Raw类对象封装了一个通用容器`sockaddr_storage类型的 storage`，这个容器可以装下任意协议簇的地址值。同时，实现了在`Raw`类中实现了到`sockaddr`的类型转换。**这样就可以在接受`sockaddr *`类型作为参数的接口中直接传入`Address`类型的变量，实现隐式转换**
- Address->Raw 这一层是属于成员变量读取，然后Raw中实现了类型转换符，这样只有一层的隐式转换，符合标准中**只允许一次隐式的用户自定义转换。
- `Address::Address( const string& ip, const uint16_t port )`和 `Address::Address( const string& hostname, const string& service )`都利用了委托构造函数的方式调用了`Address::Address( const string& node, const string& service, const addrinfo& hints )`这个构造函数

- `Address::Address( const string& node, const string& service, const addrinfo& hints )`这一个构造函数的逻辑中，最终目的是调用`Address::Address(const sockaddr *addr, size_t size)`这一个构造函数，然后通过一次的拷贝赋值`*this = Address()`实现对成员变量的赋值
  同时值得注意的是，在调用对应的构造函数之前，函数用unique_ptr包装了`addrinfo`：
```C++
auto addrinfo_deleter = []( addrinfo* const x ) { freeaddrinfo( x ); };
  unique_ptr<addrinfo, decltype( addrinfo_deleter )> wrapped_address( resolved_address, move( addrinfo_deleter ) );
```
 这是因为，异常抛出的时候，会在调用栈上逐层向上进行`栈展开(stack unwinding)`并析构对应的局部变量直到找到一个可以匹配的`catch`。这意味着动态申请的、由指针持有的变量不会自动析构。于是我们就需要智能指针来包裹这个`addrinfo`，同时在析构器上定义正确的析构函数。这样就能在异常抛出的时候利用RAII正确地析构。

- 而`Address Address::from_ipv4_numeric( const uint32_t ip_address )`就容易理解了，就是通过一个uint32_t的ip构造`sockaddr_in`再用类型转换为`sockadd*`匹配到`Address( const sockaddr* addr, std::size_t size );`

- Address在这里我设计了一个`Hash`的仿函数，便于提供给map容器来进行查找。

