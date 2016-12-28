EXECUTABLE = httpserver 

SOURCE :=$(wildcard *.cpp) $(wildcard *.cc)
OBJS := $(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(SOURCE))) 
DEPS := $(patsubst %.o,%.d,$(OBJS)) 

.PHONY : everything deps objs clean rebuild 

everything : $(EXECUTABLE) 

deps : $(DEPS) 
objs : $(OBJS) 

clean : 
	-rm *.o 
	-rm *.d 
	-rm $(EXECUTABLE) 

rebuild: clean everything 


-include $(DEPS) 

$(EXECUTABLE) : $(OBJS) 
	$(CXX) $(LDFLAGS) -o $(EXECUTABLE) $(OBJS) $(addprefix -l,$(LIBS))
