#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include <iostream>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

// You must allocate enough memory to *output.
int compress(char *input, int input_length, char *output, int *output_length) {
    std::cout << "compress start" << std::endl;
    int ret;
    z_stream strm;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_BEST_COMPRESSION);
    if (ret != Z_OK)
        return ret;

    strm.avail_in = input_length;
    strm.next_in = (Bytef *)input;
    strm.avail_out = input_length;
    strm.next_out = (Bytef *)output;
    ret = deflate(&strm, Z_FINISH); /* no bad return value */
    assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
    assert(ret == Z_STREAM_END);    /* stream will be complete */
    *output_length = input_length - strm.avail_out;

    /* clean up and return */
    (void)deflateEnd(&strm);
    std::cout << "compress end" << std::endl;
    return Z_OK;
}

// You must allocate enough memory to *output.
int decompress(char *input, int input_length, char *output,
               int *output_length) {
    int ret;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    strm.avail_in = input_length;
    strm.next_in = (Bytef *)input;
    strm.avail_out = input_length;
    strm.next_out = (Bytef *)output;

    ret = inflate(&strm, Z_NO_FLUSH);
    assert(ret != Z_STREAM_ERROR); /* state not clobbered */
    switch (ret) {
    case Z_NEED_DICT:
        ret = Z_DATA_ERROR; /* and fall through */
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        (void)inflateEnd(&strm);
        return ret;
    }

    *output_length = input_length - strm.avail_out;

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void zerr(int ret) {
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}