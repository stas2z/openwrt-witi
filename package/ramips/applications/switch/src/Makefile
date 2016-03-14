EXEC = switch

ifdef CONFIG_RALINK_MT7620
SRC = switch_gsw.c
CFLAGS += -DCONFIG_RALINK_MT7620
endif

ifdef CONFIG_RALINK_MT7621
SRC = switch_gsw.c
CFLAGS += -DCONFIG_RALINK_MT7621
endif

ifdef CONFIG_RALINK_MT7628
SRC = switch.c
CFLAGS += -DCONFIG_RALINK_MT7628
endif

ifdef CONFIG_MT7530_GSW
SRC = switch_gsw.c
CFLAGS += -DCONFIG_MT7530_GSW
endif

all: $(EXEC)

switch: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

romfs:
	$(ROMFSINST) /sbin/switch

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

