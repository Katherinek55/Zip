#define _POSIX_C_SOURCE 200809L 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Use 16-bit code words */
#define NUM_CODES 65536

/* allocate space for and return a new string s+t */
char *strappend_str(char *s, char *t);

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c);

/* look for string s in the dictionary
 *  * return the code if found
 *   * return NUM_CODES if not found 
 *    */
unsigned int find_encoding(char *dictionary[], char *s);

/* write the code for string s to file */
void write_code(int fd, char *dictionary[], char *s);

/* compress in_file_name to out_file_name */
void compress(char *in_file_name, char *out_file_name);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: zip file\n");
        exit(1);
    }

    char *in_file_name = argv[1];
    char *out_file_name = strappend_str(in_file_name, ".zip");

    compress(in_file_name, out_file_name);

    /* have to free the memory for out_file_name since strappend_str malloc()'ed it */
    free(out_file_name);

    return 0;
}
/* allocate space for and return a new string s+t */
char *strappend_str(char *s, char *t)
{
    if (s == NULL || t == NULL)
    {
        return NULL;
    }

    /* reminder: strlen() doesn't include the \0 in the length*/
    int new_size = strlen(s) + strlen(t) + 1;
    char *result = (char *)malloc(new_size*sizeof(char));
    strcpy(result, s);
    strcat(result, t);

    return result;
}

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c)
{
    if (s == NULL)
    {
        return NULL;
    }

    /*reminder: strlen() doesn't include the \0 in the length*/
    int new_size = strlen(s) + 2;
    char *result = (char *)malloc(new_size*sizeof(char));
    strcpy(result, s);
    result[new_size-2] = c;
    result[new_size-1] = '\0';

    return result;
}

/* look for string s in the dictionary
 *  * return the code if found
 *   * return NUM_CODES if not found 
 *    */
unsigned int find_encoding(char *dictionary[], char *s)
{
    if (dictionary == NULL || s == NULL)
    {
        return NUM_CODES;
    }

    for (unsigned int i=0; i< NUM_CODES; ++i)
    {
        /* code words are added in order, so if we get to a NULL value 
 *          * we can stop searching */
        if (dictionary[i] == NULL)
        {
            break;
        }

        if (strcmp(dictionary[i], s) == 0)
        {
            return i;
        }
    }
    return NUM_CODES;
}

/* write the code for string s to file */
void write_code(int fd, char *dictionary[], char *s)
{
    if (dictionary == NULL || s == NULL)
    {
        return;
    }

    unsigned int code = find_encoding(dictionary, s);
    /*should never call write_code() unless s is in the dictionary*/
    if (code == NUM_CODES)
    {
        printf("Algorithm error!");
        exit(1);
    }

    /*cast the code to an unsigned short to only use 16 bits per code word in the output file*/
    unsigned short actual_code = (unsigned short)code;
    if (write(fd, &actual_code, sizeof(unsigned short)) != sizeof(unsigned short))
    {
        perror("write");
        exit(1);
    }
}
void compress(char *in_file_name, char *out_file_name) {
    if (in_file_name == NULL || out_file_name == NULL) {
        printf("Error: null pointer passed to compress.\n");
        return;
    }

    char *dict[NUM_CODES] = {NULL};
    int next_index = 256;

    for (int i = 0; i < 256; i++) {
        dict[i] = (char *)malloc(2 * sizeof(char));
        if (dict[i] == NULL) {
            printf("Error: failed to allocate memory.\n");
            return;
        }
        dict[i][0] = (char)i;
        dict[i][1] = '\0';
    }

    int in_fd = open(in_file_name, O_RDONLY);
    if (in_fd < 0) {
        perror("open");
        return;
    }

    int out_fd = open(out_file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (out_fd < 0) {
        perror("open");
        close(in_fd);
        return;
    }

    char *current_string = NULL;
    unsigned char current_char;
    int code_word;
    int read_size;

    read_size = read(in_fd, &current_char, sizeof(current_char));
    if (read_size > 0) {
        current_string = (char *)malloc(2 * sizeof(char));
        if (current_string == NULL) {
            printf("Error: failed to allocate memory.\n");
            close(in_fd);
            close(out_fd);
            return;
        }
        current_string[0] = (char)current_char;
        current_string[1] = '\0';
    }

    while ((read_size = read(in_fd, &current_char, sizeof(current_char))) > 0) {
        char *look = strappend_char(current_string, current_char);
        if (look == NULL) {
            printf("Error: failed to allocate memory.\n");
            free(current_string);
            close(in_fd);
            close(out_fd);
            return;
        }

        code_word = find_encoding(dict, look);
        if (code_word != NUM_CODES) {
            free(current_string);
            current_string = look;
        } else {
            write_code(out_fd, dict, current_string);

            if (next_index < NUM_CODES) {
                dict[next_index++] = strdup(look);
            }
            free(look);

            current_string = (char *)malloc(2 * sizeof(char));
            if (current_string == NULL) {
                printf("Error: failed to allocate memory.\n");
                close(in_fd);
                close(out_fd);
                return;
            }
            current_string[0] = (char)current_char;
            current_string[1] = '\0';
        }
    }

    if (current_string != NULL) {
        write_code(out_fd, dict, current_string);
        free(current_string);
    }

    close(in_fd);
    close(out_fd);

    for (int i = 0; i < NUM_CODES; i++) {
        free(dict[i]);
    }
}
