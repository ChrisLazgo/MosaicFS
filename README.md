# MosaicFS
Linux FUSE driver to create disk images consisting of collection of small individual files.

## What is MosaicFS?

MosaicFS is a Linux FUSE driver to create and access disk images. Rather 
than being stored as a single large file, the content of the disk image 
is stored as a collection of individual small files (a mosaic of tile files). 
The disk image file has a fixed size defined at creation, but the individual 
tile files storing its content are created dynamically as content is added.

When used in combination with standard Linux tools, MosaicFS provides a 
functionality similar to MacOS sparse bundle disk images. Note however
 that MosaicFS is not and never will be compatible with sparse bundle disk 
images.

## Why MosaicFS?

The initial motivation for MosaicFS was to create disk images that are 
sparse and can grow dynamically as content is added. Moreover, because 
the content is stored as a collection of tile files, these disk images 
can be easily be mirrored to a remote server or cloud storage without 
having to replicate the entire image every time.

## Compiling

```bash
cd src
make
```

## Basic usage

### Overview

```bash
mosaicfs --help

Usage: mosaicfs <command> [options]

Available commands and options:

mosaicfs mount <archive> <path> [--fuse="<fuse arguments>"]

   Mount existing MosaicFS archive located in directory     <archive> on mount
   point <path>. <path> will be created if necessary. If <path> exists, it
   should be an empty file.

   --fuse="<...>" optional mount arguments to be passed to FUSE.

mosaicfs create --number <number of tiles> --size <tile size> <archive>

   Create new MosaicFS archive under directory <archive> with total size
   <number of tiles> x <tile size>:
   -n / --number N number of tile files
   -s / --size   S size of each individual tile file.     Default size is in
                   bytes, but other units are also available by appending
                   their appropriate units  (K = kilobyte,  M = megabyte,
                   G = gigabyte).
```

### Create MosaicFS disk image

```bash
mosaicfs create --number <number of tiles> --size <tile size> <archive>
mosaicfs create --number 8192 --size 128M disk.img
```

Create a new MosaicFS disk image under the directory 'disk.img'. 
The size of the disk image is 1 TB (8192 x 128 MB) and its content will 
be stored in 8192 tile files, each 128 MB in size.

### Mount MosaicFS disk image

    mosaicfs mount <archive> <path>
    mosaicfs mount disk.img disk

Mount with FUSE the MosaicFS disk image 'disk.img' under 'disk'. After 
mounting, 'disk' will appear as a a file of the size of the MosaicFS image. 
'disk' can be mounted as a loopback device and a file system can be created
on it. See next section for specific examples.

### Unmount MosaicFS disk image

```bash
fusermount -u <path>
fusermount -u disk
```


## MosaicFS disk image with ext4 file system

### Create and mount

Create a new MosaicFS 100 GB in size, consisting of 800x128 MB tile files. 
Create and ext4 file system and mount it.

```bash
mosaicfs create --number=800 --size=128M drive.img
mosaicfs mount drive.img drive --fuse="-o allow_root"
mkfs.ext4 drive
sudo mkdir -p /media/mydrive
sudo mount -o loop drive /media/mydrive
```

The fuse option "-o allow_root" is needed in order for root to be
able to access the image file. Alternatively, one can also mount
MosaicFS as root:

```bash
sudo mosaicfs mount drive.img drive
```

but some might find it objectionable to mount a Fuse file system as root.

### Mount

Mount a previously created disk image

```bash
mosaicfs mount drive.img drive --fuse="-o allow_root"
sudo mkdir -p /media/mydrive
sudo mount -o loop drive /media/mydrive
```

### Unmount

```bash
sudo umount /media/mydrive
sudo rmdir /media/mydrive
fusermount -u drive
```

## MosaicFS disk image with encrypted ext4 file system

Similar to the previous example, but adding an encryption layer using dm-crypt.

### Create and mount

```bash
# Create and Fuse mount MosaicFS
mosaicfs create --number=800 --size=128M drive.img
mosaicfs mount drive.img drive --fuse="-o allow_root"
# Find next available loopback device
sudo losetup -f
# Create loopback device
sudo losetup /dev/loop0 drive
# Create encryption layer
sudo cryptsetup -y -v luksFormat /dev/loop0
# Open encryption layer
sudo cryptsetup luksOpen /dev/loop0 drive
#--> block device is under /dev/mapper/drive
# Create file system
sudo mkfs.ext4 /dev/mapper/drive
# Mount file system
sudo mkdir -p /media/mydrive
sudo mount /dev/mapper/drive /media/mydrive
```

### Mount

```bash
# Fuse mount MosaicFS
mosaicfs mount drive.img drive --fuse="-o allow_root"
# Find next available loopback device
sudo losetup -f
# Create loopback device
sudo losetup /dev/loop0 drive
# Open encryption layer
sudo cryptsetup luksOpen /dev/loop0 drive
#--> block device is under /dev/mapper/drive
# Mount file system
sudo mkdir -p /media/mydrive
sudo mount /dev/mapper/drive /media/mydrive
```

### Unmount

```bash
sudo umount /media/mydrive
sudo rmdir /media/mydrive
sudo cryptsetup luksClose drive
sudo losetup -d /dev/loop0
fusermount -u drive
```

## Acknowledgement

Elements of MosaicFS were inspired by [concatfs](https://github.com/schlaile/concatfs) which allows read-only access to a concatenation of files. MosaicFS would not have been possible without the wonderful [Fuse](https://github.com/libfuse/libfuse) project and its documentation and tutorials.

## Disclaimer

Although MosaicFS has undergone a fair amount of testing, it is provided
AS-IS without any warranty. Before using it for mission critical applications,
you should perform due diligence and ensure that it meets your needs. 
See LICENSE for full disclaimer.

## Author

Chris Lazgo

chris.lazgo at gmx.com

