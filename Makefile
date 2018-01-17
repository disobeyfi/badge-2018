OBJFLAGS  = -O0 -std=c99 -g -mthumb -mcpu=cortex-m0 -msoft-float -Wextra -Wshadow  \
            -Wimplicit-function-declaration -Wredundant-decls  \
            -fno-common -ffunction-sections -fdata-sections  -MD \
            -Wall -Wundef -DSTM32F0 -Ilibopencm3/include \
            -Wall -Wextra -Wundef -Wdouble-promotion -Wconversion -Wpadded
PREFIX    = arm-none-eabi-
LINKFLAGS = -lm --static -nostartfiles -Tbuildstuff/nucleo-f072rb.ld -mthumb -mcpu=cortex-m0 \
            -msoft-float  -Wl,--gc-sections -Llibopencm3/lib \
            -lopencm3_stm32f0 -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group  \

BADGENAME0=  badge-user-yellow
BADGENAME1=badge-system-yellow
BADGENAME2=badge-hacker-yellow
BADGENAME3=  badge-user-red
BADGENAME4=badge-system-red
BADGENAME5=badge-hacker-red
BADGENAME6=  badge-user-green
BADGENAME7=badge-system-green
BADGENAME8=badge-hacker-green
BADGENAME9= badge-admin-yellow

BADGENAME_PROTO2=  badge-proto2


BADGEBINS = $(BADGENAME0).bin $(BADGENAME1).bin $(BADGENAME2).bin $(BADGENAME3).bin $(BADGENAME4).bin $(BADGENAME5).bin $(BADGENAME6).bin $(BADGENAME7).bin $(BADGENAME8).bin $(BADGENAME9).bin $(BADGENAME_PROTO2).bin

all: $(BADGEBINS)

%.o: %.c
	$(PREFIX)gcc $(OBJFLAGS)  -o $@ -c $<

%.elf: %.o
	$(PREFIX)gcc  -Wl,-Map=$@.map  $< -o $@ $(LINKFLAGS)

BADGEDEPS = main.o debugf.o utils.o buttons.o gamepad.o fondling.o animations.o keyboard.o keys/badgekey.o keys/sha256.o

$(BADGENAME0).elf:	 	$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE0 -DLEDYELLOW
$(BADGENAME1).elf:	 	$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE1 -DLEDYELLOW
$(BADGENAME2).elf:	 	$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE2 -DLEDYELLOW
$(BADGENAME3).elf:	 	$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE3 -DLEDRED
$(BADGENAME4).elf:	 	$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE4 -DLEDRED
$(BADGENAME5).elf:	 	$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE5 -DLEDRED
$(BADGENAME6).elf:	 	$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE6 -DLEDGREEN
$(BADGENAME7).elf: 		$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE7 -DLEDGREEN
$(BADGENAME8).elf: 		$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE8 -DLEDGREEN
$(BADGENAME9).elf: 		$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE9 -DLEDYELLOW
$(BADGENAME_PROTO2).elf: 	$(BADGEDEPS) keys/keydata.c lights.c
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS) $(OBJFLAGS) -DBADGETYPE0 -DLEDPROTO2



proto1test.elf: proto1test.o debugf.o utils.o buttons.o lights.o
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS)

proto2test.elf: proto2test.o debugf.o utils.o buttons.o lights.o
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS)

pwmtest.elf: pwmtest.o debugf.o utils.o buttons.o lights.o
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS)

usbtest.elf: usbtest.o debugf.o utils.o buttons.o lights.o
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS)

taranis.elf: taranis.o debugf.o utils.o buttons.o lights.o
	$(PREFIX)gcc  -Wl,-Map=$@.map  $^ -o $@ $(LINKFLAGS)





clean:
	rm *.o *.elf *.bin *.map *.d keys/*.o keys/*.d

%.bin: %.elf
	$(PREFIX)objcopy -Obinary $< $@

%.stlink-flash: %.bin
	/usr/local/bin/st-flash reset
	/usr/local/bin/st-flash write $< 0x8000000


