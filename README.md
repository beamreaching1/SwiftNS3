# SwiftNS3
An NS3 implementation of Google's Swift protocol

This program was built on NS3.34

To run replace duplicate NS3 files with the files from this repository.

Configure NS3 with this command:
./waf configure --enable-build-version --build-profile=optimized --enable-examples --enable-tests

There are four different simulation files/scenarios:
1. Swift
./waf -v --run="scratch/Swift/Swift"
2. DCTCP
./waf -v --run="scratch/DCTCP/DCTCP"
3. DCTCPLoad 
./waf -v --run="scratch/DCTCPLoad/DCTCPLoad"
4. SwiftLoad
./waf -v --run="scratch/SwiftLoad/SwiftLoad"

The load versions are exactly the same except there is an extra node cluster
of 50 senders to increase network load.
