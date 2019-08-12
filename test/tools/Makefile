CUR_ROOT=.
MAC=64

BOOST=$(THIRD)/boost

GCC=g++
CPPFLAGS=-g -O2 -std=c++11 -finline-functions -std=gnu++0x \
		-Wall -W -Wshadow -Wpointer-arith \
		-Wcast-qual -Wwrite-strings -Woverloaded-virtual \
		-Wno-unused-parameter -Wno-unused-function \
		-fpermissive -DLINUX -D_USE_DOUBLE_POINT_

INCLUDE_PATH=-I ../../../../lib/spatial-base/output \
        -I ../../../../lib/type_factory/output \
		-I ../../../../lib/styleparser/output \
		-I ../../../../lib/tx_tools/output \
		-I ../../../../lib/conf_helper/output \
		-I ../../../../lib/htk/dist/include
				
LIB_PATH=-L ../../../../lib/styleparser/output -lstyleparser \
		-L ../../../../lib/type_factory/output -ltype_factory_d \
        -L ../../../../lib/spatial-base/output -lwslb_d \
		-L ../../../../lib/tx_tools/output -ltx_tools \
		-L ../../../../lib/conf_helper/output -lconf_helper \
		-L ../../../../lib/htk/dist/lib -lhtk

LDFLAGS=$(LIB_PATH) -lm -lpthread

.PHONY: all clean

#OBJNAME: 可以是代码库名，也可以是二进制可执行程序名
all: mifdiff

mifdiff: mifdiff.cpp progress.h
	$(GCC) $< $(CPPFLAGS) $(INCLUDE_PATH) $(LDFLAGS) -o $@

clean:
	rm -rf ./mifdiff	
