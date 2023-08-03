export CROSS_COMPILE=arm-none-eabi-
export ARCH=arm
export DEVICE_TREE="system"

cp ../dts/system.dtb arch/arm/dts

make xilinx_zynq_virt_defconfig
make
