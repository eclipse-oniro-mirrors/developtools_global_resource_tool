# OpenHarmony资源编译工具

## 介绍
资源编译工具属于PC端工具，在OpenHarmony SDK中toolchain目录下，提供给IDE使用，支持Windows, Linux, MacOS平台。

## 目录

```
/developtools
|----global_resource_tool
|    |----include         #头文件
|    |----src             #源代码文件
|    |----test            #测试用例
|    |----build           #依赖三方库编译脚本  
|    |----BUILD.gn        #编译脚本
|    |----CMakeLists.txt  #CMake文件
|    |----win32.cmake     #Windows交叉编译脚本  
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
-f 如果输出目录中已经存在结果，强制覆盖  
-m 编译多hap包，输入多个hap包的编译结果路径，例如：entry1,entry2,..  
-j 编译多hap包+多har时，合并后config.json的文件路径  
-e Id的开始标记  

```
entry目录结构
|    |----resource
|    |----config.json 或者 module.json 都兼容
```
`./restool -i entry1 -i entry2 -j xxx/module.json -p ohos.demo.xxx -o out -r out/ResourceTable.h `

## 相关仓

**global_resource_tool**

[third_party_libxml2](https://gitee.com/openharmony/third_party_libxml2/blob/master/README.md)

[third_party_jsoncpp](https://gitee.com/openharmony/third_party_jsoncpp/blob/master/README.md)

[third_party_sqlite](https://gitee.com/openharmony/third_party_sqlite/blob/master/README.md)

[third_party_bounds_checking_function](https://gitee.com/openharmony/third_party_bounds_checking_function/blob/master/README.md )
