# You shouldn't need to change anything below this point

SOURCE :=$(wildcard *.cpp) 
OBJS := $(patsubst %.cpp,%.o,$(SOURCE)) 
DEPS := $(patsubst %.o,%.d,$(OBJS)) 
MISSING_DEPS := $(filter-out $(wildcard $(DEPS)),$(DEPS)) 

.PHONY : everything deps objs clean  
everything: libbbt.a

deps : $(DEPS) 
objs : $(OBJS) 

clean :
	rm *.o 
	rm *.d 
	rm libbbt.a


-include $(DEPS) 

libbbt.a : $(OBJS) 
	$(AR) rcs $@ $(OBJS) 
