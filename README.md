# OpenHarmony resource compile tool  

## Description
restool(resource tool) is used in computer.In OpenHarmony SDK toolchain.When IDE pack OpenHarmony application, application resources will be compiled by restool, then the result compiling will be passed to pack tool.Support window,linux, macos platform.

## Directory Structure

```
/developtools   
|----global_resource_tool
|    |----include  
|    |----src  
|    |----third_party dependence third patry lib make script  
|    |----CMakeLists.txt  
|    |----win32.cmake windows cross compile script
```

## Instructions

### Compile

1.  gcc/g++ version 9.3.0 required
2.  cmake version mini 3.15 required
3.  mkdir build
4.  cd build
5.  cmake ../restool_standard
6.  make
7.  compile result restool binary

### Help

restool usually integrate to IDE, OpenHarmony compile system.

run ./restool, show command:  

-v version  
-i input resource path  
-o output resource path  
-r resource ID header file path  
-p package name  

using IDE(recommonded)
##  Related to the storehouse

third_party_libxml2  
third_party_jsoncpp  
third_party_sqlite  
third_party_bounds_checking_function

