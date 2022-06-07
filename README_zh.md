# OpenHarmony资源编译工具

## 介绍
资源编译工具属于PC端工具，在OpenHarmony SDK中toolchain目录下，提供给IDE使用，支持Windows, Linux, MacOS平台。

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
|    |----win32.cmake Windows交叉编译脚本  
```

## 使用说明

### SDK编译命令

[SDK编译命令参考](https://gitee.com/openharmony/build/blob/master/README_zh.md)

### 测试用例

PC 上运行 `python test.py ./restool  ./out`  
 
### 命令帮助

本工具由IDE和OpenHarmony编译系统集成调用。    

支持的命令行参数，例如：

-v 显示工具版本号  
-i 资源输入目录  
-o 资源输出目录  
-r 资源ID头文件  
-p 应用包名  

`./restool -i main -o out -r out/ResourceTable.h -p ohos.demo`    

## 相关仓

[third_party_libxml2](https://gitee.com/openharmony/third_party_libxml2/README_zh.md)

[third_party_jsoncpp](https://gitee.com/openharmony/third_party_jsoncpp/README_zh.md)

[third_party_sqlite](https://gitee.com/openharmony/third_party_sqlite/README_zh.md)

[third_party_bounds_checking_function](https://gitee.com/openharmony/third_party_bounds_checking_function/README_zh.md)
