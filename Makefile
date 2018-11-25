###########################################################################
# This file is part of HLPP_WP.
#
# HLPP_WP is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# HLPP_WP is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with HLPP_WP.  If not, see <http://www.gnu.org/licenses/>.
#
# Author: Fabio Lucattini <fabiottini@gmail.com>
# Date:   Settember 2016
###########################################################################

FF_ROOT		= $(HOME)/fastflow

##CXX			= /usr/local/gnu/bin/g++
#CXX			= /opt/gnu/bin/g++ #g++
CXX			= g++

INCLUDES	= -I $(FF_ROOT) -I $(PWD)/include #./include #$(PWD)/include
INCLUDE_LLM = -I $(PWD)/lmfit-5.1/lib
#CXXFLAGS  	= -std=c++0x
CXXFLAGS  	= -std=c++11
OBJ			= obj

FLAGS			= ##-g -Wall -pedantic
LDFLAGS 	= -pthread
MACROS		= -DNDEBUG -DBOUNDED_QUEUE -DMAGIC_CONSTANT_IN=20000 -DMAGIC_CONSTANT_OUT=2000 -DFAKE_ENTRIES=50000000 -DPIANOSA
OPTFLAGS	= -v #-O3 -finline-functions

TARGETS		= test_kp test_wf trade_kp trade_wf fixed_generator real_generator tuningTicks

.DEFAULT_GOAL := all
.PHONY: all clean cleanall
.SUFFIXES: .cpp


test: test/test.cpp
	$(CXX) -g $(CXXFLAGS) $(FLAGS) $(INCLUDES) $(MACROS)  test/test.cpp  -o bin/test $(LDFLAGS)


### ACTIVE ###
test_wf_active_sw: src/test_wf_active_sw.cpp include/Win_WF_sw.hpp
	$(CXX) -g $(CXXFLAGS) $(FLAGS) $(INCLUDES) $(INCLUDE_LLM) $(MACROS)  src/test_wf_active_sw.cpp -o bin/test_wf_active_sw $(LDFLAGS)   -llmfit

### AGNOSTIC ###
test_wf_agnostic_sw: src/test_wf_agnostic_sw.cpp include/Win_WF_sw.hpp
	$(CXX)  -g $(CXXFLAGS) $(FLAGS) $(INCLUDES) $(MACROS) src/test_wf_agnostic_sw.cpp -o bin/test_wf_agnostic_sw   $(LDFLAGS) -llmfit

test_wf_agnostic_sbt: src/test_wf_agnostic_sbt.cpp include/Win_WF_sw.hpp
	$(CXX)  -g $(CXXFLAGS) $(FLAGS) $(INCLUDES) $(MACROS) src/test_wf_agnostic_sbt.cpp -o bin/test_wf_agnostic_sbt   $(LDFLAGS) -llmfit

### VERSION 1 ###
test_wf_tb_v1: src/test_wf_tb_v1.cpp
	$(CXX)  -g $(CXXFLAGS) $(FLAGS) $(INCLUDES) $(MACROS) -o bin/test_wf_tb_v1 src/test_wf_tb_v1.cpp $(LDFLAGS)

testFeedback: src/testFeedback.cpp
##	$(CXX)  -g $(CXXFLAGS) $(INCLUDES) $(MACROS) $(OPTFLAGS) -o bin/testFeedback src/testFeedback.cpp $(LDFLAGS)
	$(CXX)  -gstabs -g $(CXXFLAGS) $(FLAGS) $(INCLUDES) $(MACROS)  -o bin/testFeedback src/testFeedback.cpp $(LDFLAGS)

all: $(TARGETS)

clean:
	rm -f bin/*

cleanall:
	\rm -f bin/*.o bin/*~
