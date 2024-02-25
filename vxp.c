#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <zlib.h>
#include <elf.h>

#define COMPRESSION_CHUNK 16384

enum Scheme {
    PACKING,
    UNPACKING
};

void print_usage() {
    fprintf(stderr, "Usage: vxp <option (-p/-u)> <file>\n");
    exit(EXIT_FAILURE);
}

void seek_to_start(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        fprintf(stderr, "Error seeking to the start of the file\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
}

long get_file_size(int fd) {
    long res = lseek(fd, 0, SEEK_END);
    if (res == -1) {
        fprintf(stderr, "Error reading size of file\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    seek_to_start(fd);
    
    return res;
}

int open_file(char* file_name) {
    int fd = open(file_name, O_RDONLY);   
    if (fd < 0) {
        fprintf(stderr, "Can not open file %s\n", file_name);
        exit(EXIT_FAILURE);
    }
    return fd;
}

void read_file(int fd, long size, char* buffer) {
    if (read(fd, buffer, size) == -1) {
        fprintf(stderr, "Error reading file\n");
        free(buffer);
        close(fd);
        exit(EXIT_FAILURE);
    }
}

void parse_inputs(enum Scheme *scheme, int *fd, int argc, char** argv) {
    if (argc != 3)
        print_usage();
    else if (strcmp(argv[1], "-p") == 0)
        *scheme = PACKING;
    else if (strcmp(argv[1], "-u") == 0)
        *scheme = UNPACKING;
    else
        print_usage();

    *fd = open_file(argv[2]);
}

Elf64_Ehdr read_elf_header(int fd) {
    Elf64_Ehdr elf_header;
    size_t bytesRead = read(fd, &elf_header, sizeof(Elf64_Ehdr));
    if (bytesRead != sizeof(Elf64_Ehdr)) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (!valid_elf_magic(elf_header)) {
        fprintf(stderr, "Not an ELF file\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return elf_header;
}

void init_z_stream(z_stream *stream) {
    stream->zalloc = Z_NULL;
    stream->zfree = Z_NULL;
    stream->opaque = Z_NULL;
}

void set_up_buffers(z_stream *stream, unsigned int avail_in, const char* next_in, char** next_out) {
    stream->avail_in = (uInt)avail_in;
    stream->next_in = (Bytef*)next_in;
    stream->avail_out = COMPRESSION_CHUNK;
    stream->next_out = (Bytef*)*next_out;
}

int compress_data(const char* input, size_t inputSize, char** compressedData, size_t* compressedSize) {
    z_stream stream;
    init_z_stream(&stream);

    int ret = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        fprintf(stderr, "Error initializing compression stream\n");
        return ret;
    }

    *compressedData = (char*)malloc(COMPRESSION_CHUNK);
    if (*compressedData == NULL) {
        fprintf(stderr, "Error allocating memory for compressed data\n");
        deflateEnd(&stream);
        return Z_MEM_ERROR;
    }

    set_up_buffers(&stream, inputSize, input, compressedData);

    do {
        ret = deflate(&stream, Z_FINISH);
        if (ret == Z_OK || ret == Z_BUF_ERROR) {
            *compressedData = (char*)realloc(*compressedData, COMPRESSION_CHUNK + stream.total_out);
            if (*compressedData == NULL) {
                fprintf(stderr, "Error reallocating memory for compressed data\n");
                deflateEnd(&stream);
                return Z_MEM_ERROR;
            }

            stream.next_out = (Bytef*)(*compressedData + stream.total_out);
            stream.avail_out = COMPRESSION_CHUNK;
        } else if (ret != Z_STREAM_END) {
            fprintf(stderr, "Error during compression: %s\n", stream.msg);
            deflateEnd(&stream);
            free(*compressedData);
            return ret;
        }
    } while (ret != Z_STREAM_END);

    *compressedSize = stream.total_out;

    deflateEnd(&stream);

    return Z_OK;
}

int main(int argc, char** argv) {
    enum Scheme scheme;
    int x_fd;
    parse_inputs(&scheme, &x_fd, argc, argv);

    long x_size = get_file_size(x_fd);
    char *x_buffer = (char *)malloc(x_size);
    read_file(x_fd, x_size, x_buffer);

    Elf64_Ehdr elf_header = read_elf_header(x_fd);

    /*
    char* compressed_data = NULL;
    size_t compressed_size = 0;
    int compressed_result = compress_data(x_buffer, x_size, &compressed_data, &compressed_size);
    if (compressed_result != Z_OK) {
        fprintf(stderr, "Compression failed with error code %d\n", compressed_result);
        exit(EXIT_FAILURE);
    }
    */
    close(x_fd);
    free(x_buffer);
    exit(EXIT_SUCCESS);
}