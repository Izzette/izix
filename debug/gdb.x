target remote localhost:1234
set architecture i386:intel
tbreak *0x9000
continue
