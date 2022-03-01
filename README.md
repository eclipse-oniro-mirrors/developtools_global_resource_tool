# global_resource_tool

#### 介绍
OpenHarmony 资源编译编译工具

#### 软件架构
>developtools/   
>>----global_resource_tool
>>>----include  
>>>----src  
>>>----third_party 依赖三方库编译脚本  
>>>----CMakeLists.txt  
>>>----win32.cmake windows交叉编译脚本  


#### 安装教程

1.  编译环境gcc/g++ 9.3.0
2.  cmake 版本最低3.15
3.  与global_resource_tool同级目录新建build
4.  cd build
5.  cmake ../global_resource_tool
6.  make
7.  编译结果输出restool

#### 使用说明

1.  restool -v 显示版本信息
2.  restool 显示帮助信息
