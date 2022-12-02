# Experiment Reproduction

## Instruction
Please visit the following folders to reproduce (and plot figures) for therespective experiments.


Experiments | Scritps
---|---
microbenchmarks of persistent instructions/page faults | 0623-scripts
NPHJ prefetching | 0730/0808-scripts
bucket alignment | 0731/0802/0803-scripts
SWWCB and "ntstore" | 0819-scripts
PHJ thread scalability | 0815/0816-scripts
"putting all together" | 0829/1201-scripts
"a rigorous evaluation" | 1019/1020-scripts
payload study | 1022/1024-scripts
"read/write asymmetry" | 1021/1028/1030/1031-scripts


## Additional Data Generation
### Billion-scale workload
```
mkdir build
cd build
cmake .. -DRUN_BILLION=true
make -j 32
cd ..
./gen
```

### Large-size payload workload
```
mkdir build
cd build
cmake .. -DRUN_PAYLOAD=true -DPAYLOAD_SIZE={payload_size}
make -j 32
cd ..
./gen
```

### Other tips:
Since the above workloads are huge, you may need to reserve some space in mounting points for generating these datasets.
