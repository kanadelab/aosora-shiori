SRC	= $(shell find aosora-shiori -name '*.cpp' | sed -e 's@\(.*\)\.cpp@\1.o@g')
SRCDLL	= $(shell find aosora-shiori-dll -name '*.cpp' | sed -e 's@\(.*\)\.cpp@\1.o@g')
SRCSSTP	= $(shell find aosora-sstp -name '*.cpp' | sed -e 's@\(.*\)\.cpp@\1.o@g')

LIBRARY	= aosora.dll
PROGRAM	= aosora-sstp-posix

CXX = clang++
AR  = llvm-ar
#CXX = i686-w64-mingw32-g++
#AR	= i686-w64-mingw32-ar

CXXFLAGS	= -std=c++20 -O2 -I aosora-shiori -fPIC
#CXXFLAGS	= -std=c++20 -O2 -I aosora-shiori
SO_LDFLAGS	= -shared $(shell pkg-config -libs openssl)
#SO_LDFLAGS	= -shared -static-libgcc -static-libstdc++
LDFLAGS	= $(shell pkg-config -libs openssl)

.PHONY: all clean

.SUFFIXES: .cpp .o

all: $(LIBRARY) $(PROGRAM)

$(LIBRARY): $(SRCDLL) libaosora.a
	$(CXX) $(SO_LDFLAGS) -o $@ $^

$(PROGRAM): $(SRCSSTP) libaosora.a
	$(CXX) $(LDFLAGS) -o $@ $^

libaosora.a: $(SRC)
	$(AR) cr $@ $^

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $^

clean:
	$(RM) $(LIBRARY) $(PROGRAM) libaosora.a $(SRC) $(SRCDLL) $(SRCSSTP)
