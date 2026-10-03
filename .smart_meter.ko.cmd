savedcmd_smart_meter.ko := ld -r -m elf_x86_64 -z noexecstack --build-id=sha1  -T /usr/src/WSL2-Linux-Kernel/scripts/module.lds -o smart_meter.ko smart_meter.o smart_meter.mod.o .module-common.o
