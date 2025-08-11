{
  description = "libAPG flake";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    rust-overlay.url = "github:oxalica/rust-overlay";
    rust-overlay.inputs.nixpkgs.follows = "nixpkgs";
  };
  outputs = { self, nixpkgs, flake-utils, rust-overlay }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        overlays = [ (import rust-overlay) ];
        pkgs = import nixpkgs {
          inherit system overlays;
        };
        rustToolchain = pkgs.rust-bin.stable.latest.default.override {
          extensions = [ "rust-src" "rust-analyzer" ];
        };
        buildInputs = with pkgs; [
          rustToolchain
          pkg-config
          openssl
        ];
        shellHook = ''
          rustc --version
        '';
      in
      {
        devShells.default = pkgs.mkShell {
          inherit buildInputs shellHook;
          nativeBuildInputs = with pkgs; [
            cargo-watch
          ];
        };
        packages.default = pkgs.rustPlatform.buildRustPackage {
          pname = "libapg";
          version = "0.1.0";
          src = ./.;
          cargoLock = {
            lockFile = ./Cargo.lock;
          };
          inherit buildInputs;
        };

      }
    );
}