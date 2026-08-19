{
    description = "blot — braille plotting library written in C";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
        flake-utils.url = "github:numtide/flake-utils";
    };

    outputs = { self, nixpkgs, flake-utils }:
        flake-utils.lib.eachDefaultSystem (system:
                let
                pkgs = nixpkgs.legacyPackages.${system};

                baseVersion = pkgs.lib.fileContents ./VERSION;
                buildDate = builtins.substring 0 8 (self.lastModifiedDate or "00000000");
                shortHash = self.shortRev or self.dirtyShortRev or "unknown";
                dirtySuffix = if self ? rev then "" else "-dirty";
                blotVersion = "${baseVersion}-${buildDate}-g${shortHash}${dirtySuffix}";
                in
                {
                packages.default = pkgs.callPackage ./nix/package.nix {
                    inherit pkgs;
                    version = blotVersion;
                };

                devShells.default = pkgs.mkShell {
                    name = "blot-dev";
                    packages = with pkgs; [
                        cmake ninja pkg-config gnumake
                            gcc clang clang-tools
                            libgit2 gtest git python3
                    ];
                    PKG_CONFIG_PATH = "${pkgs.libgit2}/lib/pkgconfig";
                    shellHook = ''
                        echo "blot dev shell ready"
                        echo "  build:  make"
                        echo "  test:   make test"
                        echo "  install (local): make install"
                        '';
                };
                });
}
