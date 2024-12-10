# 3DS ROFS dumper

Allows dumping very early 3DS ROFS containers.

## Compiling

Add `-static` if static linking is needed

### Windows

`g++ -std=c++20 main.cpp -o rofs_dump.exe`

### MacOS / Linux

`g++ -std=c++20 main.cpp -o rofs_dump`

## Usage

`rofs_dump (in_file) (out_dir)`

## License

[Unlicense](LICENSE.md)
