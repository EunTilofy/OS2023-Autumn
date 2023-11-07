SRCS = $(shell find . -name *"*.S") $(shell find . -name "*.c")
OBJS = $(addsuffix .o, $(basename $(SRCS)))

all: $(OBJS)

%.o: %.S
	@echo CC $< $@
	@$(GCC) $(CFLAG) -c $< -o $@
	

%.o: %.c
	@echo CC $< $@
	@$(GCC) $(CFLAG) -c $< -o $@

clean:
	-@rm *.o 2>/dev/null
