{
  description = "messagease-test for Pebble";

  inputs = {
    pebble.url = "github:MajorMajorMajorMajor/pebble.nix";
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.follows = "pebble/nixpkgs";
  };

  outputs =
    { pebble, flake-utils, nixpkgs, ... }:
    flake-utils.lib.eachDefaultSystem (
      system: 
      let
        pkgs = nixpkgs.legacyPackages.${system};
      
      in {            
        devShell = pebble.pebbleEnv.${system} { 
          packages = [
            pkgs.llvmPackages.clang-tools
            pkgs.cloc
            pkgs.unzip
          ];
        };
      }
    );
}
