# xzdec-min
Stripped down version of `xzdec` from XZ Utils

## Rationale
The XZ file format is very prevalent. Even the files in the GitHub Arctic Code Vault are compressed with xz.
However, `xzdec` from XZ utils has been known to be hard to bootstrap.

Maybe this situation has changed (I for myself was able to find the correct switches to make it compile with
bootstrappable TinyCC on 32-bit Linux quite fast), but still the XZ Utils package is a collection of various
C and header files, of which only a small part is actually needed to build xzdec. And it tries to use
lots of userspace tools to build it and manage its shared library (which we do not want here as we only
want `xzdec`).

In case you find a compiler relevant in bootstrapping where this does *not* compile but which compiles
other reasonable packages (like gzip), feel free to open an issue or send a patch.

## Usage

If you do not build on 32-bit Linux (with glibc), you may have to edit `extracted/config.h` manually.
No autoconf bloat available here, just be a real man.

To compile, just set `CC` and run `build.sh`, which effectively is a one-liner

    cd extracted && $CC -I. *.c -o xzdec

and results in a `xzdec` binary. Building it statically should be straightforward, too, depending
on your toolchain.

In case you do not trust that the source is identical to what is shipped in `XZ utils 5.2.5`, you can
clear the `extracted` directory and run `extract.sh` to populate it again with the required files.
