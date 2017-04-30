<!-- READEME.md -->
# izix
#### A hobby \*nix OS ([Homepage](https://izix.izzette.com/))
This is a small kernel for `i386` aiming to provide a POSIX compatible API.
Currently, izix only boots with [izixboot](https://github.com/Izzette/izixboot).
This is still a very young project which is nowhere near ready for userland.

## Building:
### Tool-chain:
* First you will need to build GNU binutils for `i386-elf` (or better).  I used version 2.28 and my configure options were `--target="i686-elf" --prefix="/opt/i686-elf" --with-sysroot --disable-nls --disable-werror`
* Next you will need to build GCC for `i386-elf` (or better) with a valid `crtbegin.o` and `crtend.o`.  I used version 6.3.0 and my configure options were `--target="i686-elf" --prefix="/opt/i686-elf" --with-as="/opt/i686-elf/bin/i686-elf-as" --with-ld="/opt/i686-elf/bin/i686-elf-ld" --without-headers --enable-languages="c,c++" --disable-nls --disable-werror`
* That's it!  Because it's all freestanding right now, we just need that stage 1 tool-chain.
### The build:
Just run `make` (well, sorta).
You'll have to set the appropriate `PATH` and provide the appropriate `CC` and `AR` on the command line like this: `PATH="/opt/i686-elf/bin:$PATH" make CC=i686-elf-gcc AR=i686-elf-ar`
This will produce the `izix.kernel` ELF object.
The `strip` make target will remove unnecessary symbols form `izix.kernel`, but don't forget to specify the correct `STRIP` on the command line like this: `make STRIP=/opt/i686-elf/bin/i686-elf-strip strip`.

## Installing:
See [izixboot/README.md#installing](https://github.com/Izzette/izixboot/blob/master/README.md#installing) for details.

## Hacking:
### Debugging:
* You can generate a debug symbol file at `izix.debug` with the `debug` make target, but don't forget to specify the correct `OBJCOPY` on the command line like this: `make OBJCOPY=/opt/i686-elf/bin/i686-elf-objcopy`. You must run this target before any stripping you want to do on `izix.kernel`.  You can load the symbols with GDB like this: `gdb -s izix.debug`.
* You can debug with QEMU by opening a GDB server and stopping the CPU at startup by specifying the `-s` and `-S` command line options to your `qemu-system-<target>` where target is either i386 or x86\_64.
* A convenient GDB script has been placed in `debug/gdb.x` which will connect to a GDB server at port `1234`, set the architecture to `i386:intel`, and drop you off at `0x9000` in `_start`.
* Bringing it all together, I suggest the following commands:
```
# Build your kernel image with debug symbols by setting CFLAGS and ASFLAGS to "-O0 -ggdb -Wall -Wextra" ...
make OBJCOPY=/opt/i686-elf/bin/i686-elf-objcopy debug  # Generate izix.debug symbol file.
make OBJCOPY=/opt/i686-elf/bin/i686-elf-objcopy strip  # Strip the debug symbols that are no longer needed.
# Install your kernel image with izixboot to a disk image ...
qemu-system-i386 -hda path/to/your_disk_image -s -S &> /dev/null &  # Start QEMU with a GDB server at localhost:1234 and stop the CPU at startup.
gdb -x debug/gdb.x -s izix.debug  # Access the GDB server, set the correct architecture, break at 0x9000, and load the debuging symbols.
```

## License:
```
izix --  A hobby *nix OS
Copyright (C) 2017  Isabell Cowan

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
```
<!-- vim: set ts=2 sw=2 et syn=markdown: -->
