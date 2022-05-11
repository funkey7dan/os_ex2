// Daniel Bronfman ***REMOVED***
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>

#define CONF_MAX 150

int compareFiles(const char *work_dir, int csvFD, struct dirent *pDirent,char* correctPath) {
    int gcp1, // grand child process
    status;
    gcp1 = fork();
    if (gcp1 == -1) {
        perror("Sub-Fork failed!\n");
        return -1;
    }
        // if the fork is successful, use comp.out
    else if (gcp1 == 0) {
        chdir(work_dir);
        char *args2[] = {"./comp.put", "output.txt", correctPath , NULL};
        if (execvp("./comp.out", args2) != 0) {
            perror("execvp failed when trying to run comparison");
            return -1;
        }
    }
        // after the execution of compare
    else {
        wait(&gcp1);
        // get the return value of compare
        status = WEXITSTATUS(gcp1);
        switch (status) {
            // files are identical
            case 1:
                if (write(csvFD, pDirent->d_name, strlen(pDirent->d_name)) < 0) {
                    return -1;
                }
                if (write(csvFD, ",100,EXCELLENT\n", strlen(",100,EXCELLENT\n")) <
                    0) {
                    return -1;
                }
                break;
                // files are similar
            case 3:
                if (write(csvFD, pDirent->d_name, strlen(pDirent->d_name)) < 0) {
                    return -1;
                }
                if (write(csvFD, ",75,SIMILAR\n", strlen(",75,SIMILAR\n")) < 0) {
                    return -1;
                }
                break;
            // wrong output
            default:
                if (write(csvFD, pDirent->d_name, strlen(pDirent->d_name)) < 0) {
                    return -1;
                }
                if (write(csvFD, ",50,WRONG\n", strlen(",50,WRONG\n")) < 0) {
                    return -1;
                }
                break;
        }
    }
    return 0;
}

int compileFile(struct dirent *sdDirent, int *gcp1, const char *sd_name) {
    *(gcp1) = fork();
    if (*(gcp1) == -1) {
        perror("Sub-Fork failed!\n");
        return -1;
    }
        // if in child
    else if (*gcp1 == 0) {
        chdir(sd_name);
        char *args[] = {"gcc", "-w", "-std=c99", sdDirent->d_name, "-o", "compiled.out", NULL};
        if (execv("/usr/bin/gcc", args) == 1) {
            perror("Error in: GCC");
        } else if (execv("/usr/bin/gcc", args) < 0) {
            perror("Error in: exec()");
            return -1;
        }
    } else wait(gcp1);
    return 0;
}

int runCompiled(int *gcp1, const char *sd_name) {
    if ((*(gcp1) = fork()) < 0) {
        perror("Error in: fork()");
        return -1;
    }
        // if fork was successful, compile the files
    else if (*(gcp1) == 0) {

        chdir(sd_name);
        {
            char *args[] = {"compiled.out", NULL};
            if (execv("./compiled.out", args) != 0) {
                perror("Error in: execv()");
            }
        }
    } else wait(gcp1);
    return 0;
}

int runStud(int csvFD, const char *sd_name, DIR *sd, const char *work_dir, struct dirent *pDirent,char* correctPath) {
//    chdir(sd_name);
    int gcp1; // grand child process
    int foundFlag = 0;
    struct dirent *sdDirent;
    while ((sdDirent = readdir(sd)) != NULL) {
        // if not a folder
        if (sdDirent->d_type != DT_DIR) {
            // get file extension
            char *extension = strrchr(sdDirent->d_name, '.');
            // if we found a C file
            if (!strcmp(extension, ".c")) {
                foundFlag = 1;
                if(compileFile(sdDirent, &gcp1, sd_name)<-1)
                    return -1;
                // parent process

                // after the compilation fork finishes
                int status = WEXITSTATUS(gcp1);
                // if status is 0, compilation successful, run the compiled file.
                if (!status) {
                    if(runCompiled(&gcp1, sd_name)<0){
                        return -1;
                    }
                    // parent process
                    wait(&gcp1);
                    // try to fork again to execute compare
                    if(compareFiles(work_dir, csvFD, pDirent,correctPath)<0) return -1;
                }
                    // if gcc compilation error
                else if (status == 1) {
                    // write student name
                    if (write(csvFD, pDirent->d_name, strlen(pDirent->d_name)) < 0) {
                        return -1;
                    }
                    if (write(csvFD, ",10,COMPILATION_ERROR\n", strlen(",10,COMPILATION_ERROR\n")) <
                        0) {
                        return -1;
                    }
                }
                break;
            }
        }
    }
    if (!foundFlag) {
        if (write(csvFD, pDirent->d_name, strlen(pDirent->d_name)) < 0) {
            return -1;
        }
        if (write(csvFD, ",0,NO_C_FILE\n", strlen(",0,NO_C_FILE\n")) < 0) {
            return -1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    // check number of arguments
    if (argc != 2) {
        write(2,"Incorrect number of arguments.\n", strlen("Incorrect number of arguments.\n"));
        return -1;
    }
    int errorRd, inRd, outRd, csvFD;
    // try to redirect error output to file, and the file output to a file
    if ((errorRd = open("errors.txt", O_CREAT | O_APPEND | O_RDWR | O_TRUNC, 0777)) < 0) {
        perror("Error in: open()");
    }

    // get the current working directory
    char work_dir[PATH_MAX];
    if (getcwd(work_dir, sizeof(work_dir)) != NULL) {
    } else {
        perror("Error in: getcwd()");
        return 1;
    }
    dup2(errorRd, 2);
    char fileContents[3*CONF_MAX], pathArr[3][CONF_MAX];

    // try to open the path passed as argument (the path to the folder)
    int confFD = open(argv[1], O_RDONLY);
    if (confFD == -1) {
        perror(argv[1]);
        return -1;
    }
    int i = 0;
    const char *delim = "\n";
    char *token = NULL;
    // try to read the contents of the configuration files
    ssize_t res1 = read(confFD, fileContents, 12291);
    if (res1 < 0) {
        perror("Error in: read()");
        close(confFD);
        return -1;
    }
    // separate the lines of the file delimited by \n, into the actual passed paths.
    token = strtok(fileContents, delim);
    strcpy(pathArr[i], token);
    i++;
    while (token != NULL) {
        token = strtok(NULL, delim);
        if (!token) break;
        strcpy(pathArr[i], token);
        i++;
    }
    // check if passed folders are relative or absolute
    for(i=2;i>=0;--i){
        if(!access(pathArr[i],F_OK)){
            char temp[PATH_MAX];
            strcpy(temp,work_dir);
            //build full path
            strcat(temp, "/");
            strcat(temp,pathArr[i] );
            strcpy(pathArr[i],temp);
        }
    }

    for(i=2;i>=0;--i){
        if(access(pathArr[i],F_OK)){
            perror("Folder doesn't exist");
            return -1;
        }
    }

    // open the directory with the students folders
    DIR *d = opendir(pathArr[0]);


    if ((csvFD = open("results.csv", O_WRONLY | O_CREAT, 0777)) < 0) {
        perror("Error in: open()");
        return -1;
    }

    DIR *sd = NULL; //subdirectory
    struct dirent *pDirent;
    // iterate over the content of the main directory - the directories containing student folders
    while ((pDirent = readdir(d)) != NULL) {

        // ignore self and parent directories
        if ((strcmp(pDirent->d_name, ".") != 0) && (strcmp(pDirent->d_name, "..") != 0)) {
            // change the output stream to file
            if ((outRd = open("output.txt", O_CREAT | O_RDWR | O_TRUNC, 0777)) < 0) {
                perror("Error in: open()");
                return -1;
            }
            dup2(outRd, 1);
            // redirect the  input to the file passed in the configuration
            if ((inRd = open(pathArr[1], O_RDONLY)) < 0) {
                perror("Error in: open()");
                return -1;
            }
            dup2(inRd, 0);
            close(outRd);
            //char *sd_name = (char *) malloc(NAME_MAX);
            char sd_name[NAME_MAX] ;
            // add the full path to the directory name
            strcpy(sd_name, pathArr[0]);
            strcat(sd_name, "/");
            strcat(sd_name, pDirent->d_name);

            // open the subdirectory
            sd = opendir(sd_name);
            // fork
            runStud(csvFD, sd_name, sd, work_dir, pDirent,pathArr[2]);
            //free(sd_name);
            closedir(sd);
            remove("output.txt");
        }

    }

    close(csvFD);
    close(errorRd);
    close(inRd);
    close(outRd);
    close(confFD);
    closedir(d);
    return 0;
}


