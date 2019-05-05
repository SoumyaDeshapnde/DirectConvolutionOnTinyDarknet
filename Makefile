# Points to Utility Directory
COMMON_REPO = ../
ABS_COMMON_REPO = $(shell readlink -f $(COMMON_REPO))

include ./utils.mk
# Run Target:
#   hw  - Compile for hardware
#   sw_emu/hw_emu - Compile for software/hardware emulation
# FPGA Board Platform (Default ~ vcu1525)
#to change the make file to your files
#1.change the host name to the correct one:
#host_conv.c->whatever
#2.NNet is the name of the kernel file, and its object
#3. conv is the name of the acutal kernel function, and the executiable you call
#make sure these are all diffrent and you can do a simple find replace all for each one, if you decide to rename


TARGETS := hw
TARGET := $(TARGETS)
DEVICES := xilinx_vcu1525_dynamic
DEVICE := $(DEVICES)
XCLBIN := ./xclbin
DSA := $(call device2sandsa, $(DEVICE))


CXX := $(XILINX_SDX)/bin/xcpp
XOCC := $(XILINX_SDX)/bin/xocc

CXXFLAGS := $(opencl_CXXFLAGS) -Wall -O0 -g -std=c++14
LDFLAGS := $(opencl_LDFLAGS)

HOST_SRCS = src/host_NN.c

# Host compiler global settings
CXXFLAGS = -I $(XILINX_SDX)/runtime/include/1_2/ -I/$(XILINX_SDX)/Vivado_HLS/include/ -O0 -g -Wall -fmessage-length=0 -std=c++14
LDFLAGS = -lOpenCL -lpthread -lrt -lstdc++ -L$(XILINX_SDX)/runtime/lib/x86_64

# Kernel compiler global settings
CLFLAGS = -t $(TARGET) --platform $(DEVICE) --save-temps 

#NNet
#conv
#combo
BUILD_DIR := ./_x.$(TARGET).$(DSA)

BUILD_DIR_conv = $(BUILD_DIR)/conv

EXECUTABLE = combo

EMCONFIG_DIR = $(XCLBIN)/$(DSA)

BINARY_CONTAINERS_conv += $(XCLBIN)/conv.$(TARGET).$(DSA).xclbin
BINARY_CONTAINER_conv_OBJS += $(XCLBIN)/conv.$(TARGET).$(DSA).xo
BINARY_CONTAINER_conv_OBJS += $(XCLBIN)/maxpool.$(TARGET).$(DSA).xo

#Include Libraries
include $(ABS_COMMON_REPO)/libs/opencl/opencl.mk
include $(ABS_COMMON_REPO)/libs/xcl2/xcl2.mk
CXXFLAGS += $(xcl2_CXXFLAGS)
LDFLAGS += $(xcl2_LDFLAGS)
HOST_SRCS += $(xcl2_SRCS)

CP = cp -rf

.PHONY: all clean cleanall docs emconfig
all: $(EXECUTABLE) $(BINARY_CONTAINERS_conv) emconfig

.PHONY: exe
exe: $(EXECUTABLE)


.PHONY: build
build: $(BINARY_CONTAINERS)

# Building kernel conv
$(XCLBIN)/conv.$(TARGET).$(DSA).xo: ./src/convS.cl
	mkdir -p $(XCLBIN)
	$(XOCC) $(CLFLAGS)  --temp_dir $(BUILD_DIR_conv) -c  --profile_kernel stall:all:all:all  --report_level estimate  -k conv -I'$(<D)' -o'$@' '$<'
# Building kernel pool
$(XCLBIN)/maxpool.$(TARGET).$(DSA).xo: ./src/convS.cl
	mkdir -p $(XCLBIN)
	$(XOCC) $(CLFLAGS) --temp_dir $(BUILD_DIR_conv) -c  --profile_kernel stall:all:all:all --report_level estimate -k maxpool -I'$(<D)' -o'$@' '$<'

$(XCLBIN)/conv.$(TARGET).$(DSA).xclbin: $(BINARY_CONTAINER_conv_OBJS)
	mkdir -p $(XCLBIN)
	$(XOCC) $(CLFLAGS) --temp_dir $(BUILD_DIR_conv) -l $(LDCLFLAGS)  --profile_kernel data:all:all:all  --profile_kernel stall:all:all:all  --nk conv:1  --nk maxpool:1  -o'$@' $(+)



#$(XCLBIN)/maxpool.$(TARGET).$(DSA).xclbin: $(BINARY_CONTAINER_maxpool_OBJS)
#	$(XOCC) $(CLFLAGS) -l $(LDCLFLAGS) --nk maxpool:1 -o'$@' $(+)




# Building Host
$(EXECUTABLE): $(HOST_SRCS) $(HOST_HDRS)
	mkdir -p $(XCLBIN)
	$(CXX) $(CXXFLAGS) $(HOST_SRCS) $(HOST_HDRS) -o '$@' $(LDFLAGS)

emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(DEVICE) --od $(EMCONFIG_DIR)

check: all
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) ./$(EXECUTABLE)
else
	 ./$(EXECUTABLE)
endif
	sdx_analyze profile -i sdaccel_profile_summary.csv -f html

# Cleaning stuff
clean:
	-$(RMDIR) $(EXECUTABLE) $(XCLBIN)/{*sw_emu*,*hw_emu*} 
	-$(RMDIR) sdaccel_* TempConfig system_estimate.xtxt *.rpt
	-$(RMDIR) src/*.ll _xocc_* .Xil emconfig.json dltmp* xmltmp* *.log *.jou *.wcfg *.wdb

cleanall: clean
	-$(RMDIR) $(XCLBIN)
	-$(RMDIR) ./_x

.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Device."
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""
	$(ECHO) "  make check TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""

docs: README.md

README.md: description.json
	$(ABS_COMMON_REPO)/utility/readme_gen/readme_gen.py description.json

