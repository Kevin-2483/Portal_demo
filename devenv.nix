{ pkgs, lib, ... }:
{
  dotenv.enable = true;
  languages.java.enable = true;
  languages.python.enable = true;
  languages.python.venv.enable = true;
  languages.cplusplus.enable = true;
  packages = with pkgs; [
    cocoapods
    clang
    cmake
    ninja
    pkg-config
    just
    python3
    scons
  ];
  enterShell = ''
    export PATH=$PATH:/Applications/Godot.app/Contents/MacOS
    zsh
'';
}
