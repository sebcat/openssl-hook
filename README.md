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
This can be used to debug network applications sending/receiving data using
a dynamically linked OpenSSL library.

The logged data is written to *hooklog.bin* in the current working directory.
The file format is platform-dependent.

Any encountered errors in the hook code gets written to *stderr*. Hook errors
causes application termination.

## Example

using a POSIX shell and [socat](http://www.dest-unreach.org/socat/):

````
$ make
cc -Wall -Werror -g -fPIC -shared -o hook.so.1 hook.c -lc
cc -Wall -Werror -g -o print-hooklog print-hooklog.c -lc
$ printf 'GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n' | \
	LD_PRELOAD=`pwd`/hook.so.1 \
	socat stdio openssl-connect:www.google.com:443
[...]
$ ./print-hooklog hooklog.bin
write - 192.168.0.8 39808 -> 83.255.235.89 443
00000000  47 45 54 20 2f 20 48 54  54 50 2f 31 2e 31 0d 0a  |GET / HTTP/1.1..|
00000010  48 6f 73 74 3a 20 77 77  77 2e 67 6f 6f 67 6c 65  |Host: www.google|
00000020  2e 63 6f 6d 0d 0a 0d 0a                           |.com....|
00000028

read  - 83.255.235.89 443 -> 192.168.0.8 39808
00000000  48 54 54 50 2f 31 2e 31  20 33 30 32 20 46 6f 75  |HTTP/1.1 302 Fou|
00000010  6e 64 0d 0a 43 61 63 68  65 2d 43 6f 6e 74 72 6f  |nd..Cache-Contro|
00000020  6c 3a 20 70 72 69 76 61  74 65 0d 0a 43 6f 6e 74  |l: private..Cont|
00000030  65 6e 74 2d 54 79 70 65  3a 20 74 65 78 74 2f 68  |ent-Type: text/h|
00000040  74 6d 6c 3b 20 63 68 61  72 73 65 74 3d 55 54 46  |tml; charset=UTF|
00000050  2d 38 0d 0a 4c 6f 63 61  74 69 6f 6e 3a 20 68 74  |-8..Location: ht|
00000060  74 70 73 3a 2f 2f 77 77  77 2e 67 6f 6f 67 6c 65  |tps://www.google|
00000070  2e 73 65 2f 3f 67 66 65  5f 72 64 3d 63 72 26 65  |.se/?gfe_rd=cr&e|
00000080  69 3d 64 4b 71 51 56 70  47 67 47 49 71 41 38 51  |i=dKqQVpGgGIqA8Q|
00000090  66 6c 6c 71 61 41 42 51  0d 0a 43 6f 6e 74 65 6e  |fllqaABQ..Conten|
000000a0  74 2d 4c 65 6e 67 74 68  3a 20 32 35 39 0d 0a 44  |t-Length: 259..D|
000000b0  61 74 65 3a 20 53 61 74  2c 20 30 39 20 4a 61 6e  |ate: Sat, 09 Jan|
000000c0  20 32 30 31 36 20 30 36  3a 33 36 3a 33 36 20 47  | 2016 06:36:36 G|
000000d0  4d 54 0d 0a 53 65 72 76  65 72 3a 20 47 46 45 2f  |MT..Server: GFE/|
000000e0  32 2e 30 0d 0a 0d 0a 3c  48 54 4d 4c 3e 3c 48 45  |2.0....<HTML><HE|
000000f0  41 44 3e 3c 6d 65 74 61  20 68 74 74 70 2d 65 71  |AD><meta http-eq|
00000100  75 69 76 3d 22 63 6f 6e  74 65 6e 74 2d 74 79 70  |uiv="content-typ|
00000110  65 22 20 63 6f 6e 74 65  6e 74 3d 22 74 65 78 74  |e" content="text|
00000120  2f 68 74 6d 6c 3b 63 68  61 72 73 65 74 3d 75 74  |/html;charset=ut|
00000130  66 2d 38 22 3e 0a 3c 54  49 54 4c 45 3e 33 30 32  |f-8">.<TITLE>302|
00000140  20 4d 6f 76 65 64 3c 2f  54 49 54 4c 45 3e 3c 2f  | Moved</TITLE></|
00000150  48 45 41 44 3e 3c 42 4f  44 59 3e 0a 3c 48 31 3e  |HEAD><BODY>.<H1>|
00000160  33 30 32 20 4d 6f 76 65  64 3c 2f 48 31 3e 0a 54  |302 Moved</H1>.T|
00000170  68 65 20 64 6f 63 75 6d  65 6e 74 20 68 61 73 20  |he document has |
00000180  6d 6f 76 65 64 0a 3c 41  20 48 52 45 46 3d 22 68  |moved.<A HREF="h|
00000190  74 74 70 73 3a 2f 2f 77  77 77 2e 67 6f 6f 67 6c  |ttps://www.googl|
000001a0  65 2e 73 65 2f 3f 67 66  65 5f 72 64 3d 63 72 26  |e.se/?gfe_rd=cr&|
000001b0  61 6d 70 3b 65 69 3d 64  4b 71 51 56 70 47 67 47  |amp;ei=dKqQVpGgG|
000001c0  49 71 41 38 51 66 6c 6c  71 61 41 42 51 22 3e 68  |IqA8QfllqaABQ">h|
000001d0  65 72 65 3c 2f 41 3e 2e  0d 0a 3c 2f 42 4f 44 59  |ere</A>...</BODY|
000001e0  3e 3c 2f 48 54 4d 4c 3e  0d 0a                    |></HTML>..|
000001ea
````

