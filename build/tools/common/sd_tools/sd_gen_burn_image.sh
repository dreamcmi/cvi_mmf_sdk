#!/bin/bash
# a sd image generator for sophpi
# usage
if [ "$#" -ne "1" ]
then
	echo "usage: sudo ./sd_gen_burn_image.sh OUTPUT_DIR"
	echo ""
	echo "       The script is used to create a sdcard image with two partitions, "
	echo "       one is fat32 with 128MB, the other is ext4 with 256MB."
	echo "       You can modify the capacities in this script as you wish!"
	echo ""
	echo "Note:  Please backup you sdcard files before using this image!"

	exit
fi

output_dir=$1
echo ${output_dir}
pushd ${output_dir}

image=${PROJECT_FULLNAME}-`date +%Y%m%d-%H%M`-sdcard.zip

genimage --config genimage.cfg --rootpath fs/ --tmppath `mktemp -d`  --inputpath $PWD --outputpath $PWD

zip ${image} sdcard.img

echo "Gen image successful: ${image}"
echo ""

popd
