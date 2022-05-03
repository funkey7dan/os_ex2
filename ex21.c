// Daniel Bronfman ***REMOVED***
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

void exitHandler(int f1, int f2) {
    close(f1);
    close(f2);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        perror("Incorrect number of arguments.");
        return -1;
    }
    char char1, char2; // bytes we will read into
    // try to open the files
    int file1, file2, similiarFlag = 0;
    ssize_t res1, res2;
    file1 = open(argv[1], O_RDONLY);
    file2 = open(argv[2], O_RDONLY);
    if (file1 == -1) {
        perror(argv[1]);
        return -1;
    }
    if (file2 == -1) {
        perror(argv[2]);
        close(file1);
        return -1;
    }

    // read all bytes in loop
    while (1) {
        res1 = read(file1, &char1, 1);
        res2 = read(file2, &char2, 1);
        if (res1 < 0) {
            perror("file read failure!");
            exitHandler(file1, file2);
            return -1;
        }
        if (res2 < 0) {
            perror("file read failure!");
            exitHandler(file1, file2);
            return -1;
        }
        // if both reads resulted in end of file, the files where identical
        if (res1 == 0 && res2 == 0) {
            exitHandler(file1, file2);
            // if we finished the files check if there was a minor difference
            return similiarFlag ? 3 : 1;
        }
        // if only one of the files ended before the other
        if (res1 == 0 || res2 == 0) {
            exitHandler(file1, file2);
            return 2;
        }
        // different chars read
        while (char1 != char2) {
            similiarFlag = 1;
            // check if case difference
            if (abs(char1 - char2) == 32) {
                break;
            }
            // if there is a newline or empty space scan to the next and compare
            if (char1 == ' ' || char1 == '\n') {
                res1 = read(file1, &char1, 1);
                if (res1 < 0) {
                    perror("Seek failure!");
                    exitHandler(file1, file2);
                    return -1;
                }
                continue;
            }
            if (char2 == ' ' || char2 == '\n') {
                res2 = read(file2, &char2, 1);
                if (res2 < 0) {
                    perror("Seek failure!");
                    exitHandler(file1, file2);
                    return -1;
                }
                continue;
            }
            // if different chars
            return 2;
        }
    }
}