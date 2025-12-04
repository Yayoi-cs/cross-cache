make
musl-gcc e.c -o tsune -static
cp tsune.ko ../vm/out/
cp tsune ../vm/out/
cd ../vm
archive-cpio
python3 -c "input(114514)"
./run.sh

