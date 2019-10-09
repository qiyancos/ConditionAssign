CUR_ROOT=.
MAC=64

BOOST=$(THIRD)/boost

GCC=g++
CPPFLAGS=-g -static -O2 -std=c++11 -finline-functions -std=gnu++0x \
		-Wall -W -Wshadow -Wpointer-arith \
		-Wcast-qual -Wwrite-strings -Woverloaded-virtual \
		-fpermissive -fPIC -DLINUX -D_USE_DOUBLE_POINT_ \
		-Wno-unused-parameter -Wno-unused-function \
        -Wno-overloaded-virtual -Wno-sign-compare \
        -DDEBUG -DUSE_TIMER

INCLUDE_PATH=-I ../../lib/rtree/dist/include \
        -I ../../lib/spatial-base/output \
		-I ../../lib/type_factory/output \
		-I ../../lib/styleparser/output \
		-I ../../lib/tx_tools/output \
		-I ../../lib/program_util/output \
		-I ../../lib/htk/dist/include
				
LIB_PATH=-L ../../lib/rtree/dist/lib -lrtree \
        -L ../../lib/styleparser/output -lstyleparser \
		-L ../../lib/spatial-base/output -lwslb_d \
		-L ../../lib/type_factory/output -ltype_factory_d \
		-L ../../lib/tx_tools/output -ltx_tools \
		-L ../../lib/program_util/output -lprogram_util \
		-L ../../lib/htk/dist/lib -lhtk

LDFLAGS=-g -static $(LIB_PATH) -lm -Wl,--whole-archive \
	    -lpthread -Wl,--no-whole-archive

EXENAME=ConditionAssign
OUTPUT=./bin
OUTPUT_OBJ=./outputobj


##################################################################################
OBJ := $(patsubst %.cpp,$(OUTPUT_OBJ)/%.o,$(wildcard *.cpp))
OBJ += $(patsubst %.c,$(OUTPUT_OBJ)/%.o,$(wildcard *.c))
LIB_OBJ := $(patsubst %.cpp,$(OUTPUT_OBJ)/%.o,$(wildcard mp_*.cpp))
LIB_OBJ += $(patsubst %.c,$(OUTPUT_OBJ)/%.o,$(wildcard mp_*.c))
##################################################################################

.PHONY: all clean

#OBJNAME: �����Ǵ��������Ҳ�����Ƕ����ƿ�ִ�г�����
all: outputdir $(OUTPUT)/$(EXENAME)
	@echo "**************************************************************"
	@echo "$(EXENAME) project pass the build, you can use it!GO, GO, GO!"
	@echo "**************************************************************"

outputdir:
	mkdir $(OUTPUT) 2>/dev/null || echo
	mkdir $(OUTPUT_OBJ) 2>/dev/null || echo	
	@echo "OBJ: $(OBJ)"	
	@echo "CUR_ROOT: $(CUR_ROOT)"
	
clean:
	rm -rf $(OUTPUT_OBJ) $(OUTPUT)	

install:
	cp $(OBJNAME) $(OUTPUT)/bin -rf 2>/dev/null
	cp ./conf/* $(OUTPUT)/conf -rf 2>/dev/null

$(OUTPUT_OBJ)/%.o	: %.cpp
	$(GCC) $(CPPFLAGS) -c $< -o $@ $(INCLUDE_PATH) 	

$(OUTPUT_OBJ)/%.o	: %.c
	$(GCC) $(CPPFLAGS) -c $< -o $@ $(INCLUDE_PATH) 		
	
$(OUTPUT)/$(EXENAME) : $(OBJ)
	$(GCC) -o $@ $(OBJ) -Xlinker "-(" $(LIB_PATH) $(LDFLAGS) -rdynamic -Xlinker  "-)" 
