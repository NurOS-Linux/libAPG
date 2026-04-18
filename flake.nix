{
  description = "Libapg project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    iron-log.url = "github:ruzen42/iron-log";
  };

  outputs = { self, nixpkgs, iron-log }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      libiron = iron-log.packages.${system}.default;
    in
    {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "libapg";
        version = "1.1.0";

        src = ./.;

        nativeBuildInputs = [
          pkgs.meson
          pkgs.ninja
          pkgs.pkg-config
          pkgs.gcc
          pkgs.binutils    # gas (as) for .S files
          libiron
        ];

        buildInputs = [
          pkgs.lmdb
          pkgs.libarchive
          pkgs.yyjson
          pkgs.libsodium   # for package signing (sodium backend)
          # pkgs.gpgme     # alternative signing backend
        ];

        mesonFlags = [
          "--buildtype=release"
        ];

        installPhase = ''
          ninja install
        '';
      };

      devShells.${system}.default = pkgs.mkShell {
        inputsFrom = [ self.packages.${system}.default ];
        packages = [
          pkgs.gdb
          pkgs.valgrind
        ];
      };
    };
}
