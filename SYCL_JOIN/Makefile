# Name Parameters
VERSION = v4
JOINF = build/join_$(VERSION)_fpga_$(SEED)_$(MHZ)_$(BKTS)$(SLTS)
JOINH = build/join_$(VERSION)_hw_$(SEED)_$(MHZ)_$(BKTS)$(SLTS)

# Compilation Parameters
BKTS = -D_BKTS_=64
SLTS = -D_SLTS_=4
CONFIG = $(BKTS) $(SLTS) -w

# Runtime Parameters
## Real Data
TBDIM = 5 13 8 101
DSRC0 = /home/18643_team/18643_project/SYCL_JOIN/manual_unique_32.txt
DSRC1 = /home/18643_team/18643_project/SYCL_JOIN/out_less_32.txt
CJOIN = 2
## Dummy Data
TB1X = 1
TB1Y = 128
TB2X = 1
TB2Y = 1024
DDSEL = 5
## Both
HASH = 0

ARGS = $(TB1X) $(TB1Y) $(TB2X) $(TB2Y) $(DSRC0) $(DSRC0) $(CJOIN) $(DDSEL) $(HASH)
# ARGS = $(TBDIM) $(DSRC0) $(DSRC0) $(CJOIN) $(DDSEL) $(HASH)

TARGET = $(OFS_BSP_BASE):ofs_n6001
MHZ = 650MHz

SEED = 0

EARLY = -fsycl-link=early
SIMULATE = $(CONFIG) -DFPGA_EMULATOR=1 -fsycl -fintelfpga -qactypes -Wall -Wextra -g -O0
SYNTHESIS = $(CONFIG) -Xsv -fsycl -fintelfpga -qactypes -DFPGA_HARDWARE=1 -Xshardware -Xsclock=$(MHZ) -Xsseed=$(SEED) -Xstarget=$(TARGET) -DIS_BSP -O3
RTLSIM = $(CONFIG) -Xsv -fsycl -fintelfpga -DFPGA_SIMULATOR=1 -Xssimulation -Xstarget=$(TARGET) #-DIS_BSP -O3
CXX = $(CONFIG) -DCXXONLY -std=c++1y -g -O0 #-Wall

JOINSRC = main.cpp

sim:
	icpx $(CONFIG) $(SIMULATE) $(JOINSRC) -o build/join_sim
	build/join_sim $(ARGS) > test$(HASH).txt

rtl:
	icpx $(CONFIG) $(RTLSIM) $(JOINSRC) -o build/join_rtl
	build/join_rtl $(ARGS) > test$(HASH).txt

fpga:
	icpx $(CONFIG) $(SYNTHESIS) $(JOINSRC) -o $(JOINF)

link:
	icpx $(CONFIG) $(SYNTHESIS) $(EARLY) $(JOINSRC) -o $(JOINH)

############# misc ##################

clang:
	clang-format -i *.cpp *.h

clean:
	rm -f *#*#* *~ test.txt simplerun
reset:
	/ofs/ofs-2023.1-1/ofs_init.sh

resetold:
	/ofs/ofs-2023.1-1/ofs_init_old.sh

query:
	/ofs/ofs-2023.1-1/ofs_n6001_eval.sh

