# TempleVCS
TempleOS Version Control System

## Purpose
This project aims to create an extremely lightweight version control system with only POSIX complient code. Only the very basic functionalities of a version control system will be implemented.

The intended use case would be for the user to checkout the compressed TempleOS source code directly onto a blank disk image. This will bypass the installation step. To update TempleOS, the user will power off the virtual machine, mount the disk image on the host OS, and then update the repository.

## Build
Work in progress

## Commands
Work in progress

## Background
Traditional version control systems such as Git and SVN are too heavyweight. Furthurmore, they do not support TempleOS' RedSea file format. As a result, the TempleOS repo needed to be converted to ASCII prior to uploading, which strips scprites and some other data. If that repo is downloaded as-is, then the demos and the games will not work due to the stripped sprites and such.
