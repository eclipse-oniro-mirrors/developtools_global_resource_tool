# OpenHarmony 应用资源编译工具

## 介绍
资源编译工具(resource tool, 缩写restool) 属于PC端工具，在OpenHarmony SDK中toolchain 目录下。在IDE生成 OpenHarmony 应用包过程中，调用restool 编译应用资源，编译结果传递给打包工具生成 OpenHarmony 应用包。支持window, linux, macos平台。

## 目录

```
/developtools
|----global_resource_tool
|    |----include  
|    |----src  
|    |----third_party 依赖三方库编译脚本  
|    |----CMakeLists.txt  
|    |----win32.cmake windows交叉编译脚本  
```

## 使用说明

### 代码编译命令

1.  编译环境gcc/g++ 9.3.0
2.  cmake 版本最低3.15
3.  与global_resource_tool同级目录新建build
4.  cd build
5.  cmake ../global_resource_tool
6.  make
7.  编译结果输出restool

### 命令帮助

本工具一般被IDE 和OpenHarmony 编译系统集成调用。    

手动执行./restool 会提示支持的命令行参数。简单命令参数如下：

-v 显示工具版本号  
-i 资源输入目录  
-o 资源输出目录  
-r 资源ID头文件  
-p 应用包名  

./restool -i main -o out -r out/ResourceTable.h -p ohos.demo  

推荐使用IDE工具。  

## 相关仓

third_party_libxml2  
third_party_jsoncpp  
third_party_sqlite  
third_party_bounds_checking_function
