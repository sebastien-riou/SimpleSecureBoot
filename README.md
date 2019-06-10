# SimpleSecureBoot
RSA based secure boot for embedded devices

## Dependencies
### Python3
Python3 is used to signed the hex file.

### Python3 intelhex
On ubuntu:

    pip3 install --user intelhex

### Optional: Python3 pycryptodome
This is needed only if you want to generate keys using `genrsakey.py`.
On ubuntu:

    pip3 install --user pycryptodome

## Test
A self test is provided, to run it type the following:

    ./test.py

## Usage
### Create a key pair:
* Generate a key using `genrsakey.py` (or any other method).
* Place the generate output in key file.

One command does everything:

    ./genrsakey.py > mykey

### Integrate in boot code
* Include the C99 header files in the firmware which resides in internal memory.
* Call the `ssb_check_sig` function to validate an incoming image.

### Sign an image
Use `ssb.py` to sign the hex file:

    ./ssb.py <start address> <size> <hex file> <key file>

This will compile the C99 test code to make sure it verify this signature correctly.

If this check fails, `ssb.py` output the key as it should appear in `ssb_pub_key.h`. Update this file accordingly and try again, `ssb.py` output should then end by "sig match".
