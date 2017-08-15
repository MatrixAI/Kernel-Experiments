{ pkgs ? import <nixpkgs> {} }:
  with pkgs;
  stdenv.mkDerivation {
    name = "kernel-experiments";
    buildInputs = [ gmp mpfr libmpc nasm grub2 xorriso qemu ];
    hardeningDisable = [ "all" ];
    shellHook = ''
      export PREFIX="$(pwd)/opt/cross"
      export TARGET=i686-elf
      export PATH="$PREFIX/bin:$PATH"
    '';
  }
