# OpenHarmony resource compile tool  

## Description
resource tool is used in computer.In OpenHarmony SDK toolchain.Provide to IDE.Support window,linux, macos platform.

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
|    |----win32.cmake windows cross compile script
```

## Instructions

### Quickly Build

1.  ubuntu 18 preinstalled gcc/g++  
2.  cmake version mini 3.15 required  
```
3.  mkdir build  
4.  cd build  
5.  cmake ../global_resource_tool    
6.  make
```  
7.  compile result restool binary  

### SDK Build

1. ` ./build.sh --product-name ohos-sdk `  

[SDK build refer]( https://gitee.com/openharmony/build/blob/master/README_zh.md )

### Test

1.in PC,  run `python test/test.py param1 param2`  

param1:  restool path  
param2:  result path  

### Help

restool usually integrate to IDE, OpenHarmony compile system.

run ./restool, show command:  

-v version  
-i input resource path  
-o output resource path  
-r resource ID header file path  
-p package name  

`./restool -i main -o out -r out/ResourceTable.h -p ohos.demo`  

using IDE(recommonded)
##  Related to the storehouse

third_party_libxml2  
third_party_jsoncpp  
third_party_sqlite  
third_party_bounds_checking_function

