# hashjoin-scm


## Dependencies
### Install PMDK:

```
$ aptitude search pmem
$ apt-cache search pmem
$ apt search pmem

$ apt-get install <library>

All Runtime: 
$ sudo apt-get install libpmem1 librpmem1 libpmemblk1 libpmemlog1 libpmemobj1 libpmempool1
All Development: 
$ sudo apt-get install libpmem-dev librpmem-dev libpmemblk-dev libpmemlog-dev libpmemobj-dev libpmempool-dev libpmempool-dev
All Debug:
$ sudo apt-get install libpmem1-debug librpmem1-debug libpmemblk1-debug libpmemlog1-debug libpmemobj1-debug libpmempool1-debug
```

### Install daxctl
```
sudo apt install daxctl
```

### Install ndctl
```
sudo apt install ndctl daxctl
```

### Install ipmctl
```
$ sudo apt update
$ sudo apt search ipmctl
$ sudo apt info ipmctl
```
Install [ipmctl](https://github.com/intel/ipmctl)


### Install Intel-PMWatch
```
$ sudo apt update -y
$ sudo apt install -y autoconf automake libtool libconfig-dev libkmod-dev libudev-dev uuid-dev libipmctl-dev libnuma-dev
```
Install [Intel-PMWatch](https://github.com/intel/intel-pmwatch)


### Install Other Packages
```
$ sudo apt install -y numactl
```

### Grant Permissions (for PAPI):  
```
sudo sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'
```

