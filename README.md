# A Design Space Exploration and Evaluation for Main-Memory Hash Joins in Storage Class Memory.
Code repository for our [VLDB'23 paper](https://www.vldb.org/pvldb/vol16/p1249-huang.pdf). 

## Prerequisites
```
1. A two-socket machine with 2nd generation scalable processors.  
2. 1.5TB Intel Optane DIMMs.  

Note: If you do not meet the above requirements, you may not be able to reproduce all the experimental results.  
```

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

## Quick Start
### Compile & Build
```
$ ./revitalize.sh
```
### Data Generation
```
$ ./bin/gen
```
### Running Joins
```
To run NPHJ-SC: 
$ ./bin/main --workload=uniform --subtype=A --param=pkfk --algo=nphj_sc

To run RDX-BC with the bandwidth regulation mechanism:
$ ./bin/main --workload=uniform --subtype=A --param=pkfk --algo=phj_radix_bw_reg_bc
```

## Reproducing Experiments 
See [scripts/](https://github.com/fukien/hashjoin-scm/tree/main/scripts).  
Note: You may want to amend the paths of SCM mounting points, PMWatch, and PAPI in [src/](https://github.com/fukien/hashjoin-scm/tree/main/src) before running the experimental scripts.


## Hardware Configuration
Since the experiments depend on special hardwares, the hardware configuration practices are available under request. 


## Further Support
If you have any enquiries, please contact huang@comp.nus.edu.sg (Huang Wentao) for the further support.

## Bibliography
```
@article{DBLP:journals/pvldb/HuangJZHT23,
  author       = {Wentao Huang and
                  Yunhong Ji and
                  Xuan Zhou and
                  Bingsheng He and
                  Kian{-}Lee Tan},
  title        = {A Design Space Exploration and Evaluation for Main-Memory Hash Joins
                  in Storage Class Memory},
  journal      = {Proc. {VLDB} Endow.},
  volume       = {16},
  number       = {6},
  pages        = {1249--1263},
  year         = {2023},
  url          = {https://www.vldb.org/pvldb/vol16/p1249-huang.pdf},
  timestamp    = {Tue, 18 Apr 2023 16:42:01 +0200},
  biburl       = {https://dblp.org/rec/journals/pvldb/HuangJZHT23.bib},
  bibsource    = {dblp computer science bibliography, https://dblp.org}
}
```