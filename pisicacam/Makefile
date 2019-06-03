
CXX=${CROSS_COMPILE}g++

src = $(wildcard *.cpp)
obj = $(src:.cpp=.o)

LDFLAGS =  -lcurl -lpthread -lpng -ljpeg -lv4l2

CXXFLAGS +=  -I${SYSFS}/include -I${SYSFS}/usr/include -I${SYSFS}  -L${SYSFS}/lib -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -O0 -DDEBUG -gdwarf-3


pisicacam: $(obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) pisicacam


