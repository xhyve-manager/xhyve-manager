#!/usr/bin/env sh

echo "Creating directories"
mkdir -p ~/VDisks
mkdir -p ~/Xhyve\ Virtual\ Machines

echo "Setting up NFS"
echo "# XHYVE\n/Users -mapall=501 -network 192.168.64.0 -alldirs -mask 255.255.255.0\n" | sudo tee -a /etc/exports
sudo nfsd restart
