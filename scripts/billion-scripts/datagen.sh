date


mkdir ../../build
cd ../../build
rm -rf *
cmake .. -DRUN_BILLION=true
make -j 32
cd ../scripts/billion-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/gen




date