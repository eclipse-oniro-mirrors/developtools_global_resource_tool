# OpenHarmony resource compile tool  

## Description
resource tool is used in computer.In OpenHarmony SDK toolchain.Provide to IDE.Support Windows, Linux, MacOS platform.

## Directory Structure

```
/developtools   
|----global_resource_tool
|    |----include  
|    |----src  
|    |----test
|    |----build dependence third patry lib make script  
|    |----CMakeLists.txt  
|    |----BUILD.gn  
|    |----win32.cmake Windows cross compile script
```

## Instructions

### SDK Build

[SDK build refer](https://gitee.com/openharmony/build/blob/master/README_zh.md)

### Test

In PC, run `python test.py ./restool ./out`  

### Help

resource tool usually integrate to IDE, OpenHarmony compile system.

resource tool command e.g:  

-v version  
-i input resource path  
-o output resource path  
-r resource ID header file path  
-p package name  

`./restool -i main -o out -r out/ResourceTable.h -p ohos.demo`  

##  Related to the storehouse

[third_party_libxml2](https://gitee.com/openharmony/third_party_libxml2/README.md)

[third_party_jsoncpp](https://gitee.com/openharmony/third_party_jsoncpp/README.md)

[third_party_sqlite](https://gitee.com/openharmony/third_party_sqlite/README.md)

[third_party_bounds_checking_function](https://gitee.com/openharmony/third_party_bounds_checking_function/README.md)

