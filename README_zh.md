# OpenHarmony 应用资源编译工具

## 介绍
资源编译工具(resource tool)属于PC端工具，在OpenHarmony SDK中 toolchain 目录下。 提供给IDE使用，支持window, linux, macos平台。

## 目录

```
/developtools
|----global_resource_tool
|    |----include  
|    |----src  
|    |----test  
|    |----build 依赖三方库编译脚本  
|    |----BUILD.gn  
|    |----CMakeLists.txt  
|    |----win32.cmake windows交叉编译脚本  
```

## 使用说明

### 快速调试编译

1.  ubuntu 18 系统预装 gcc/g++  
2.  cmake 版本最低3.15  
3.  与global_resource_tool同级目录新建build  
```
4.  cd build  
5.  cmake ../global_resource_tool  
6.  make
```  
7.  编译结果输出restool  

### SDK编译命令

```
1.  ./build.sh --product-nane ohos-sdk  
```
SDK 编译参考https://gitee.com/openharmony/build/blob/master/README_zh.md 仓编译sdk说明。  

### 测试用例

```
1.PC 上运行 python test/test.py 参数1  参数2  
```
参数1  restool 命令路径  
参数2  输出结果路径  

### 命令帮助

本工具一般被IDE 和OpenHarmony 编译系统集成调用。    

手动执行./restool 会提示支持的命令行参数。简单命令参数如下：

-v 显示工具版本号  
-i 资源输入目录  
-o 资源输出目录  
-r 资源ID头文件  
-p 应用包名  
```
./restool -i main -o out -r out/ResourceTable.h -p ohos.demo  
```
推荐使用IDE工具。  

## 相关仓

third_party_libxml2  
third_party_jsoncpp  
third_party_sqlite  
third_party_bounds_checking_function
