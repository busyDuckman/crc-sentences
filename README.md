# CRC-sentences
  
A simple tool to create "autological sentences" for testing/fun, ie: sentences that describe themselves.
  
  
eg:   
  - handily, this text has a CRC of: 00cb5f79 and a length of 61. 
  - believe it or not, this text has a CRC of: 00BEE21E



##How to build

Compile from source as per: 

    git clone https://github.com/busyDuckman/crc-sentences.git
    cd crc-sentences
    mkdir build
    cd build
    cmake .. 
    make

## Credit's
This code uses:
  - Daniel Bahr's [CRC++ library](https://github.com/d-bahr/CRCpp)
