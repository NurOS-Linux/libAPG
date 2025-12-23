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
        ];

        buildInputs = [
          pkgs.gcc
          pkgs.lmdb
          pkgs.libarchive
          pkgs.cjson
          pkgs.tree
          libiron
        ];

        mesonFlags = [
          "--buildtype=release"
        ];

        installPhase = ''
          mkdir -p $out/lib $out/include/pkg-config
          cp libapg.so $out/lib/
        '';
      };
    };
}
