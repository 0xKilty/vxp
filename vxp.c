#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <zlib.h>

enum Scheme {
    PACKING,
    UNPACKING
};

void print_usage() {
    fprintf(stderr, "Usage: vxp <option (-p/-u)> <file>\n");
    exit(EXIT_FAILURE);
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

    *fd = open(argv[2], O_RDONLY);
    if (*fd < 0) {
        fprintf(stderr, "Can not open file %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    enum Scheme scheme;
    int fd;
    parse_inputs(&scheme, &fd, argc, argv);

    

    return 0;
}