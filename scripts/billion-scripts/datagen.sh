date


mkdir ../../build
cd ../../build
rm -rf *
cmake .. -DRUN_BILLION=true
make -j 32
cd ../scripts/billion-scripts/
numactl --physcpubind=20-39 --membind=0-1 ../../bin/gen


date