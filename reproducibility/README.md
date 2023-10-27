# PVLDB Reproducibility
> Wentao Huang, et al. *A Design Space Exploration and Evaluation for Main-Memory Hash Joins in Storage Class Memory* VLDB'23.

[**[Paper]**](https://www.vldb.org/pvldb/vol16/p1249-huang.pdf) 
[**[Bibtex]**](https://dblp.org/rec/journals/pvldb/HuangJZHT23.html?view=bibtex)

## 1. Requirements and Overview
### Hardware requirements
* A two-socket machine with 2nd generation (or above) scalable processors.  
* 1.5TB Intel Optane DIMMs. 
* DRAM >= 300GB 
* CPU >= 80 cores \& >= 40 physical cores (because the maximum thread number is 88 in the evaluations). 

Note: If you do not meet the above requirements, you may not be able to reproduce all the experimental results.  

### Software requirements
* Linux Kernel >= 5.4.0-163
* OS version >= Ubuntu 20.04.6
* CMake >= 3.16.3
* gcc >= 9.4.0
* PAPI >= 6.0.0.1
* Intel PMWatch >= 3.2.1


## 2. Setup (Skip to Section-3 if evaluated on our provided machine)

### 2.1. Clone this project from Github.
```
$ git clone https://github.com/fukien/hashjoin-scm
```

### 2.2 Install PAPI
Install [PAPI](https://icl.utk.edu/papi/)

### 2.3 Install Intel-PMWatch
Install [Intel-PMWatch](https://github.com/intel/intel-pmwatch)

### 2.4 Install other dependencies
```
$ cd reproducibility/
$ sudo bash install.sh

```
### 2.5 SCM setup
Configure the SCM mounting points as "/pmemfs0" and "/pmemfs1," and ensure that "/pmemfs0" is set as the default SCM mounting point for the first socket (assuming core IDs start from 0 on the first socket).

### 2.6 Configuration Parameters
- Adjust "[./config/mc/cpu.cfg](../config/mc/cpu.cfg)" file according to the core IDs obtained from "numactl -H." By default, we consider socket-0 as "local" and socket-1 as "remote." For example:

```
cores: {
	local_core_id_arrays = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]; 
	remote_core_id_arrays = [20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39];
};  
```
 
- Modify the core number and the single socket core number in [CMakeLists.txt](../CMakeLists.txt) (lines 19-20). 

- Update the paths for PAPI and Intel-PMWatch in [CMakeLists.txt](../CMakeLists.txt) (lines 15-17). 


### 2.7 Compile & build
```
$ ./revitalize.sh
```
### 2.8 Data generation
```
$ ./bin/gen
```

### 2.9 Run initial test experiment
- Run NPHJ-SC: 
```
$ ./bin/main --workload=uniform --subtype=A --param=pkfk --algo=nphj_sc
```

- Run RDX-BC with the bandwidth regulation mechanism:
```
$ ./bin/main --workload=uniform --subtype=A --param=pkfk --algo=phj_radix_bw_reg_bc
```




## 3 Reproduce the experiment results from the paper.

### General instruction
Please navigate to the indicated folders and execute the provided scripts ("run\_fig\*.sh") to replicate the corresponding experiments. Additionally, you'll find the plotting scripts ("plot\_\*.py") within these folders for further analysis.

Figure | Scritps
---|---
"Figure 1" | [fig1-scripts](./fig1-scripts)  
"Figure 3" | [fig3-scripts](./fig3-scripts)  
"Figure 4(a)" | [fig4a-scripts](./fig4a-scripts)  
"Figure 4(b)" | [fig4b-scripts](./fig4b-scripts)  
"Figure 5" | [fig5-scripts](./fig5-scripts)  
"Figure 6" | [fig6-scripts](./fig6-scripts)  
"Figure 7" | [fig7-scripts](./fig7-scripts)  
"Figure 8(a)" | [fig8a-scripts](./fig8a-scripts)  
"Figure 8(b)" | [fig8b-scripts](./fig8b-scripts)  
"Figure 8(c)" | [fig8c-scripts](./fig8cscripts)  
"Figure 8(d)" | [fig8d-scripts](./fig8d-scripts)  
"Figure 9" | [fig9-scripts](./fig9-scripts)  
"Figure 10" | [fig10-scripts](./fig10-scripts)  

Note:  
* For replicating the experiments in Figure 3 and Figure 4(b), sudo access may be required.  
* To reproduce experiments in Figure 8(d), Figure 9, and Figure 10(b), it may be necessary to clear the SCM mounting path since they typically involve datasets of several hundred gigabytes."

### One-click replication for all experiments
```
$ cd reproducibility/
$ bash run_all.sh
```

All experiment running logs will be stored in "[./logs/](../logs/)" directory (our running logs are saved in "[./logs/](../logs/)" for your reference ). 

Note: "[run\_all.sh](./run_all.sh)" usually takes 27 hours to finish (Fig 10(b) experiments typically last 17 hours).


### Plot all experiment figures corresponding to the paper
Retrieve all the logs from "[./logs/](../logs/)" directory to your local machine, especially if the experiments were conducted on a remote SCM-equipped server.   

```
$ cd reproducibility/
$ bash plot_all.sh
```
All experiment figures will be generated and saved in "[./figs/](../figs/)" directory. 

Note: We have uploaded the figures we generated to "[./figs/](../figs/)" directory for your reference.