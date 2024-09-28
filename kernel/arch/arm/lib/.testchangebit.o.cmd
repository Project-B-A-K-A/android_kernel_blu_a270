cmd_arch/arm/lib/testchangebit.o := arm-linux-androideabi-gcc -Wp,-MD,arch/arm/lib/.testchangebit.o.d  -nostdinc -isystem /home/ubuntu/Fproject/S8321AP_BLU_OPEN/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin/../lib/gcc/arm-linux-androideabi/4.6.x-google/include -I/home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include -Iarch/arm/include/generated -Iinclude  -include /home/ubuntu/Project/KernelSource/alps/kernel/include/linux/kconfig.h -I../mediatek/custom///common -I../mediatek/platform/mt6572/kernel/core/include/ -I../mediatek/custom/blu_open/common -I../mediatek/kernel/include/ -I../mediatek/custom/out/blu_open/kernel/dct/ -I../mediatek/custom/out/blu_open/kernel/alsps/ -I../mediatek/custom/out/blu_open/kernel/vibrator/ -I../mediatek/custom/out/blu_open/kernel/battery/ -I../mediatek/custom/out/blu_open/kernel/accelerometer/ -I../mediatek/custom/out/blu_open/kernel/core/ -I../mediatek/custom/out/blu_open/kernel/flashlight/ -I../mediatek/custom/out/blu_open/kernel/imgsensor/ -I../mediatek/custom/out/blu_open/kernel/rtc/ -I../mediatek/custom/out/blu_open/kernel/kpd/ -I../mediatek/custom/out/blu_open/kernel/leds/ -I../mediatek/custom/out/blu_open/kernel/camera/ -I../mediatek/custom/out/blu_open/kernel/headset/ -I../mediatek/custom/out/blu_open/kernel/usb/ -I../mediatek/custom/out/blu_open/kernel/sound/ -I../mediatek/custom/out/blu_open/kernel/sound/inc/ -I../mediatek/custom/out/blu_open/kernel/alsps/inc/ -I../mediatek/custom/out/blu_open/kernel/barometer/inc/ -I../mediatek/custom/out/blu_open/kernel/accelerometer/inc/ -I../mediatek/custom/out/blu_open/kernel/eeprom/ -I../mediatek/custom/out/blu_open/kernel/eeprom/inc/ -I../mediatek/custom/out/blu_open/kernel/flashlight/inc/ -I../mediatek/custom/out/blu_open/kernel/imgsensor/inc/ -I../mediatek/custom/out/blu_open/kernel/ssw/inc/ -I../mediatek/custom/out/blu_open/kernel/ssw/ -I../mediatek/custom/out/blu_open/kernel/ccci/ -I../mediatek/custom/out/blu_open/kernel/cam_cal/ -I../mediatek/custom/out/blu_open/kernel/cam_cal/inc/ -I../mediatek/custom/out/blu_open/kernel/hdmi/inc/ -I../mediatek/custom/out/blu_open/kernel/leds/inc/ -I../mediatek/custom/out/blu_open/kernel/./ -I../mediatek/custom/out/blu_open/kernel/lens/ -I../mediatek/custom/out/blu_open/kernel/lens/inc/ -I../mediatek/custom/out/blu_open/kernel/jogball/inc/ -I../mediatek/custom/out/blu_open/kernel/magnetometer/inc/ -I../mediatek/custom/out/blu_open/kernel/gyroscope/inc/ -I../mediatek/custom/out/blu_open/kernel/lcm/ -I../mediatek/custom/out/blu_open/kernel/lcm/inc/ -I../mediatek/custom/out/blu_open/kernel/touchpanel/ -I../mediatek/custom/out/blu_open/hal/imgsensor/ -I../mediatek/custom/out/blu_open/hal/audioflinger/ -I../mediatek/custom/out/blu_open/hal/camera/ -I../mediatek/custom/out/blu_open/hal/inc/ -I../mediatek/custom/out/blu_open/hal/lens/ -I../mediatek/custom/out/blu_open/hal/sensors/ -I../mediatek/custom/out/blu_open/hal/camera/inc/ -I../mediatek/custom/out/blu_open/hal/eeprom/ -I../mediatek/custom/out/blu_open/hal/ant/ -I../mediatek/custom/out/blu_open/hal/flashlight/ -I../mediatek/custom/out/blu_open/hal/vt/ -I../mediatek/custom/out/blu_open/hal/matv/ -I../mediatek/custom/out/blu_open/hal/cam_cal/ -I../mediatek/custom/out/blu_open/hal/combo/ -I../mediatek/custom/out/blu_open/hal/fmradio/ -I../mediatek/custom/out/blu_open/hal/bluetooth/ -I../mediatek/custom/out/blu_open/common -D__KERNEL__   -mlittle-endian -DMODEM_3G -Imediatek/platform/mt6572/kernel/core/include -D__ASSEMBLY__   -mabi=aapcs-linux -mno-thumb-interwork  -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float -gdwarf-2        -c -o arch/arm/lib/testchangebit.o arch/arm/lib/testchangebit.S

source_arch/arm/lib/testchangebit.o := arch/arm/lib/testchangebit.S

deps_arch/arm/lib/testchangebit.o := \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/linkage.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/mt/sched/monitor.h) \
    $(wildcard include/config/smp.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/hwcap.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
    $(wildcard include/config/cpu/use/domains.h) \
  arch/arm/lib/bitops.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/unwind.h \
    $(wildcard include/config/arm/unwind.h) \

arch/arm/lib/testchangebit.o: $(deps_arch/arm/lib/testchangebit.o)

$(deps_arch/arm/lib/testchangebit.o):
