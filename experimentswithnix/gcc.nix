# our build script is different
# what was the designation of the build script?
{ pkgs ? import ../nixpkgs/default.nix {} }:
  with pkgs;
  stdenv.mkDerivation {
    name = "i686-elf-gcc-4.9.2";
    src = fetchurl {
      url = ftp://ftp.gnu.org/gnu/gcc/gcc-4.9.2/gcc-4.9.2.tar.gz;
      sha256 = "0pivlj3y93ahjba4qn9yvwhd073jb2y8yh11g3a643cbxhk3hmry";
    };
    buildInputs = [ (callPackage ./binutils.nix {}) gmp mpfr libmpc libelf ];
    configureFlags = "--target=i686-elf --with-sysroo";
    hardeningDisable = [ "all" ];
    buildPhase = ''
      make all-gcc
      make all-target-libgcc
    '';
    NIX_DEBUG = 1;
  }
