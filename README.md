# openssl-hook

When debugging network applications it's common to look at the data being
sent to and from the application, using tools like e.g.,
[tcpdump](http://www.tcpdump.org/) or [wireshark](https://www.wireshark.org/).

When that data is sent over TLS, it's often times harder to use these tools
due to the private keys for the TLS sessions not being available to the person
doing the debugging.

A commonly used library for sending data over TLS is
[OpenSSL](http://openssl.org/).

openssl-hook uses LD_PRELOAD to hook the OpenSSL functions *SSL_read* and
*SSL_write* and writes the read/written data to disk for later inspection.

The logged data is written to *hooklog.bin* in the current working directory.
The file format is platform-dependent.

Errors are written to stderr and the application is terminated.

## Example

using bash and [socat](http://www.dest-unreach.org/socat/):

````
$ make
cc -Wall -Werror -g -fPIC -shared -o hook.so.1 hook.c -lc
cc -Wall -Werror -g -o print-hooklog print-hooklog.c -lc
$ echo -en 'GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n' | \
	LD_PRELOAD=`pwd`/hook.so.1 \
	socat stdio openssl-connect:www.google.com:443,verify=0
[...]
````

