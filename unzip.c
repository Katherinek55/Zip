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

/* allocate space for and return a new string s+c */
char *strappend_char(const char *s, char c);

/* read the next code from fd
 *  * return NUM_CODES on EOF
 *   * return the code read otherwise
 *    */
unsigned int read_code(int fd);

/* uncompress in_file_name to out_file_name */
void uncompress(char *in_file_name, char *out_file_name);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: unzip file\n");
        exit(1);
    }

    char *in_file_name = argv[1];
    char *out_file_name = strdup(in_file_name);
    out_file_name[strlen(in_file_name)-4] = '\0';

    uncompress(in_file_name, out_file_name);

    free(out_file_name);

    return 0;
}
char *strappend_char(const char *s, char c)
{
    if (s == NULL)
    {
        return NULL;
    }

    int new_size = strlen(s) + 2;
    char *result = (char *)malloc(new_size * sizeof(char));
    strcpy(result, s);
    result[new_size - 2] = c;
    result[new_size - 1] = '\0';

    return result;
}

/* read the next code from fd
 *  * return NUM_CODES on EOF
 *   * return the code read otherwise
 *    */
unsigned int read_code(int fd)
{
    /*code words are 16-bit unsigned shorts in the file*/
    unsigned short actual_code;
    int read_return = read(fd, &actual_code, sizeof(unsigned short));
    if (read_return == 0)
    {
        return NUM_CODES;
    }
    if (read_return != sizeof(unsigned short))
    {
       perror("read");
       exit(1);
    }
    return (unsigned int)actual_code;
}

/* uncompress in_file_name to out_file_name */
void uncompress(char *in_file_name, char *out_file_name) {
    int in_fd = open(in_file_name, O_RDONLY);
    if (in_fd < 0) {
        perror("open");
        return;
    }

    int out_fd = open(out_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd < 0) {
        perror("open");
        close(in_fd);
        return;
    }

    char *dict[NUM_CODES] = {NULL};
    for (int i = 0; i < 256; i++) {
        dict[i] = (char *)malloc(2 * sizeof(char));
        if (dict[i] == NULL) {
            printf("Error: failed to allocate memory.\n");
            for (int j = 0; j < i; j++) {
                free(dict[j]);
            }
            close(in_fd);
            close(out_fd);
            return;
        }
        dict[i][0] = (char)i;
        dict[i][1] = '\0';
    }

    unsigned int current_code = read_code(in_fd);
    if (current_code == NUM_CODES) {
        close(in_fd);
        close(out_fd);
        return;
    }

    char *current_string = dict[current_code];
    write(out_fd, current_string, strlen(current_string));
    char current_char = current_string[0];

    unsigned int next_code = 0;
    int dict_size = 256;
    while (1) {
        next_code = read_code(in_fd);
        if (next_code == NUM_CODES) {
            break;
        }

        char *next_string = NULL;
        if (next_code < dict_size) {
            next_string = dict[next_code];
        } else if (next_code == dict_size) {
            next_string = strappend_char(current_string, current_char);
            if (next_string == NULL) {
                printf("Error: failed to allocate memory.\n");
                for (int i = 0; i < dict_size; i++) {
                    free(dict[i]);
                }
                close(in_fd);
                close(out_fd);
                return;
            }
        } else {
            printf("Error: invalid code %u\n", next_code);
            for (int i = 0; i < dict_size; i++) {
                free(dict[i]);
            }
            close(in_fd);
            close(out_fd);
            return;
        }

        write(out_fd, next_string, strlen(next_string));
        current_char = next_string[0];

        if (dict_size < NUM_CODES) {
            char *new_string = strappend_char(current_string, current_char);
            if (new_string == NULL) {
                printf("Error: failed to allocate memory.\n");
                for (int i = 0; i < dict_size; i++) {
                    free(dict[i]);
                }
                close(in_fd);
                close(out_fd);
                return;
            }
            dict[dict_size++] = new_string;
        }

        if (next_code == dict_size - 1) {
            free(next_string);
        }

        current_string = next_string;
    }

    for (int i = 0; i < dict_size; i++) {
        free(dict[i]);
    }
    close(in_fd);
    close(out_fd);
}
