# i2c-set-volume
A utility app to set the volume of the speakers of external displays connected to a Mac.

To build it, execute the following command:
```
clang -o i2c_set_volume main.c -framework IOKit -framework CoreFoundation
```

Usage: 
```
i2c_set_volume <volume:[0-100]> <vendor_id> <product_id>
```
