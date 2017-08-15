Kernel Experiments
=======================

Playing with QEMU and creating a new OS. Minimal one.

We need to first build a cross compiler, since we don't have a bootstrapped OS yet. So we need to target some platform, this is the platform that the cross compiler will be compiling to (programs that the compiler produces). However the compiler will be built on `x86_64` and hosted on `x86_64`.

Our target platform will be `i686-elf`.

The nix stdenv already provides a compatible GCC and binutils installed. Actually it is stated that the stdenv provides the gcc with C and C++ support, coreutils, findutils, diffutils, sed, grep, awk, tar, gzip, bzip and xz. Also make, bash and patch command. On Linux there's also patchelf. However it does not make mention of the binutils, so we should add that in. Actually I think the nix-shell DOES provide this. I wonder why binutils is not mentioned. But I guess it's usually part of the gcc suite. As ld and as is part of binutils and not the gcc package.

Ok so the environment is setup already, all we need is the GCC source code so we can build it.

We will also need bison, flex, gmp, mpfr, mpc, texinfo.

Turns out I need to read this:

https://nixos.org/nixpkgs/manual/#sec-cross-intro

See I don't know which things I need as source code, and which things I need as actual compiled binaries.

In nixpkgs, the three platforms of build, host and target has their names `buildPlatform`, `hostPlatform`, and `targetPlatform`.

We can get these as function parameters when called with `callPackage`, I'm not sure how this relates to a nix-shell however.

```
{ stdenv, buildPlatform, hostPlatform, targetPlatform, ... }:
```

The `buildPlatform` looks like this (strange how does this relate to `stdenv.system`):

```
{
  platform = { 
    kernelArch = "x86_64"; 
    kernelAutoModules = true; 
    kernelBaseConfig = "defconfig"; 
    kernelHeadersBaseConfig = "defconfig"; 
    kernelTarget = "bzImage"; 
    name = "pc"; 
    uboot = null;
  };
  system = "x86_64-linux";
}
```

The `hostPlatform` appears the same, which makes sense since we are intending to run the compiler on the same computer. I wonder if there's some sort of configuration that changes this. And even the `targetPlatform` is the same as well. I just found out this info from `nix-repl`.

So apparently nix expressions are actually meant to be cross compiled easily, but noone has written expressions like this yet. And we're meant to be using the `buildPackages` set, not just `pkgs` set. However the `callPackage` function will combine the 2 and perform magic on the 4 attributes of `buildInputs`, `nativeBuildInputs` `propagatedNativeBuildInputs` and `propagatedBuildInputs`. There is some work intended to revamp this further, so for now we mostly use the `buildInputs` attribute along with `callPackage`.

Also `buildInputs` are for dependencies that will exist at the `hostPlatform`. That also includes static libraries and not just shared objects (dynamic libraries). Remember all interpreted language source code is basically dynamic libraries with a `hostPlatform` interpreter.

So for example `autoreconfHook` should really be part of `nativeBuildInputs` and not `buildInputs` for building a autotools package, since all of the autotools is actually not used for the finally built package when running on the `hostPlatform`.

But is this true for `nix-shell`? Perhaps everything in `nix-shell` is intended for the build environment, so that should mean everything is in `buildInputs`, because the output of the `nix-shell` is just the build environment. But I'm not sure, then for `default.nix` it would be different.

We should actually define `NIX_DEBUG` as this will make `gcc` and `ld` print out the complete command passed to wrapped tools.

Apparently `<nixpkgs>` is also just a function where you can pass these optional parameters. Mostly they are `system`, `platform` and `crossSystem`. This means you can parameterise all of the nixpkgs package set to target a particular platform and to build on a particular platform. That's pretty cool! Instead of passing `system` and `platform`, apparently you can just pass `localSystem` that contains `system` and `platform`.

So you can do something like `import <nixpkgs> { system = 'x86_64-linux'; platform = {...} }`. The `platform` is based on several other properties similar to `hostPlatform` as described above. I'm not sure what the implications of this is. The `localSystem` I think is meant to be used instead of `system` and `platform`. If none of this is applied, it appears that the system and platform is defaulted on the current system, and this is specified with `nixpkgs/pkgs/top-level/impure.nix`.

I just discovered that `~/.nixpkgs/config.nix` is obsolete, and it is now recommended to go for `~/.config/nixpkgs/config.nix`.

Also there is nixpkgs overlays which extends the current set of nixpkgs, this might be useful for later. An overlay is intended to exist in `~/.config/nixpkgs/overlays/default.nix`. And some other special paths like `<nixpkgs-overlays>`.

So basically `crossSystem` should be null when one doesn't want to build for another platform. While `localSystem` is never null. I'm not sure if this is newer than my current nixpkgs however.

Also these 2 things which represent a build-deploy dichotomy is then transformed in the platform triple mentioned above.

OSDev mentions target triples as well which is related but not the same, it refers to target strings like `arch-vendor-os`. Which is usually `x86_64-unknown-linux`.

Derivations like `gccCross` are a legacy thing that will no longer be used in the future, they were originally used for situations where people need to build under the constraint of `build == host != target`. While the non-cross derivations were deriving under the constraint of `host == target`. While whether build was the same as host was basedd on whether one was using `.nativeDrv` or `.crossDrv`. I actually don't see the `gccCross` anymore, but I do see specific `gcc` for a given architecture like arm, and apple and mingw. I'm guessing those gcc will be useful for building like Windows applications using Mingw.

---

```
{ pkgs ? import <nixpkgs>  {
  crossSystem = {
    config = "";
    bigEndian = false;
    arch = "arm";
    float = "hard";
    fpu = "vfp";
    withTLS = true;
    libc = "glibc";
    platform = platforms.pc32;
    openssl.system = "linux-generic32";
    gcc = {
      arch = "";
      fpu = "vfp";
      float = "softfp";
      abi = "aapcs-linux";
    }
  }
} }:
```

You can get `buildPlatform`, `hostPlatform` and `targetPlatform` as parameters to your nix expression as it is passed by `callPackage`, and this means you can actually write a nix expression that builds inputs parameterised by the required build, host and target platform. This appears to be the goal of being able to have nix expressions that build packages for a multitude of platforms. But of course not all software will work on all platforms, but for those that will, they can take advantage of these 3 parameters. The docs also mention that the `targetPlatform` only works for certain build tools, such as gcc, as it was designed that a single build of gcc can only compile code for a single target platform (thus you need multiple builds to target different platforms). This is different from something like go which can target multiple platforms from a single tool. LLVM is also something was designed with cross compilation in mind, so a single toolchain can target multiple backends easily.


We should use the `localSystem` which describes `system` and `platform`. These 2 tells us where packages are built, while `crossSystem` is specifying where the platform is intended to run on. So just `localSystem` and `crossSystem` right?

We can leave out `localSystem` since we expect building on the current computer, so the only thing I need to fill is `crossSystem` which needs to point to: `i686-elf`.

At the pkgs/top-level/default.nix there is the usage of the function `lib.systems.elaborate`.

This is of course defined inside the lib/systems/default.nix. Here it is applied to teh definition of `crossSystem0` and `builtins.intersectAttrs { platform = null; } config // args.localSystem`. The latter is intended to allow setting the platform in the user's config file, this takes precedence over the inferred platform but not over an explicitly passed local system. Ok so that should be fine then...

So I think the precedence is like haskell where `lib.mapNullable` is applied on `lib.systems.elaborate`, thus it's like `map f`. Where the result is mapped onto the `crossSystem0`.

The definition of the function:

```
mapNullable = f: a: if isNull a then a else f a;
```

Oh so basically only apply f if a is not null. Then why is called map, since it's no really. It's more like apply if not null, not even map cause there's not functorial operation here.

It appears that this functionality is not available except at the master branch. So at release 17.09, this is not there yet. We need to use the 17.09 branch to get this inside our nix-repl.

It's easy, we just do `nix-repl`, the use `:l default.nix` inside the master branch of nixpkgs. Which we have!

What this function does is that it fills the blind spots, that is given a minimal specification of the crossSystem, it is possible for elaborate to fill what hasn't been specified, how it does this is still unknown.

The target triplet we want is `i686-elf` for the bare bones QEMU OS tutorial.

What exactly is this. Apparently, you can view the UNAMBIGUOUS target triplet for the current GCC build by doing: `gcc -dumpmachine`. Which shows for me: `x86_64-unknown-linux-gnu`. So we can see 3 things here, machine architecture, vendor and then OS.

```
machine-vendor-operatingsystem
```

The vendor field is mostly irrelevant, and is usually `pc` for 32 bit systems, while `unknown` or `none` for other systems. Except of course for things liek apple, which could be `aarch64-apple-darwin14`, which tells us the target triple for the latest iphones. And in MINGW, it tells us to use `x86_64-pc-mingw32`. So the exact target triplet seems to be magic strings that depend on the intended toolchain you want to make, there's no ultimate list of possible target triplets available.

In many cases the `vendor` can be left out, which leaves only `machine-operatingsystem`. So in the case of `i686-elf`, are we saying that `elf` is an operating system? Note that `operatingsystem` can be `linux-gnu`, so this does make it more ambiguos.

It appears `elf` could be a generic target well supported by the gcc compiler. So we can do things like:

```
i686-elf
x86_64-elf
arm-none-eabi
aarch64-none-elf
```

These are intended for "freestanding" programs such as bootloaders and kernels, which do not have a user space.

So we just need to find out how the `i686-elf` target triplet maps the `crossSystem` specification and elaborate function. I wonder if it is as simple as:

```
parse = lib.systems.elaborate {
  config = "i686-pc-none-elf";
  libc = "glibc";
  platform = platforms.pc32; 
}
```

Dunno.

So apparently the `config` comes from LLVM target format. Which is often more reliable and unambiguous. It appears that the equivalent target for LLVM is actually `i686-pc-none-elf`.

Trying with `lib.systems.elaborate { config = "i686-pc-none-elf"; }` results in some errors actually, it does try parse it actually though. Trying with `{ system = "i686-elf" }` also results in some problems as well. I'm not sure if the current parsing supports this.

---

Ok back to the drawing board. Let's read some more and investigate some more.

For a fully working cross compiler the following are needed:

1. A cross binutils - assembler, archiver, linker
2. A cross compiler - gcc
3. A cross C library - clib?

A toolchain is setup like this:

1. Build binutils with specified target platform
2. Build linux kernel headers for the target platform (is this relevant for freestanding programs that don't run on linux?)
3. Build a C only version for gcc with specified target platform
4. Build a C lib for the target platforms
5. Build a full gcc (this doesn't seem necessary for freestanding programs)

So an example of doing this is creating cross compiler for arm-linux platform wit uClibc. Right so this would create a compiler that could generate executables that would run on a linux OS running on arm hardware. The difference in libc, is probably not that important.

Ok so instead of using the nixpkgs expression for binutils, we just bring in and fetch the source ourselves. How do we fetch multiple sources? And then just set the configureFlags ourselves.

```
stdenv.mkDerivation {
  name = "binutils-2.16.1-arm";
  builder = ./builder.sh;
  src = fetchurl {
    url = http://ftp...tar.bz2;
    sha256 = "...";
  };
  inherit noSysDirs;
  configureFlags = "--target=arm-linux";
}
```

---

noSysDirs is defined inside the top-level/stage.nix that states that non-GNU/Linux OSes are currently "impure" platforms with their libc outside of the store. Thus gcc, gfortran must always look for files in standard directories like `/usr/include`... etc. This then uses a conditional that checks if the buildPlatform is equal to x86_64-freebsd, solaris. Hey but this doesn't include the linux for some reason?

Since this is true/false based on whether buildplatform is a particular thing, I think in my case, it should be false. SO it should be fine then.

Cool it appears that my binutils worked. There are no compilation errors, investigating the built result, it appears to have produced many i686-elf binaries available inside it's bin folder. Like:

```
i686-elf-addr2line*  i686-elf-elfedit*  i686-elf-nm*       i686-elf-readelf*
i686-elf-ar*         i686-elf-gprof*    i686-elf-objcopy*  i686-elf-size*
i686-elf-as*         i686-elf-ld*       i686-elf-objdump*  i686-elf-strings*
i686-elf-c++filt*    i686-elf-ld.bfd*   i686-elf-ranlib*   i686-elf-strip*
```

So this appears to be correct. The next issue is then gcc. And I wonder if this requires the extra library sources as well, perhaps they can be provided by nixpkgs. And also how do make use of these new binaries inside our next nix file. How do we import 1 nix expression as a package input to another nix expression? This is just stdenv. I think we may need to use the callPackage call.

Alternatively we add this expression into our config.nix and basically add these into our PATH.

With binutils version 2.28, we now need to use gcc 5.4.0.

The binaries made available inside the i686-elf-binutils-2.28 must be available in the PATH before compiling the GCC. This appears to actually build the libgcc and not the normal gcc?

It also builds into a new directory. That's fine. That's what I did before. Also since we're using nix-build, then this already targets a new directory.

So the target flags are pretty sane.

The next result is:

```
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
```

What is libgcc, it's a code generation library that all code generated by gcc needs to link to. However when creating freestanding executables, it turns out that you need to explicitly link with libgcc, this is why we need to make libgcc along with gcc.

So the first command `all-gcc` must create gcc, but then `all-target-libgcc` ends up making  a second one. The install targets must then try to actually install things in the OS. I don't think we need the install commands, instead we need to move it into our `$out` directory. Not sure how that's supposed to happen, perhaps with the prefix flag?

The result is a `libgcc.a` installed at the compiler prefix. Each different target tends to have their own libgcc.

Once built, you can the nlink to id by doing `-lgcc`. You don't need this unless you use `-nodefaultlibs` or `-nostdlib`.

For example:

```
i686-elf-gcc -T linker.ld -o myos.kernel -ffreestanding boot.o kernel.o -nostdlib -lgcc
```

The linking of libgcc occurs through gcc directly not through `ld`. This means that gcc knows where to find the `libgcc`.

Let me try without the necessary `make install-gcc`, I cannot see where this is needed and what it really means. Nixpkgs will then perform the install phase after the build phase, so this should just work I think. But I still got to get the necessary target's of binutils included here in this mkDerivation.

What can buildInputs be set to?

Er.. let's try this..

`nix-build --out-link ./gccresult ./gcc.nix`

The `stdenvNoCC` just doesn't have a cc part of the stdenv. Which makes compilation impossible unless you explicitly specify which clib and bring that in. I don't think that's a good idea.

Ok so we're stuck trying to compile this.

Compilation errors involving fibheap is somewhat related to the GCC provided in stdenv due to this problem:

https://github.com/NixOS/nixpkgs/issues/27889

Try the commit before this date, (or after this problem is resolved). And try to compile a gcc. Changing gcc versions doesn't change anything. Also this means there's nothing wrong with the gcc source code, but something wrong about the compilation technique used in nixpkgs at this moment.

---

Ok that all failed too.

So I'm just going to perform a local installation of gcc cross using just shell.nix. The shell.nix brings in gmp, mpfr, and libmpc. And very importantly sets `hardeningDisable = [ "all" ];'`. This is needed since gcc source code is still not amenable to hardening, also we don't want to make it harder than it has to be.

Anyway, then we install the necessary binutils along with gcc. We pick gcc 5.4.0 and the corresponding binutils.

```
wget ftp://ftp.gnu.org/gnu/binutils/binutils-2.28.tar.bz2
wget ftp://ftp.gnu.org/gnu/gcc/gcc-5.4.0/gcc-5.4.0.tar.bz2

tar xvjf ./binutils-2.28.tar.bz2
tar xvjf ./gcc-5.4.0.tar.bz2

cd ./gcc-5.4.0
./contrib/download_prerequisites

export PREFIX="$(pwd)/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

mkdir build-binutils
cd build-binutils
../binutils-x.y.z/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

mkdir build-gcc
cd build-gcc
../gcc-x.y.z/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
```

By using the `--prefix` option, we can perform local directory installations!

It appears to be working, it got past the issue with libiberty. But now the compilation is taking quite some time...

Remember everything gets installed inside `opt/cross`. So even the normal gcc will be installed there. This is quite weird why nix-build fails to do this properly, but normal style does... I wonder if this is due to some custom phases occuring on NixOS...

My `LD_LIBRARY_PATH` inside the nix-shell is not being set properly which appears to make the `make all-target-libgcc` fail to find those libraries, even though I have these libraries as part of my `buildInputs`.

No what we need to do is to put the libPaths into the actual linkable paths for the make command.

The gcc wrapper which is `stdenv.mkDerivation` will add the include sub directory to `NIX_CFLAGS_COMPILE` while the lib and lib64 will be in `NIX_LDFLAGS`. Is there a way to make use of this while performing the make target?

```
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/nix/store/asv92dzp12fpxd3fg6mqp8nanhml4n8y-qemuos/lib:/nix/store/qzyd45h6668mvl7c0d4r2h6bak9dm9dl-gmp-6.1.1/lib:/nix/store/0s0m3gj8jwpf81q35fx7h6s2879q8krg-mpfr-3.1.3/lib:/nix/store/2hj7d9by971djpx5v8pg1d077qdbdv8w-libmpc-1.0.3/lib"
```

So apparently the idea is that only specific MPR and ..etc libraries must be used. And according to the GCC FAQ. If you have them installed but not in the default search path, then you need to use `--with-mpr` settings. But if you use the above contrib download script, you'll get the sources in with gcc, which means make all-gcc will actually just build gcc directly and build the other libraries from source as well, so then you don't need to worry about bringing those into the shared paths, so I think I was missing the `--with-mpr` options, or nix-build must be plugging in the lib paths properly, I don't understand stdenv mkDerivation's nix-build well enough at this moment. Note that those are configure falgs.

---

Yes it finally works, but it's using gmp, mpfr and libmpc as compiled from source, and then statically linked rather than nixpkgs provided gmp, mpfr and libmpc. To figure out how to get those involved, I will need to try to change my `LD_LIBRARY_PATH` or figure out passing the flags like `--with-mpfr` and friends at the configure script or configure phase. Most importantly I need to get a better understanding of stdenv.mkDerivation.

---

So now we have the cross compiler running inside `opt/cross`.

We're going to use nasm instead of gas, since gas sucks.

So how do we get nasm for i686-elf? Wait how come nasm doesn't require to be cross compiled while gas does?

Oh looks like you actually need to cross compile the nasm as well with the relevant target. Will try to do the proper building later by using the actual `--with` settings on configure, but later I think a proper understanding of mkDerivation will be needed.

The multiboot specification is a open standard for how a bootloader can load an x86 operating system kernel. I wonder if this applies specifically to GRUB + kernel, or to things like UEFI + kernel, since apparently the linux kernel can act as a UEFI application directly.

The specification was created at 1995 and now used by Hurd, VMWare, ESXi, Xen, L4 microkernels.

GNU Grub is the reference implementation used in GNU operating systems. Or does the bootloader work like a interface between the firmware (UEFI) and OS.

So Hardware -> UEFI Firmware -> Launch UEFI application (bootloader or directly launching linux kernel) -> Launch kernel via multiboot.

Remember that Linux kernel since 3.3 supports EFISTUB which means EFI BOOT STUP. This allows uefi firmware to load the kernel as a EFI executable, thus skipping the bootloader stage. However it is preferable to use a bootloader as you get advantages such as being able to switch between multiple kernels (if you have different iterations and generations of the kernel just like NixOS does which takes advantage of this process for having rollbackable kernel generations.). But even if your kernel hasn't changed, there's als othe issue with changing the initramfs. And also the bootloader is something you can choose, whereas the UEFI firmware is something that comes with teh vendor, and it is fixed, generally you can't or shouldn't change the firmware, because there's only something like coreboot which isn't even supported by lots of systems.

Note that UEFI can also do bootloader style. Where it can choose between different UEFI applications, but again this depends on the UEFI implementation. So of course a bootloader is still preferred.

Turns out that nasm is already cross compiler capable, no need to have a specific version of it!

---

We compile the:

```
nasm -felf32 boot.asm boot.o
i686-elf-as boot.s -o boot.o
i686-elf-gcc -c kernel.c -o kernel.o -std=gnu11 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc
```

Linking the kernel requires a custom linker script. Never heard of linker scripts.

A linker script is basically a file that ld will parse and link according to it. So it's a more sophisticated way to specify ld options basically.

The myos.bin is now the kernel executable. It is completely statically compiled, if you use `ldd` on it, it says not a dynamic executable, so nothing is linked into there!

So we can install GRUB as an actual application, we should be able to check whether the myos.bin is multiboot compatible. But installing the grub package shows that the grub-file doesn't even exist. So I'm not sure which package allows grub-file to exist?

We use grub2 to bring in necessary tools like `grub-file` to test if our os is multiboot compatible.

Now we need to provide the necessary metadata and compile it all into a iso archive format (which hypervisors like QEMU can read), or even physical machines if the iso archive is burned onto disk. Although I wonder if this means grub2 should really be used in preference over the other bootloaders like systemd boot.

We need to install `xorriso`.

```
mkdir -p isodor/boot/grub
cp src/myos.bin isodir/boot/myos.bin
cp src/grub.cfg isodir/boot/grub/grub.cfg
grub-mkrescue -o myos.iso isodir
```

This basically wraps the kernel with grub, and exports an ISO. Of which it seems to use a particular file format. An iso is an archive file of an optical disk. It is composed of the data contents of every written sector on an optical disk, including the optical disk filesystem. ISO is taken from the name of the standard called "ISO 9660" which was meant to be the filesystem used for CD-ROM media. Funny how the optical disk has now died, and been replaced with USB, but the filesystem format lives on, as we can flash ISOs onto USBs, and have computers read from a USB thinking of it like a CD-ROM. "flashing" is probably not the right word to use, as this is meant for flashing flash memory or EEPROM. basically overwriting firmware or data in these storage devices. The more correct term is to "burn" and ISO. but that's for CDs, what about just "writing" it to the USB. But again this is not correct as we are replacing the entire USB contents. I guess "formatting" would be the right one to use for USBs and HDD/SSD partitions.. etc. On windows I use rufus to format USBs to be bootable and write ISOs to them. But it's only available on Windows, so for Linux, UNetbootin appears more popular. Oh wait rufus works on linux too! But nixpkgs doesn't have it, only unetbootin. Without these, there's always good old dd, but it's very error prone. I think bootable ISOs are ultimately used as a delivery method for desktop and laptops and x86 style architectures, for other architectures, this may not make sense, such as creating an OS for an embedded arm platform, it wouldn't make sense to do this, as you would be "flashing" firmware instead. Oh apparently ISO was most common for CD, while UDF was most common for DVDs. I guess bluray is different.

You have a choice, you can either "flash" the firmware onto embedded device storage, "burn" the ISO to a optical disc, or "format" it to bootable USB or virtual disk image file.

Distributing a disk with this iso, requires you to also publish the sourcecode of GRUB. This is easy if you release this online and with nixpkgs, afterall, it's just whatever your nixpkgs commit hash and that points to agrub derivation and that points toa grub source revision!

With qemu this is even easier, we can just start running this stuff immediately!

```
qemu-system-i386 -cdrom myos.iso
qemu-system-i386 -kernel myos.bin
```

Then we get it running!

So we don't need to wrap it into an ISO since we can test directly from myos.bin using the `-kernel` setting, this is because our kernel is multiboot compatible.

Actually it's apparently simple to put it into a USB: `dd if=myos.iso of=/dev/... && sync`.

But I'm not sure if this would make it usable as UEFI. Basically the ability to boot it like a UEFI style would require special gpt partitions I think.

To allow newlines you need to actually program in how the VGA text mode interprets the newline, and not actually print out the newline character but reset the text cursor to the next location.

Then you have to implement terminal scrolling. Then rendering colorful ascii art. You have 16 colours available. Normally now you can write a custom OS to just do what is needed, so this would be relevant to embedded systems. What's more interesting is keyboard input! So you can write things there.

Also I wonder if since we have C available here, this is when we could use things that compiled to freestanding programs. Things like Rust were designed to do this, so it would be interesting to try this in Rust. Also at some point you need to bring in a C standard library, for simple use cases, apparently an embedded C stdlib is a good idea. I wonder how to bring in something like uclibc.

It apparently possible for qemu to launch a bunch of other virtual hardware, I wonder what could be done for ARM systems. It's possible a different kind of assembly needs to be written. NASM doesn't support anything except x86/64. It's after all created by intel. So I guess that's one advantage of gas syntax which supports targetting non-x86 platforms. But I should be able to use intel syntax on the GAS assembler, perhaps we should try this. Let's try using gas with intel syntax.

Apparently an alternative to creating a multiboot kernel, is a kernel wrapped as a UEFI application, which the linux kernel is when it supports the EFI stub. There are a number of advantages over a multiboot kernel:

1. For x86 processors, you avoid having to upgrade from real mode to protected mode, and then from protected mode to long mode, which is more commonly known as 64 bit mode. As a uefi application, your kernel is already in a x64 environment.
2. UEFI is well documented.
3. UEFI is extensible.

However writing out a UEFI application does require extra infrastructure, basically you need a system to setup the UEFI application, which usually requires the GNU-EFI libraries, which does mean you'll start writing out everything in C instead of assembly. Furthermore you'll also need a UEFI firmware, which QEMU does not provide, this means you can use the tianocore which is an opensource UEFI firmware, and finally you also need the combine everything into a bootable UEFI fat32 partition (which requires partitioning tools like parted and gparted..), just like combining it into an ISO. However QEMU could have just executed the multiboot kernel directly without all this jazz.

So I wrote the GAS style as well, as it turns out that NASM is only for x86 and if we want to use other systems, we won't be able to, but with GAS, we can eventually target other kinds of architectures.

Note that while NASM are usually `.asm` and `.inc` for inclusions, GAS assembler are usuaslly `.s` for normal implementations. I think you can end up using `.inc` as well for GAS assembler or it doesn't matter. Yea so we don't really have header files for assembly, so it's all `.s` really.

---

Following:

* https://os.phil-opp.com/multiboot-kernel/
* http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html
* http://wiki.osdev.org/Going_Further_on_x86
* http://wiki.osdev.org/Meaty_Skeleton
