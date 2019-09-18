CUR_ROOT = .
MAC=64

BOOST=$(THIRD)/boost

GCC = g++
CPPFLAGS = 	 -g -O2 -finline-functions \
		-Wall -W -Wshadow -Wpointer-arith -Wcast-qual -Wwrite-strings \
		-Woverloaded-virtual \
		-Wno-unused-parameter -Wno-unused-function -fpermissive -fPIC -DLINUX \
		-D_USE_DOUBLE_POINT_
   LDFLAGS     =  -pg


INCLUDE_PATH = -I ../lib/spatial-base/output\
				-I ../lib/type_factory/output\
				-I ../lib/styleparser/output\
				-I ../lib/tx_tools/output\
	
				
LIB_PATH=		-L ../lib/styleparser/output -lstyleparser\
				-L ../lib/spatial-base/output -lwslb_d\
				-L ../lib/type_factory/output -ltype_factory_d\
				-L ../lib/tx_tools/output -ltx_tools\

LDFLAGS     =  $(LIB_PATH) -lm
EXENAME  = catalog
OUTPUT=./output
OUTPUT_OBJ=./outputobj


##################################################################################
OBJ := $(patsubst %.cpp,$(OUTPUT_OBJ)/%.o,$(wildcard *.cpp))
OBJ += $(patsubst %.c,$(OUTPUT_OBJ)/%.o,$(wildcard *.c))
LIB_OBJ := $(patsubst %.cpp,$(OUTPUT_OBJ)/%.o,$(wildcard mp_*.cpp))
LIB_OBJ += $(patsubst %.c,$(OUTPUT_OBJ)/%.o,$(wildcard mp_*.c))
##################################################################################

.PHONY: all clean

#OBJNAME: 可以是代码库名，也可以是二进制可执行程序名
all: outputdir $(EXENAME)
	@echo "**************************************************************"
	@echo "$(EXENAME) project pass the build, you can use it!GO, GO, GO!"
	@echo "**************************************************************"
outputdir:
	mkdir $(OUTPUT) 2>/dev/null || echo
	mkdir $(OUTPUT)/bin 2>/dev/null || echo
	mkdir $(OUTPUT)/conf 2>/dev/null || echo
	mkdir $(OUTPUT)/log 2>/dev/null || echo
		
	mkdir $(OUTPUT_OBJ) 2>/dev/null || echo	
	@echo "OBJ: $(OBJ)"	
	@echo "CUR_ROOT: $(CUR_ROOT)"
	
clean:
	rm -rf *.o $(OBJ) $(EXENAME)  $(OUTPUT_OBJ)	 $(OUTPUT)	

install:
	cp $(OBJNAME) $(OUTPUT)/bin -rf 2>/dev/null
	cp ./conf/* $(OUTPUT)/conf -rf 2>/dev/null

$(OUTPUT_OBJ)/%.o	: %.cpp
	$(GCC) $(CPPFLAGS) -c $< -o $@ $(INCLUDE_PATH) 	

$(OUTPUT_OBJ)/%.o	: %.c
	$(GCC) $(CPPFLAGS) -c $< -o $@ $(INCLUDE_PATH) 		
	
$(EXENAME) : $(OBJ)	
	$(GCC) -o $(EXENAME) $(OBJ) -Xlinker "-(" $(LIB_PATH) $(LDFLAGS) --export-dynamic -rdynamic -Xlinker  "-)" 
	cp $(EXENAME) $(OUTPUT)/bin	
	cp conf $(OUTPUT)/ -rf
	
	
