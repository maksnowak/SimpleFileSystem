# Simple File System

This repository contains a simple file system implementation in C. The file system allows for:
- creation of virtual disks
- selecting existing virtual disks
- copying files to and from the virtual disk
- listing files stored in the vritual disk
- deleting files from the virtual disk
- deleting virtual disks
- displaying disk information (used space, number of inodes and data blocks, etc.)
- defragmenting the virtual disk

The file system can store both text and binary files.

## How to run

Note: the program has been tested on Ubuntu 22.04 LTS.

1. Clone the repository
2. Compile source file - `gcc main.c fs.c`
3. Launch the executable - `./a.out`