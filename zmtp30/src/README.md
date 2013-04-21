# Minimal PUB/SUB stubs for ZMTP v3.0

To build:

    ./c -l pub sub pub20 sub20

To run

    ./sub       #  Binds and waits for data
    ./pub       #  Connects and sends data

# Notes

This is a strictly minimal test to demo the ZMTP v3.0 protocol and NULL mechanism. Right now these two programs are the only ZMTP v3.0 programs in the universe so they're kind of stuck talking to each other. I'll expand these slowly to cover other socket types, and the other mechanisms.

This code implements ZMTP 2.0 backwards compatibility, which you can test using the sub20 and pub20 publishers (also minimal stubs that don't use libzmq).