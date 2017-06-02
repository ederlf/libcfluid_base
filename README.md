# libcfluid_base
C implementation of the network portion of [LibFluid](http://opennetworkingfoundation.github.io/libfluid/).

**libcfluid_base** is a pure C implementation of the [libfluid_base](https://github.com/OpenNetworkingFoundation/libfluid_base), a library that provides a connectivity layer for the OpenFlow protocol.

The C port aims to keep the same structure of the C++ code, with some tweaks to adapt the previous Object Oriented design. For now most of the code
remains the same from the C++ version, which may not reflect an ideal C implementation.   

### TODO

Implementation of OFBaseServer and OFServer.

### ACKS

Many thanks to [Allan Vidal](https://github.com/alnvdl) for bringing the beautiful original version of libfluid_base to life. 

The library includes a nice and short implementation of vector in c named 
[c-vector](https://github.com/eteran/c-vector).