img=$1
key=$2

echo $key $img

./tools/mkimage -F $img -k $key
fdtput $img -d /images/firmware-1 data-size
fdtput $img -d /images/firmware-1 data-offset
fdtput $img -ts /images/firmware-1/hash-1 algo sha256
fdtput $img -d /images/fdt-1 data-size
fdtput $img -d /images/fdt-1 data-offset
fdtput $img -ts /images/fdt-1/hash-1 algo sha256
fdtput $img -c /configurations/conf-1/signature
fdtput $img  /configurations/conf-1/signature algo -ts "sha256,ecdsa256"
./tools/mkimage -F $img -k $key
