cmake_minimum_required(VERSION 3.15)

project(myxml2 LANGUAGES C)
#include(CheckCSourceCompiles)
#include(CheckIncludeFiles)
#include(CheckStructHasMember)
#include(CheckFunctionExists)
#include(CheckLibraryExists)
#include(CheckSymbolExists)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckIncludeFile)
include(CheckLibraryExists)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(LIBXML2_WITH_AUTOMATA ON)
option(LIBXML2_WITH_C14N "Add the Canonicalization support" ON)
option(LIBXML2_WITH_CATALOG "Add the Catalog support" ON)
option(LIBXML2_WITH_DEBUG "Add the debugging module" ON)
option(LIBXML2_WITH_DOCB "Add Docbook SGML support" ON)
set(LIBXML2_WITH_EXPR ON)
option(LIBXML2_WITH_FTP "Add the FTP support" ON)
option(LIBXML2_WITH_HTML "Add the HTML support" ON)
option(LIBXML2_WITH_HTTP "Add the HTTP support" ON)
option(LIBXML2_WITH_ICONV "Add ICONV support" OFF)
option(LIBXML2_WITH_ICU "Add ICU support" OFF)
option(LIBXML2_WITH_ISO8859X "Add ISO8859X support if no iconv" ON)
option(LIBXML2_WITH_LEGACY "Add deprecated APIs for compatibility" ON)
option(LIBXML2_WITH_LZMA "Use liblzma" OFF)
option(LIBXML2_WITH_MEM_DEBUG "Add the memory debugging module" OFF)
option(LIBXML2_WITH_MODULES "Add the dynamic modules support" ON)
option(LIBXML2_WITH_OUTPUT "Add the serialization support" ON)
option(LIBXML2_WITH_PATTERN "Add the xmlPattern selection interface" ON)
option(LIBXML2_WITH_PROGRAMS "Build programs" ON)
option(LIBXML2_WITH_PUSH "Add the PUSH parser interfaces" ON)
option(LIBXML2_WITH_PYTHON "Build Python bindings" OFF)
option(LIBXML2_WITH_READER "Add the xmlReader parsing interface" ON)
option(LIBXML2_WITH_REGEXPS "Add Regular Expressions support" ON)
option(LIBXML2_WITH_RUN_DEBUG "Add the runtime debugging module" OFF)
option(LIBXML2_WITH_SAX1 "Add the older SAX1 interface" ON)
option(LIBXML2_WITH_SCHEMAS "Add Relax-NG and Schemas support" ON)
option(LIBXML2_WITH_SCHEMATRON "Add Schematron support" ON)
option(LIBXML2_WITH_THREADS "Add multithread support" ON)
option(LIBXML2_WITH_THREAD_ALLOC "Add per-thread memory" OFF)
option(LIBXML2_WITH_TREE "Add the DOM like tree manipulation APIs" ON)
set(LIBXML2_WITH_TRIO OFF)
set(LIBXML2_WITH_UNICODE ON)
option(LIBXML2_WITH_VALID "Add the DTD validation support" ON)
option(LIBXML2_WITH_WRITER "Add the xmlWriter saving interface" ON)
option(LIBXML2_WITH_XINCLUDE "Add the XInclude support" ON)
option(LIBXML2_WITH_XPATH "Add the XPATH support" ON)
option(LIBXML2_WITH_XPTR "Add the XPointer support" ON)
option(LIBXML2_WITH_ZLIB "Use libz" OFF)

set(LIBXML_MAJOR_VERSION 2)
set(LIBXML_MINOR_VERSION 9)
set(LIBXML_MICRO_VERSION 10)

set(VERSION "${LIBXML_MAJOR_VERSION}.${LIBXML_MINOR_VERSION}.${LIBXML_MICRO_VERSION}")
set(LIBXML_VERSION ${LIBXML_MAJOR_VERSION}0${LIBXML_MINOR_VERSION}0${LIBXML_MICRO_VERSION})
set(LIBXML_VERSION_STRING "${LIBXML_VERSION}")
set(LIBXML_VERSION_EXTRA "")
set(LIBXML_VERSION_NUMBER ${LIBXML_VERSION})

foreach(VARIABLE IN ITEMS WITH_AUTOMATA WITH_C14N WITH_CATALOG WITH_DEBUG WITH_DOCB WITH_EXPR WITH_FTP WITH_HTML WITH_HTTP WITH_ICONV WITH_ICU WITH_ISO8859X WITH_SAX1 WITH_LEGACY WITH_LZMA WITH_MEM_DEBUG WITH_MODULES WITH_OUTPUT WITH_PATTERN WITH_PUSH WITH_READER WITH_REGEXPS WITH_RUN_DEBUG WITH_SCHEMAS WITH_SCHEMATRON WITH_THREADS WITH_THREAD_ALLOC WITH_TREE WITH_TRIO WITH_UNICODE WITH_VALID WITH_WRITER WITH_XINCLUDE WITH_XPATH WITH_XPTR WITH_ZLIB)
    if (LIBXML2_${VARIABLE})
        set(${VARIABLE} 1)
    else()
        set(${VARIABLE} 0)
    endif()
endforeach()

check_c_source_compiles("
		void __attribute__((destructor))
		f(void) {}
		int main(void) { return 0; }
	" ATTRIBUTE_DESTRUCTOR)
check_c_source_compiles("
		#include <netdb.h>
		int main() { (void) gethostbyname((const char*) \"\"); return 0; }
	" GETHOSTBYNAME_ARG_CAST_CONST)
if(NOT GETHOSTBYNAME_ARG_CAST_CONST)
	set(GETHOSTBYNAME_ARG_CAST "(char *)")
else()
	set(GETHOSTBYNAME_ARG_CAST "/**/")
endif()
CHECK_INCLUDE_FILE(arpa/inet.h HAVE_ARPA_INET_H)
check_include_files(arpa/nameser.h HAVE_ARPA_NAMESER_H)
check_struct_has_member("struct sockaddr_storage" ss_family "sys/socket.h;sys/types.h" HAVE_SS_FAMILY)
check_struct_has_member("struct sockaddr_storage" __ss_family "sys/socket.h;sys/types.h" HAVE_BROKEN_SS_FAMILY)
if(HAVE_BROKEN_SS_FAMILY)
	set(ss_family __ss_family)
endif()
check_function_exists(class HAVE_CLASS)
check_include_files(ctype.h HAVE_CTYPE_H)
check_include_files(dirent.h HAVE_DIRENT_H)
check_include_files(dlfcn.h HAVE_DLFCN_H)
check_library_exists(dl dlopen "" HAVE_DLOPEN)
check_include_files(dl.h HAVE_DL_H)
check_include_files(errno.h HAVE_ERRNO_H)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_function_exists(finite HAVE_FINITE)
check_include_files(float.h HAVE_FLOAT_H)
check_function_exists(fpclass HAVE_FPCLASS)
check_function_exists(fprintf HAVE_FPRINTF)
check_function_exists(fp_class HAVE_FP_CLASS)
check_function_exists(ftime HAVE_FTIME)
check_function_exists(getaddrinfo HAVE_GETADDRINFO)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_function_exists(isascii HAVE_ISASCII)
check_function_exists(isinf HAVE_ISINF)
check_function_exists(isnan HAVE_ISNAN)
check_function_exists(isnand HAVE_ISNAND)
check_library_exists(history append_history "" HAVE_LIBHISTORY)
check_library_exists(lzma lzma_code "" HAVE_LIBLZMA)
check_library_exists(pthread pthread_join "" HAVE_LIBPTHREAD)
check_library_exists(readline readline "" HAVE_LIBREADLINE)
check_library_exists(z gzread "" HAVE_LIBZ)
check_include_files(limits.h HAVE_LIMITS_H)
check_function_exists(localtime HAVE_LOCALTIME)
check_include_files(lzma.h HAVE_LZMA_H)
check_include_files(malloc.h HAVE_MALLOC_H)
check_include_files(math.h HAVE_MATH_H)
check_include_files(memory.h HAVE_MEMORY_H)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(munmap HAVE_MUNMAP)
check_symbol_exists(DIR ndir.h HAVE_NDIR_H)
check_include_files(netdb.h HAVE_NETDB_H)
check_include_files(netinet/in.h HAVE_NETINET_IN_H)
check_include_files(poll.h HAVE_POLL_H)
check_function_exists(printf HAVE_PRINTF)
check_include_files(pthread.h HAVE_PTHREAD_H)
check_function_exists(putenv HAVE_PUTENV)
check_function_exists(rand HAVE_RAND)
check_function_exists(rand_r HAVE_RAND_R)
check_include_files(resolv.h HAVE_RESOLV_H)
check_library_exists(dld shl_load "" HAVE_SHLLOAD)
check_function_exists(signal HAVE_SIGNAL)
check_include_files(signal.h HAVE_SIGNAL_H)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(sprintf HAVE_SPRINTF)
check_function_exists(srand HAVE_SRAND)
check_function_exists(sscanf HAVE_SSCANF)
check_function_exists(stat HAVE_STAT)
check_include_files(stdarg.h HAVE_STDARG_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_function_exists(strftime HAVE_STRFTIME)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(string.h HAVE_STRING_H)
check_symbol_exists(DIR sys/dir.h HAVE_SYS_DIR_H)
check_include_files(sys/mman.h HAVE_SYS_MMAN_H)
check_symbol_exists(DIR sys/ndir.h HAVE_SYS_NDIR_H)
check_include_files(sys/select.h HAVE_SYS_SELECT_H)
check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_function_exists(time HAVE_TIME)
check_include_files(time.h HAVE_TIME_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_function_exists(va_copy HAVE_VA_COPY)
check_function_exists(vfprintf HAVE_VFPRINTF)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(vsprintf HAVE_VSPRINTF)
check_function_exists(__va_copy HAVE___VA_COPY)
check_c_source_compiles("
	#include <stdlib.h>
	#include <iconv.h>
	extern
	#ifdef __cplusplus
	\"C\"
	#endif
	#if defined(__STDC__) || defined(__cplusplus)
	size_t iconv(iconv_t cd, char** inbuf, size_t* inbytesleft, char** outbuf, size_t* outbytesleft);
	#else
	size_t iconv();
	#endif
	int main() { return 0; }
" ICONV_CONST_TEST)
if(NOT ICONV_CONST_TEST)
	set(ICONV_CONST "const")
endif()
set(LT_OBJDIR ".libs/")
check_c_source_compiles("
	#include <sys/socket.h>
	#include <sys/types.h>
	int main() { (void) send(1, (const char*) \"\", 1, 1); return 0; }
" SEND_ARG2_CAST_CONST)
if(NOT SEND_ARG2_CAST_CONST)
	set(SEND_ARG2_CAST "(char *)")
else()
	set(SEND_ARG2_CAST "/**/")
endif()
check_include_files("float.h;stdarg.h;stdlib.h;string.h" STDC_HEADERS)
check_c_source_compiles("
	#include <stdarg.h>
	void a(va_list* ap) {};
	int main() { va_list ap1, ap2; a(&ap1); ap2 = (va_list) ap1; return 0; }
" VA_LIST_IS_ARRAY_TEST)
if(VA_LIST_IS_ARRAY_TEST)
	set(VA_LIST_IS_ARRAY FALSE)
else()
	set(VA_LIST_IS_ARRAY TRUE)
endif()
check_c_source_compiles("
	#include <stddef.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	int main() { (void) getsockopt(1, 1, 1, NULL, (socklen_t*) NULL); return 0; }
" XML_SOCKLEN_T_SOCKLEN_T)
if(XML_SOCKLEN_T_SOCKLEN_T)
	set(XML_SOCKLEN_T socklen_t)
else()
	check_c_source_compiles("
		#include <stddef.h>
		#include <sys/socket.h>
		#include <sys/types.h>
		int main() { (void) getsockopt(1, 1, 1, NULL, (size_t*) NULL); return 0; }
	" XML_SOCKLEN_T_SIZE_T)
	if(XML_SOCKLEN_T_SIZE_T)
		set(XML_SOCKLEN_T size_t)
	else()
		check_c_source_compiles("
			#include <stddef.h>
			#include <sys/socket.h>
			#include <sys/types.h>
			int main() { (void) getsockopt (1, 1, 1, NULL, (int*) NULL); return 0; }
		" XML_SOCKLEN_T_INT)
		set(XML_SOCKLEN_T int)
	endif()
endif()


file(COPY ${libxml2_dir} DESTINATION ./)
set(libxml2_original ${CMAKE_CURRENT_BINARY_DIR}/libxml2)
include_directories(${libxml2_original}/include)
set(third_party_libxml2 ${CMAKE_CURRENT_BINARY_DIR}/../../../../third_party/libxml2)
configure_file(${third_party_libxml2}/config.h.cmake.in ${third_party_libxml2}/include/config.h)
set(libxml2_original_header
    ${libxml2_original}/include/libxml/c14n.h
    ${libxml2_original}/include/libxml/catalog.h
    ${libxml2_original}/include/libxml/chvalid.h
    ${libxml2_original}/include/libxml/debugXML.h
    ${libxml2_original}/include/libxml/dict.h
    ${libxml2_original}/include/libxml/DOCBparser.h
    ${libxml2_original}/include/libxml/encoding.h
    ${libxml2_original}/include/libxml/entities.h
    ${libxml2_original}/include/libxml/globals.h
    ${libxml2_original}/include/libxml/hash.h
    ${libxml2_original}/include/libxml/HTMLparser.h
    ${libxml2_original}/include/libxml/HTMLtree.h
    ${libxml2_original}/include/libxml/list.h
    ${libxml2_original}/include/libxml/nanoftp.h
    ${libxml2_original}/include/libxml/nanohttp.h
    ${libxml2_original}/include/libxml/parser.h
    ${libxml2_original}/include/libxml/parserInternals.h
    ${libxml2_original}/include/libxml/pattern.h
    ${libxml2_original}/include/libxml/relaxng.h
    ${libxml2_original}/include/libxml/SAX.h
    ${libxml2_original}/include/libxml/SAX2.h
    ${libxml2_original}/include/libxml/schemasInternals.h
    ${libxml2_original}/include/libxml/schematron.h
    ${libxml2_original}/include/libxml/threads.h
    ${libxml2_original}/include/libxml/tree.h
    ${libxml2_original}/include/libxml/uri.h
    ${libxml2_original}/include/libxml/valid.h
    ${libxml2_original}/include/libxml/xinclude.h
    ${libxml2_original}/include/libxml/xlink.h
    ${libxml2_original}/include/libxml/xmlIO.h
    ${libxml2_original}/include/libxml/xmlautomata.h
    ${libxml2_original}/include/libxml/xmlerror.h
    ${libxml2_original}/include/libxml/xmlexports.h
    ${libxml2_original}/include/libxml/xmlmemory.h
    ${libxml2_original}/include/libxml/xmlmodule.h
    ${libxml2_original}/include/libxml/xmlreader.h
    ${libxml2_original}/include/libxml/xmlregexp.h
    ${libxml2_original}/include/libxml/xmlsave.h
    ${libxml2_original}/include/libxml/xmlschemas.h
    ${libxml2_original}/include/libxml/xmlschemastypes.h
    ${libxml2_original}/include/libxml/xmlstring.h
    ${libxml2_original}/include/libxml/xmlunicode.h
    ${libxml2_original}/include/libxml/xmlwriter.h
    ${libxml2_original}/include/libxml/xpath.h
    ${libxml2_original}/include/libxml/xpathInternals.h
    ${libxml2_original}/include/libxml/xpointer.h
    ${libxml2_original}/include/wsockcompat.h
)

set(libxml2_original_other_header
    ${libxml2_original}/buf.h
    ${libxml2_original}/elfgcchack.h
    ${libxml2_original}/enc.h
    ${libxml2_original}/libxml.h
    ${libxml2_original}/save.h
    ${libxml2_original}/timsort.h
)

set(libxml2_original_source
    ${libxml2_original}/buf.c
    ${libxml2_original}/c14n.c
    ${libxml2_original}/catalog.c
    ${libxml2_original}/chvalid.c
    ${libxml2_original}/debugXML.c
    ${libxml2_original}/dict.c
    ${libxml2_original}/encoding.c
    ${libxml2_original}/entities.c
    ${libxml2_original}/error.c
    ${libxml2_original}/globals.c
    ${libxml2_original}/hash.c
    ${libxml2_original}/HTMLparser.c
    ${libxml2_original}/HTMLtree.c
    ${libxml2_original}/legacy.c
    ${libxml2_original}/list.c
    ${libxml2_original}/nanoftp.c
    ${libxml2_original}/nanohttp.c
    ${libxml2_original}/parser.c
    ${libxml2_original}/parserInternals.c
    ${libxml2_original}/pattern.c
    ${libxml2_original}/relaxng.c
    ${libxml2_original}/SAX.c
    ${libxml2_original}/SAX2.c
    ${libxml2_original}/schematron.c
    ${libxml2_original}/threads.c
    ${libxml2_original}/tree.c
    ${libxml2_original}/uri.c
    ${libxml2_original}/valid.c
    ${libxml2_original}/xinclude.c
    ${libxml2_original}/xlink.c
    ${libxml2_original}/xmlIO.c
    ${libxml2_original}/xmlmemory.c
    ${libxml2_original}/xmlmodule.c
    ${libxml2_original}/xmlreader.c
    ${libxml2_original}/xmlregexp.c
    ${libxml2_original}/xmlsave.c
    ${libxml2_original}/xmlschemas.c
    ${libxml2_original}/xmlschemastypes.c
    ${libxml2_original}/xmlstring.c
    ${libxml2_original}/xmlunicode.c
    ${libxml2_original}/xmlwriter.c
    ${libxml2_original}/xpath.c
    ${libxml2_original}/xpointer.c
    ${libxml2_original}/xzlib.c
)

#set(CMAKE_THREAD_LIBS_INIT "-lpthread")
#message("pthread " ${CMAKE_THREAD_LIBS_INIT})
find_package(Threads REQUIRED)
add_library(myxml2 STATIC ${libxml2_original_header} ${libxml2_original_other_header} ${libxml2_original_source})
target_link_libraries(myxml2 PRIVATE Threads::Threads)

if(WIN32)
	target_link_libraries(myxml2 PRIVATE ws2_32)
	set(WIN32_EXTRA_LIBADD "-lws2_32")
endif()
configure_file(${libxml2_original}/include/libxml/xmlversion.h.in ${libxml2_original}/include/libxml/xmlversion.h)

file(COPY ${libxml2_original_header} DESTINATION ./include/libxml)
file(COPY ${libxml2_original}/include/libxml/xmlversion.h DESTINATION ./include/libxml)