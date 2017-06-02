# libcfluid_base
C implementation of the network portion of [LibFluid](http://opennetworkingfoundation.github.io/libfluid/).

**libcfluid_base** is a pure C implementation of the [libfluid_base](https://github.com/OpenNetworkingFoundation/libfluid_base), a library that provides a connectivity layer for the OpenFlow protocol.

The C port aims to keep the same structure of the C++ code, with some tweaks to adapt the previous Object Oriented design. For now most of the code
remains the same from the C++ version, which may not reflect an ideal C implementation.   

### Install

Default installation installs library on "/usr/local/lib" and include files on "/usr/local/include/cfluid". 
You can specify an installation prefix other than "/usr/local" using "--prefix" on configure.

```bash
./autogen
./configure
make
sudo make install
```

### Basic Usage

Example of how to connect to an OpenFlow controller. 

```C
#include <cfluid/of_client.h>
#include <stdio.h>
#include <signal.h>

volatile sig_atomic_t quit = false;

static void sigint_handler(int sig) {
   quit = true;
}

static void wait_for_sigint() {
   struct sigaction sa;
   memset(&sa, 0, sizeof(sa));
   sa.sa_handler = sigint_handler;
   sigfillset(&sa.sa_mask);
   sigaction(SIGINT, &sa, NULL);

   while (1) {

       if (quit) {
        break;
       }
       sleep(1000);
   }
}

int main(int argc, char const *argv[])
{
    /* code */
    struct of_settings *ofsc = of_settings_new();
    ofsc_supported_version(ofsc, 0x04);
    ofsc->is_controller = false;
    struct of_client *ofc = of_client_new(0, "127.0.0.1", 6653, ofsc);
    of_client_start(ofc, false);
    wait_for_sigint();
    of_client_stop(ofc);
    of_client_destroy(ofc);
    return 0;
}

```

### TODO

Implementation of OFBaseServer and OFServer in C.

### ACKS

Many thanks to [Allan Vidal](https://github.com/alnvdl) for bringing the beautiful original version of libfluid_base to life and aswering my questions. 

The library includes a nice and short implementation of vector in c named 
[c-vector](https://github.com/eteran/c-vector).
