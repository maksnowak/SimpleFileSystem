#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    char command[256];
    char filename[256] = "";
    char source[256];
    char destination[256];
    int blocks;
    printf("SOI Lab 6 - File System\n\n");
    printf("Available commands:\n");
    printf("init\n");
    printf("select\n");
    printf("copy\n");
    printf("get\n");
    printf("list\n");
    printf("remove\n");
    printf("info\n");
    printf("defrag\n");
    printf("delete\n");
    printf("exit\n\n\n");
    while (1) {
        printf("%s> ", filename);
        scanf("%255s", command);
        if (strcmp(command, "init") == 0) {
            printf("Enter the name of the file system to create: ");
            scanf("%255s", filename);
            printf("Enter the number of data blocks in the file system: ");
            scanf("%d", &blocks);
            init_fs(filename, blocks);
        } else if (strcmp(command, "select") == 0) {
            printf("Enter the name of the file system to select: ");
            scanf("%255s", filename);
            select_fs(filename);
        } else if (strcmp(command, "copy") == 0) {
            printf("Enter the name of the file to copy to the file system: ");
            scanf("%255s", source);
            copy_file_to_fs(filename, source);
        } else if (strcmp(command, "get") == 0) {
            printf("Enter the name of the file to get from the file system and the destination filename: ");
            scanf("%255s %255s", source, destination);
            copy_file_from_fs(filename, source, destination);
        } else if (strcmp(command, "list") == 0) {
            list_files(filename);
        } else if (strcmp(command, "remove") == 0) {
            printf("Enter the name of the file to remove from the file system: ");
            scanf("%255s", source);
            delete_file(filename, source);
        } else if (strcmp(command, "info") == 0) {
            usage_map(filename);
        } else if (strcmp(command, "defrag") == 0) {
            defragment_fs(filename);
        } else if (strcmp(command, "delete") == 0) {
            printf("Enter the name of the file system to delete: ");
            char current_filename[256];
            strcpy(current_filename, filename);
            scanf("%255s", filename);
            delete_fs(filename);
            if (strcmp(current_filename, filename) == 0)
                filename[0] = '\0';
            else
                strcpy(filename, current_filename);
        } else if (strcmp(command, "exit") == 0) {
            break;
        } else {
            printf("Unknown command: %s\n", command);
            printf("Available commands:\n");
            printf("init\n");
            printf("select\n");
            printf("copy\n");
            printf("get\n");
            printf("list\n");
            printf("remove\n");
            printf("info\n");
            printf("defrag\n");
            printf("delete\n");
            printf("exit\n");
        }
    }

    return 0;
}