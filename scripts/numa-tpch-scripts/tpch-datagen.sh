date

cd ../..
git clone https://github.com/electrum/tpch-dbgen
cd tpch-dbgen
make
dbgen -s 100
mkdir ../tpch-data
mv lineitem.tbl ../tpch-data
mv part.tbl ../tpch-data
cd ..
./revitalize.sh
./bin/tpch/tpch_tbl2bin 100


date