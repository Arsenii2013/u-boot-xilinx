DTB=../boot/dtb/system.dtb

if [ ! -f $DTB ]; then
    echo "$DTB file dont exist"
    echo "First, generate Device Tree"
    exit 1 
fi

export CROSS_COMPILE=arm-none-eabi-
export ARCH=arm
export DEVICE_TREE="system"

cp $DTB arch/arm/dts

make xilinx_zynq_virt_defconfig
make
