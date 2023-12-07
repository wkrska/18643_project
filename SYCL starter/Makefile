VERSION = v2_2

AGGF = build/join_$(VERSION)_fpga_$(SEED)_$(MHZ)
AGGH = build/join_$(VERSION)_hw_$(SEED)_$(MHZ)

WHICH = -DWHICH=4

CONFIG = 

TARGET = $(OFS_BSP_BASE):ofs_n6001

MHZ = 650MHz

SEED = 0

EARLY = -fsycl-link=early

SIMULATE = $(CONFIG) -DFPGA_EMULATOR=1 -fsycl -fintelfpga -qactypes -Wall -Wextra -g -O0

SYNTHESIS = $(CONFIG) -Xsv -fsycl -fintelfpga -qactypes -DFPGA_HARDWARE=1 -Xshardware -Xsclock=$(MHZ) -Xsseed=$(SEED) -Xstarget=$(TARGET) -DIS_BSP -O3

RTLSIM = $(CONFIG) -Xsv -fsycl -fintelfpga -DFPGA_SIMULATOR=1 -Xssimulation #-Xstarget=$(TARGET) #-DIS_BSP -O3

CXX = $(CONFIG) -DCXXONLY -std=c++1y -g -O0 #-Wall

AGGSRC = main.cpp

sim:
	icpx $(CONFIG) $(SIMULATE) $(AGGSRC) -o build/join_sim
	build/join_sim > test.txt

rtl:
	icpx $(CONFIG) $(RTLSIM) $(AGGSRC) -o build/agg_rtl

fpga:
	icpx $(CONFIG) $(SYNTHESIS) $(AGGSRC) -o $(AGGF)

link:
	icpx $(CONFIG) $(SYNTHESIS) $(EARLY) $(AGGSRC) -o $(AGGH)

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

