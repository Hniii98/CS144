## 命令执行流

在主CMakelists.txt中，通过设置`CMAKE_MAKE_PROGRAM`为预设的`make-parallel`脚本实现了在键入`cmake --build build`命令时，（在unix中）原本应该执行`make`被替换为了执行脚本`${PROJECT_SOURCE_DIR}/scripts/make-parallel.sh -C build`

脚本中，通过`exec`命令将父进程的shell替换为`make -j`nproc` "$@"`保证进程树中没有多余的sh进程。\`nproc\` 又通过命令替换得到核心数同时"$@"将参数完整的传入当前的`make`命令中，不进行重新分词。

## 关注点分离

ect/文件夹下将CMakelist的不同选择配置分模块进行存放，通过主CMakelist的`include`引入。`include_directories`用于添加所有的头文件搜索路径。`add_subdirectory`用于添加project的不同子模块。

同时，CMakelists通过不同的方式控制默认编译的目标：
- `add_library(name EXCLUDE_FROM_ALL STATIC src.cc)` 默认加入all，需要显式排除
- `add_executable(name EXCLUDE_FROM_ALL src.cc)` 默认加入all，需要显式排除
- `add_custom_target(name ALL)`   默认不加入all，需要显式加入


## 测试框架的设计

在`tests.cmake`中定义了测试相关的cmake。CS144采用`CTest`这个框架进行测试。

### 生命周期
在进行测试程序运行之前首先就是得保证测试程序已经编译完成，这种生命周期的管理是依赖`fixture`来实现的。

这里的设计是将`编译命令`封装到`add_test()`测试，将它的属性设置为`FIXTURES_SETUP`，`FIXTURES_SETUP`会作为测试的起点，无视外部的并行设置，串行地执行对应的命令。

这样之后，其他的测试可以通过设置属性为`FIXTURES_REQUIRED`确保测试程序的编译是在运行之前完成的。

## 架构

在tests.cmake中，测试程序通过ttest 宏封装起来。随后通过`add_custom_target`来划分不同check所需要执行的测试程序。
