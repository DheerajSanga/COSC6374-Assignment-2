#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "stb_image.h"
#include "stb_image_write.h"
#include <ctype.h>
#define MESSAGE_SIZE 512

void hide_message(char* image_path, char* output_path, char* message) {
    int width, height, channels;
    unsigned char *image = stbi_load(image_path, &width, &height, &channels, 0);
    if (!image) {
        printf("Error loading image %s\n", image_path);
        return;
    }
    
    if (channels < 3) {
        printf("Image must have at least 3 channels (RGB)\n");
        stbi_image_free(image);
        return;
    }
    strcat(message, "\0\0");
    int message_length = strlen(message);
    if (message_length * 8 > width * height * 3) {
        printf("Message too large to hide in image\n");
        stbi_image_free(image);
        return;
    }
    
    int message_idx = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (message_idx >= message_length * 8) {
                break;
            }
            
            unsigned char* pixel = image + (i * width + j) * channels;
            for (int k = 0; k < 3; k++) {
                if (message_idx >= message_length * 8) {
                    break;
                }
                
                int bit = (message[message_idx / 8] >> (message_idx % 8)) & 1;
                pixel[k] = (pixel[k] & ~1) | bit;
                message_idx++;
            }
        }
    }
    
    int result = stbi_write_png(output_path, width, height, channels, image, width * channels);
    if (!result) {
        printf("Error writing output image %s\n", output_path);
    }
    
    stbi_image_free(image);
}

char* extract_message(char* image_path) {
    int width, height, channels;
    unsigned char *image = stbi_load(image_path, &width, &height, &channels, 0);
    if (!image) {
        printf("Error loading image %s\n", image_path);
        return NULL;
    }
    
    if (channels < 3) {
        printf("Image must have at least 3 channels (RGB)\n");
        stbi_image_free(image);
        return NULL;
    }
    
    char* message = malloc(MESSAGE_SIZE);
    memset(message, 0, MESSAGE_SIZE);
    
    int message_idx = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (message_idx >= MESSAGE_SIZE * 8) {
                break;
            }
            
            unsigned char* pixel = image + (i * width + j) * channels;
            for (int k = 0; k < 3; k++) {
                if (message_idx >= MESSAGE_SIZE * 8) {
                    break;
                }
                
                int bit = pixel[k] & 1;
                message[message_idx / 8] |= (bit << (message_idx % 8));
                message_idx++;
            }
        }
    }
    
    stbi_image_free(image);
    
    return message;
}

void createwm(char *inputdir, char *peoplelist, char *outputdir){
    DIR *dir;
    struct dirent *entry;
    char *ext;

    dir = opendir(inputdir);

    if(!dir) {
        printf("Error: Failed to open directory %s\n", inputdir);
        exit(1);;
    }

    if (access(outputdir, F_OK) == -1) {
        if (mkdir(outputdir, 0777) == -1) {
            printf("Failed to create directory.\n");
            exit(1);
        }

        printf("Directory %s created successfully.\n", outputdir);
    }

    FILE *fp;
    char line[MESSAGE_SIZE];
    
    fp = fopen(peoplelist, "r");
    
    if(fp == NULL) {
        printf("Error: Failed to open file. %s\n", peoplelist);
        exit(1);
    }    
    
    while((entry = readdir(dir))) {
        ext = strrchr(entry->d_name, '.');
        if(ext && (!strcmp(ext, ".jpg") ||!strcmp(ext, ".png")) ) {
            int count = 0;
            while(fgets(line, MESSAGE_SIZE, fp)) {
                // Hide message in image
                char input[256], output[256];
                sprintf(input, "%s/%s", inputdir, entry->d_name);
                sprintf(output, "%s/%d%s", outputdir, count, entry->d_name);
                hide_message(input, output, line);
                count += 1;
            }
            /*Begining of the file*/
            fseek(fp, 0, SEEK_SET);
        }
    }

    fclose(fp);
}

void verifywm(char *inputdir){
    DIR *dir;
    struct dirent *entry;
    char *ext;

    dir = opendir(inputdir);

    if(!dir) {
        printf("Error: Failed to open directory %s\n", inputdir);
        exit(1);
    }
    
    while((entry = readdir(dir))) {
        ext = strrchr(entry->d_name, '.');
        if(ext && (!strcmp(ext, ".jpg") ||!strcmp(ext, ".png")) ) {
            char str[256];
            sprintf(str, "%s/%s", inputdir, entry->d_name);
            // Extract message from image
            char* extracted_message = extract_message(str);
            if(isalnum(extracted_message[0]))
                printf("%s -> %s\n", entry->d_name, extracted_message);
            else
                printf("%s -> No Watermark Detected\n", entry->d_name);
            free(extracted_message);
        }
    }
}
void createwm_help(){
    printf("\tstegatool createwm -inputdir INPUT-FILES-DIR -peoplelist PEOPLE-LIST-FILE -outputdir OUTPUT-FILES-DIR\n");
}

void verifywm_help() {
    printf("\tstegatool verifywm -inputdir INPUT-FILES-DIR\n");
}

void help(){
    printf("Usage: \n");
    createwm_help();
    verifywm_help();
    exit(1);
}


int main(int argc, char* argv[]) {

    if(argc < 2){
        help();
    }

    if(strcmp(argv[1], "createwm") == 0){
        if(argc == 8){
            char *inputdir = NULL;
            char *peoplelist = NULL;
            char *outputdir = NULL;

            // Parse command line arguments
            for (int i = 2; i < argc; i++) {
                if (strcmp(argv[i], "-inputdir") == 0) {
                    if (i + 1 < argc) {
                        inputdir = argv[i + 1];
                    }
                } else if (strcmp(argv[i], "-peoplelist") == 0) {
                    if (i + 1 < argc) {
                        peoplelist = argv[i + 1];
                    }
                } else if (strcmp(argv[i], "-outputdir") == 0) {
                    if (i + 1 < argc) {
                        outputdir = argv[i + 1];
                    }
                }
            }

            // Check if all required arguments are present
            if (inputdir == NULL || peoplelist == NULL || outputdir == NULL) {
                createwm_help();
                return 1;
            }

            createwm(inputdir, peoplelist, outputdir);
        }
        else {
            createwm_help();
            return 1;
        }
    }
    else if(strcmp(argv[1], "verifywm") == 0) {
        if(argc == 4){
            char *inputdir = NULL;

            // Parse command line arguments
            for (int i = 2; i < argc; i++) {
                if (strcmp(argv[i], "-inputdir") == 0) {
                    if (i + 1 < argc) {
                        inputdir = argv[i + 1];
                    }
                }
            }

            // Check if all required arguments are present
            if (inputdir == NULL) {
                verifywm_help();
                return 1;
            }

            verifywm(inputdir);

        }
        else{
            verifywm_help();
            return 1;
        }
    }
    else {
        help();
    }

    return 0;
}