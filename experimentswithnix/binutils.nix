{ pkgs ? import ../nixpkgs/default.nix {} }:
  with pkgs;
  stdenv.mkDerivation {
    name = "i686-elf-binutils-2.25";
    src = fetchurl {
      url = ftp://ftp.gnu.org/gnu/binutils/binutils-2.25.tar.gz;
      sha256 = "0b9gj330zjv5hby6kgf1nda5ca7pp3z1ips6dykm46mld1qkgkyc";
    };
    configureFlags = "--target=i686-elf --with-sysroot --disable-nls --disable-werror";
  }
