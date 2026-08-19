{ stdenv
, pkgs
, cmake
, ninja
, pkg-config
, git
, glib
, clipp
, version ? "unstable"
}:

stdenv.mkDerivation {
    pname = "blot";
    inherit version;

    src = ./..;

    nativeBuildInputs = [ cmake ninja pkg-config git ];

    buildInputs = [ glib.dev clipp ];

    PKG_CONFIG_PATH = with pkgs; lib.makeSearchPath "lib/pkgconfig" [
    ];

    postPatch = ''
        '';

    preConfigure = ''
        cmakeFlagsArray+=()
        '';

    cmakeFlags = [
        "-DCMAKE_BUILD_TYPE=Release"
        "-DBUILD_TESTING=OFF"
        "-DGIT_REFSPEC=..."          # TODO: these need to be filled with info about the version
        "-DGIT_REVISION=..."
        "-DGIT_SHA1=..."
    ];

    buildPhase = ''
        # TODO: use cmake to build, using cmakeFlags
        '';

    installPhase = ''
        # TODO: use cmake or shell to install build/cli/blot in $out
        '';

    meta = with pkgs.lib; {
        description = "blot — braille plotting library written in C";
        homepage = "https://github.com/bartman/blot";
        license = licenses.gpl2Only;
        platforms = platforms.linux ++ platforms.darwin;
    };
}
