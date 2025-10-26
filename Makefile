SRC	= $(shell find aosora-shiori -name '*.cpp' | sed -e 's@\(.*\)\.cpp@\1.o@g')
SRCDLL	= $(shell find aosora-shiori-dll -name '*.cpp' | sed -e 's@\(.*\)\.cpp@\1.o@g')
SRCSSTP	= $(shell find aosora-sstp -name '*.cpp' | sed -e 's@\(.*\)\.cpp@\1.o@g')
SRCANALYZER	= $(shell find aosora-analyzer -name '*.cpp' | sed -e 's@\(.*\)\.cpp@\1.o@g')

LIB_A	= aosora-shiori/libaosora.a
LIBRARY	= ninix/libaosora.so
PROGRAM_SSTP	= ninix/aosora-sstp
PROGRAM_ANALYZER	= ninix/aosora-analyzer

CXX = clang++
AR  = llvm-ar
#CXX = i686-w64-mingw32-g++
#AR	= i686-w64-mingw32-ar

CXXFLAGS	= -std=c++20 -O2 -I aosora-shiori -fPIC $(shell pkg-config --cflags openssl jsoncpp)
#CXXFLAGS	= -std=c++20 -O2 -I aosora-shiori
LDFLAGS	= $(shell pkg-config --libs openssl jsoncpp)
SO_LDFLAGS	= -shared $(LDFLAGS)
#SO_LDFLAGS	= -shared -static-libgcc -static-libstdc++

.PHONY: all clean

.SUFFIXES: .cpp .o

all: $(LIBRARY) $(PROGRAM_SSTP) $(PROGRAM_ANALYZER)

$(LIBRARY): $(SRCDLL) $(LIB_A)
	$(CXX) $(SO_LDFLAGS) -o $@ $^

$(PROGRAM_ANALYZER): $(SRCANALYZER) $(LIB_A)
	$(CXX) $(LDFLAGS) -o $@ $^

$(PROGRAM_SSTP): $(SRCSSTP) $(LIB_A)
	$(CXX) $(LDFLAGS) -o $@ $^

$(LIB_A): $(SRC)
	$(AR) cr $@ $^

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $^

clean:
	$(RM) $(LIBRARY) $(PROGRAM_SSTP) $(PROGRAM_ANALYZER) $(LIB_A) $(SRC) $(SRCDLL) $(SRCSSTP) $(SRCANALYZER)
