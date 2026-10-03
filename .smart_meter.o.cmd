savedcmd_smart_meter.o := ld -m elf_x86_64 -z noexecstack   -r -o smart_meter.o @smart_meter.mod  ; /usr/src/WSL2-Linux-Kernel/tools/objtool/objtool --hacks=jump_label --hacks=noinstr --hacks=skylake --ibt --retpoline --rethunk --stackval --static-call --uaccess --prefix=16  --link  --module smart_meter.o

smart_meter.o: $(wildcard /usr/src/WSL2-Linux-Kernel/tools/objtool/objtool)
