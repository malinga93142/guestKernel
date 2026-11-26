CXX      := g++
CXXFLAGS := -g -std=gnu++23 -O0 -Wall -Wextra -fsanitize=address,undefined -MMD -MP

KCXX     := x86_64-linux-gnu-g++
KCXXFLAGS := -std=gnu++11 -O0 -Wall -Wextra \
             -m64 -ffreestanding -fno-exceptions -fno-rtti \
             -fno-pic -fno-pie -fno-stack-protector \
             -fno-threadsafe-statics -fno-use-cxa-atexit \
             -mno-red-zone -mgeneral-regs-only \
             -MMD -MP -mcmodel=kernel

AS       := x86_64-linux-gnu-g++
ASFLAGS  := $(KCXXFLAGS) -x assembler-with-cpp
LD       := x86_64-linux-gnu-ld
LDFLAGS  := -m elf_x86_64 -T kernel.ld -nostdlib
OBJCOPY  := objcopy
OBJDUMP  := objdump

HOST     := host
GUEST ?= kernel

simple_OBJS := boot.o
kernel_OBJS := boot.o kernel.o isr.o exceptions.o idt.o
OBJS := $($(GUEST)_OBJS)

DUMPS ?= i8086/0x0/0x40

ifeq ($(V),1)
  Q :=
  E := @true
else
  Q := @
  E := @echo
endif

.PHONY: all run objdump clean
.SECONDARY:

all: $(HOST) $(GUEST).bin

$(HOST): $(HOST).cpp
	$(E) "[CXX] $<"
	$(Q)$(CXX) $(CXXFLAGS) $< -o $@

%.o: %.S
	$(E) "[AS] $<"
	$(Q)$(AS) $(ASFLAGS) -c $< -o $@

%.o: %.cpp
	$(E) "[KCXX] $<"
	$(Q)$(KCXX) $(KCXXFLAGS) -c $< -o $@

$(GUEST).elf: $(OBJS) kernel.ld
	$(E) "[LD] $@"
	$(Q)$(LD) $(LDFLAGS) $(OBJS) -o $@

%.bin: %.elf
	$(E) "[OBJCOPY] $@"
	$(Q)$(OBJCOPY) -O binary $< $@

run: all
	./$(HOST) $(GUEST).bin

define dump_range
	$(E) "[OBJDUMP] $(word 1,$(subst /, ,$1)) $(word 2,$(subst /, ,$1))-$(word 3,$(subst /, ,$1))"
	$(Q)$(OBJDUMP) -D -b binary -m$(word 1,$(subst /, ,$1)) -M intel \
		--start-address=$(word 2,$(subst /, ,$1)) \
		--stop-address=$(word 3,$(subst /, ,$1)) $(GUEST).bin

endef

objdump: $(GUEST).bin
	$(foreach d,$(DUMPS),$(call dump_range,$(d)))

clean:
	$(E) "[REMOVE] ..."
	$(Q)rm -f $(HOST) *.o *.elf *.bin *.d *.ko

-include $(wildcard *.d)
