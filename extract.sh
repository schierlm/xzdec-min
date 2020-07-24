#!/bin/sh -e
mkdir -p build extracted/lzma
cd build
wget -nc https://tukaani.org/xz/xz-5.2.5.tar.xz
test -d xz-5.2.5 || tar xfvJ xz-5.2.5.tar.xz
cd xz-5.2.5/src/liblzma
cp api/lzma.h check/check.c check/crc32_small.c check/crc64_small.c check/sha256.c \
	check/check.h common/block_decoder.c common/block_header_decoder.c \
	common/block_util.c common/common.c common/filter_common.c common/filter_decoder.c \
	common/filter_flags_decoder.c common/index.c common/index_hash.c \
	common/stream_decoder.c common/stream_flags_common.c common/stream_flags_decoder.c \
	common/vli_decoder.c common/vli_size.c common/block_decoder.h common/common.h \
	common/filter_common.h common/filter_decoder.h common/index.h common/stream_decoder.h \
	common/stream_flags_common.h delta/delta_common.c delta/delta_decoder.c \
	delta/delta_common.h delta/delta_decoder.h delta/delta_private.h lz/lz_decoder.c \
	lz/lz_decoder.h rangecoder/range_common.h rangecoder/range_decoder.h simple/arm.c \
	simple/armthumb.c simple/ia64.c simple/powerpc.c simple/simple_coder.c \
	simple/simple_decoder.c simple/sparc.c simple/x86.c simple/simple_coder.h \
	simple/simple_decoder.h simple/simple_private.h lzma/lzma2_decoder.c lzma/lzma_decoder.c \
	lzma/lzma2_decoder.h lzma/lzma_common.h lzma/lzma_decoder.h \
	../../../../extracted
cp api/lzma/*.h ../../../../extracted/lzma
cd ../common
cp sysdefs.h tuklib_common.h tuklib_config.h tuklib_integer.h ../../../../extracted
cd ../../../..
cp config.h xzdec.c extracted
patch -d extracted <patch.patch
echo Done.
