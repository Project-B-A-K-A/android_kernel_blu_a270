cmd_arch/arm/mm/proc-v7.o := arm-linux-androideabi-gcc -Wp,-MD,arch/arm/mm/.proc-v7.o.d  -nostdinc -isystem /home/ubuntu/Fproject/S8321AP_BLU_OPEN/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin/../lib/gcc/arm-linux-androideabi/4.6.x-google/include -I/home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include -Iarch/arm/include/generated -Iinclude  -include /home/ubuntu/Project/KernelSource/alps/kernel/include/linux/kconfig.h -I../mediatek/custom///common -I../mediatek/platform/mt6572/kernel/core/include/ -I../mediatek/custom/blu_open/common -I../mediatek/kernel/include/ -I../mediatek/custom/out/blu_open/kernel/dct/ -I../mediatek/custom/out/blu_open/kernel/alsps/ -I../mediatek/custom/out/blu_open/kernel/vibrator/ -I../mediatek/custom/out/blu_open/kernel/battery/ -I../mediatek/custom/out/blu_open/kernel/accelerometer/ -I../mediatek/custom/out/blu_open/kernel/core/ -I../mediatek/custom/out/blu_open/kernel/flashlight/ -I../mediatek/custom/out/blu_open/kernel/imgsensor/ -I../mediatek/custom/out/blu_open/kernel/rtc/ -I../mediatek/custom/out/blu_open/kernel/kpd/ -I../mediatek/custom/out/blu_open/kernel/leds/ -I../mediatek/custom/out/blu_open/kernel/camera/ -I../mediatek/custom/out/blu_open/kernel/headset/ -I../mediatek/custom/out/blu_open/kernel/usb/ -I../mediatek/custom/out/blu_open/kernel/sound/ -I../mediatek/custom/out/blu_open/kernel/sound/inc/ -I../mediatek/custom/out/blu_open/kernel/alsps/inc/ -I../mediatek/custom/out/blu_open/kernel/barometer/inc/ -I../mediatek/custom/out/blu_open/kernel/accelerometer/inc/ -I../mediatek/custom/out/blu_open/kernel/eeprom/ -I../mediatek/custom/out/blu_open/kernel/eeprom/inc/ -I../mediatek/custom/out/blu_open/kernel/flashlight/inc/ -I../mediatek/custom/out/blu_open/kernel/imgsensor/inc/ -I../mediatek/custom/out/blu_open/kernel/ssw/inc/ -I../mediatek/custom/out/blu_open/kernel/ssw/ -I../mediatek/custom/out/blu_open/kernel/ccci/ -I../mediatek/custom/out/blu_open/kernel/cam_cal/ -I../mediatek/custom/out/blu_open/kernel/cam_cal/inc/ -I../mediatek/custom/out/blu_open/kernel/hdmi/inc/ -I../mediatek/custom/out/blu_open/kernel/leds/inc/ -I../mediatek/custom/out/blu_open/kernel/./ -I../mediatek/custom/out/blu_open/kernel/lens/ -I../mediatek/custom/out/blu_open/kernel/lens/inc/ -I../mediatek/custom/out/blu_open/kernel/jogball/inc/ -I../mediatek/custom/out/blu_open/kernel/magnetometer/inc/ -I../mediatek/custom/out/blu_open/kernel/gyroscope/inc/ -I../mediatek/custom/out/blu_open/kernel/lcm/ -I../mediatek/custom/out/blu_open/kernel/lcm/inc/ -I../mediatek/custom/out/blu_open/kernel/touchpanel/ -I../mediatek/custom/out/blu_open/hal/imgsensor/ -I../mediatek/custom/out/blu_open/hal/audioflinger/ -I../mediatek/custom/out/blu_open/hal/camera/ -I../mediatek/custom/out/blu_open/hal/inc/ -I../mediatek/custom/out/blu_open/hal/lens/ -I../mediatek/custom/out/blu_open/hal/sensors/ -I../mediatek/custom/out/blu_open/hal/camera/inc/ -I../mediatek/custom/out/blu_open/hal/eeprom/ -I../mediatek/custom/out/blu_open/hal/ant/ -I../mediatek/custom/out/blu_open/hal/flashlight/ -I../mediatek/custom/out/blu_open/hal/vt/ -I../mediatek/custom/out/blu_open/hal/matv/ -I../mediatek/custom/out/blu_open/hal/cam_cal/ -I../mediatek/custom/out/blu_open/hal/combo/ -I../mediatek/custom/out/blu_open/hal/fmradio/ -I../mediatek/custom/out/blu_open/hal/bluetooth/ -I../mediatek/custom/out/blu_open/common -D__KERNEL__   -mlittle-endian -DMODEM_3G -Imediatek/platform/mt6572/kernel/core/include -D__ASSEMBLY__   -mabi=aapcs-linux -mno-thumb-interwork  -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float -gdwarf-2     -Wa,-march=armv7-a   -c -o arch/arm/mm/proc-v7.o arch/arm/mm/proc-v7.S

source_arch/arm/mm/proc-v7.o := arch/arm/mm/proc-v7.S

deps_arch/arm/mm/proc-v7.o := \
    $(wildcard include/config/arm/lpae.h) \
    $(wildcard include/config/arm/cpu/suspend.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/arm/errata/430973.h) \
    $(wildcard include/config/arm/errata/458693.h) \
    $(wildcard include/config/arm/errata/460075.h) \
    $(wildcard include/config/arm/errata/742230.h) \
    $(wildcard include/config/arm/errata/742231.h) \
    $(wildcard include/config/arm/errata/743622.h) \
    $(wildcard include/config/arm/errata/751472.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/arm/thumbee.h) \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/swp/emulate.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/linux/linkage.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/linkage.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/mt/sched/monitor.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/hwcap.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
    $(wildcard include/config/cpu/use/domains.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/asm-offsets.h \
  include/generated/asm-offsets.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/pgtable-hwdef.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/pgtable-2level-hwdef.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/pgtable.h \
    $(wildcard include/config/arm/dma/mem/bufferable.h) \
    $(wildcard include/config/highpte.h) \
  include/linux/const.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/proc-fns.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/glue-proc.h \
    $(wildcard include/config/cpu/arm610.h) \
    $(wildcard include/config/cpu/arm7tdmi.h) \
    $(wildcard include/config/cpu/arm710.h) \
    $(wildcard include/config/cpu/arm720t.h) \
    $(wildcard include/config/cpu/arm740t.h) \
    $(wildcard include/config/cpu/arm9tdmi.h) \
    $(wildcard include/config/cpu/arm920t.h) \
    $(wildcard include/config/cpu/arm922t.h) \
    $(wildcard include/config/cpu/fa526.h) \
    $(wildcard include/config/cpu/arm925t.h) \
    $(wildcard include/config/cpu/arm926t.h) \
    $(wildcard include/config/cpu/arm940t.h) \
    $(wildcard include/config/cpu/arm946e.h) \
    $(wildcard include/config/cpu/sa110.h) \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/arm1020.h) \
    $(wildcard include/config/cpu/arm1020e.h) \
    $(wildcard include/config/cpu/arm1022.h) \
    $(wildcard include/config/cpu/arm1026.h) \
    $(wildcard include/config/cpu/xscale.h) \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/mohawk.h) \
    $(wildcard include/config/cpu/v6.h) \
    $(wildcard include/config/cpu/v6k.h) \
    $(wildcard include/config/cpu/v7.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/glue.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/page.h \
    $(wildcard include/config/cpu/copy/v3.h) \
    $(wildcard include/config/cpu/copy/v4wt.h) \
    $(wildcard include/config/cpu/copy/v4wb.h) \
    $(wildcard include/config/cpu/copy/feroceon.h) \
    $(wildcard include/config/cpu/copy/fa.h) \
    $(wildcard include/config/cpu/copy/v6.h) \
    $(wildcard include/config/have/arch/pfn/valid.h) \
  include/asm-generic/getorder.h \
  include/asm-generic/pgtable-nopud.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/memory.h \
    $(wildcard include/config/need/mach/memory/h.h) \
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/have/tcm.h) \
    $(wildcard include/config/arm/patch/phys/virt.h) \
    $(wildcard include/config/phys/offset.h) \
  arch/arm/include/generated/asm/sizes.h \
  include/asm-generic/sizes.h \
  ../mediatek/platform/mt6572/kernel/core/include/mach/memory.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/pgtable-2level.h \
  arch/arm/mm/proc-macros.S \
    $(wildcard include/config/cpu/dcache/writethrough.h) \
    $(wildcard include/config/pm/sleep.h) \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/thread_info.h \
  /home/ubuntu/Project/KernelSource/alps/kernel/arch/arm/include/asm/fpstate.h \
    $(wildcard include/config/vfpv3.h) \
    $(wildcard include/config/iwmmxt.h) \
  arch/arm/mm/proc-v7-2level.S \
    $(wildcard include/config/arm/errata/754322.h) \

arch/arm/mm/proc-v7.o: $(deps_arch/arm/mm/proc-v7.o)

$(deps_arch/arm/mm/proc-v7.o):
