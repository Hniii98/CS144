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


## unix_error

C++中的错误体系中包含两大类： 错误码封装体系std::error_code 异常继承体系 std::exception

1. std::error_code提供的是兼容C风格错误码的C++封装，它包含两个成员变量：
- int  _M_value
- const error_category* 	_M_cat

_M_value就是错误码本身，而error_category提供一个`错误码值->字符串`的转换。简单来说就是一个查找表，把用于程序识别的错误码转换成人类可直接阅读的字符串。

error_code不会改变程序的执行流，这点和exception不同。

2. std::exception提供一个C++的异常基类，它的接口非常简单
```
    virtual const char*
    what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW;
```
返回C风格字符串描述当前错误。关于exception最关键的就是，它会改变程序的执行流，当异常抛出的时候会在函数的调用栈向上搜索匹配的捕获处理程序（异常抛出位置后面所有的代码就不会执行了）。

这个过程中当前栈的局部对象会被析构，直到匹配到了对应的`catch`捕获。（如果没有任何`catch`捕获的话，最外层，如main()会调用std::terminate终止程序。

而这一行为带来的风险就是，如果你在某个栈中的某个变量持有了`动态`申请的资源，那么这个动态资源并不会正确被析构，可能会造成内存泄漏。因此，通常的解决方案是 ` 1. 采用基于栈分配的资源 2.栈无法实现就用只能指针包裹申请的资源，利用指针的RAII实现正确的析构。`

在exception的体系中，通常不需要引入额外的成员函数和变量。但在system_error中有点不同，system_error中封装了一个error_code。这里的设计哲学是希望提供一个支持错误码的异常。

**tagged_error** 这里的设计继承system_error，其实可以直接用system_error中封装的error_code来获取对应上文所讲的` _M_value`错误码。但是注意，system_error采用的设计是`私有封装+访问接口`的设计，如果这样做的话可能会不直观。

同时，在类中加了一个`attempt_and_error_`，这样在实例化一个`tagged_error`对象的时候，`what()`不仅可以用到error_code对应的错误原因，还可以提供一个额外的上下文来指示异常发生的原因。

这个用了两层封装`tagged_error` -> `unix_error`一开始我是觉得会不会多余？因为通常能是推荐用组合的方式来实现自己的异常类，只需要继承一个exception加入异常体系来支持多态的捕获，额外所需的信息用成员变量来支持。但是这里`1. 异常都是和系统调用有关的，继承system_error有自解释的功能 2. DNS解析的错误码不和其他错误码兼容，如Unix，需要实现自己的error_category`

在**unix_error**这里，错误码实现如下：
```
/* The error code set by various library functions.  */
extern int *__errno_location (void) __THROW __attribute_const__;
# define errno (*__errno_location ())
```
它会指向当前线程最后一个系统调用所产生的错误码，提供了一个线程安全的错误码获取接口。unix_error用这个错误码最为默认参数进行构，同时加上一个用户提供的上下文`s_attempt`,和全局的单例`std::system_category()`这个单例提供了系统调用错误码到字符串转换的接口，无需关心是什么操作系统。
