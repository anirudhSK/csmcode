#!/bin/bash
# extend this to some 10 nodes
brctl addbr br1
brctl addbr br2
brctl addbr br3
brctl addbr br4
brctl addbr br5
brctl addbr br6


tunctl -t tap1
tunctl -t tap2
tunctl -t tap3
tunctl -t tap4
tunctl -t tap5
tunctl -t tap6


ifconfig tap1 0.0.0.0 promisc up
ifconfig tap2 0.0.0.0 promisc up
ifconfig tap3 0.0.0.0 promisc up
ifconfig tap4 0.0.0.0 promisc up
ifconfig tap5 0.0.0.0 promisc up
ifconfig tap6 0.0.0.0 promisc up


brctl addif br1 tap1
ifconfig br1 up

brctl addif br2 tap2
ifconfig br2 up

brctl addif br3 tap3
ifconfig br3 up

brctl addif br4 tap4
ifconfig br4 up

brctl addif br5 tap5
ifconfig br5 up

brctl addif br6 tap6
ifconfig br6 up


pushd /proc/sys/net/bridge
for f in bridge-nf-*; do echo 0 > $f; done
popd

lxc-create -n vm1 -f lxc1.conf
lxc-create -n vm2 -f lxc2.conf
lxc-create -n vm3 -f lxc3.conf
lxc-create -n vm4 -f lxc4.conf
lxc-create -n vm5 -f lxc5.conf
lxc-create -n vm6 -f lxc6.conf
