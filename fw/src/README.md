Setup is done with environment variables.

## Build

```
mkdir build
cd build
cmake ..
make -j4
```

## Partition

To modify the partiton you have to modify the file pt.json
and run the following command to create the UF2 file :
```bash
picotool partition create pt.json pt.uf2
```

And to flash the partition:
```bash
picotool load pt.json
picotool reboot -u
```
