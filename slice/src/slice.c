/*
**  @(#)slice.c
**
**  slice - slice text file utility functions
**  -----------------------------------------
**
**  copyright (c) 2020 Code Construct Systems (CCS)
*/
#include "modules.h"

/*
** Unwind buffer stack pointer
*/
jmp_buf unwind_buffer_sp;

/*
** Input file stream
*/
static FILE *input_file;

/*
** Input file chunk buffer
*/
static char *input_file_chunk_buffer;

/*
** Local function prototypes
*/
static void SetSystemSignals(void);
static void InterruptHandler(int);
static void ProcessFile(options_t *opts);

/*
** Slice text file utility driver
*/
int main(int argc, string_c_t argv[]) {
    options_t opts;

    /*
    ** Set system signals and if stack was unwound, free file chunk buffer and exit
    */
    SetSystemSignals();
    if (setjmp(unwind_buffer_sp) != 0) {
        fclose_p(input_file);
        free(input_file_chunk_buffer);
        return (EXIT_FAILURE);
    }

    /*
    ** Get options
    */
    GetOptions(argc, argv, &opts);

    /*
    ** Process file
    */
    ProcessFile(&opts);

    /*
    ** Close input file
    */
    fclose_p(input_file);
    
    /*
    ** Free allocated memory for file chunk buffer
    */
    free(input_file_chunk_buffer);

    /*
    ** Return success return code
    */
    return (EXIT_SUCCESS);
}

/*
** Set system signals
*/
static void SetSystemSignals(void) {
    signal(SIGINT, InterruptHandler);
    signal(SIGTERM, InterruptHandler);
}

/*
** Interrupt handler
*/
static void InterruptHandler(int signal_number) {
    printf("signal detected (%d)!\n", signal_number);
    EXIT_APPLICATION(EXIT_SUCCESS);
}

/*
** Process file
*/
static void ProcessFile(options_t *opts) {
    size_t input_file_chunk_buffer_size = opts->chunk_size * 1024, bytes_read, part = 0;
    char output_file_name[_MAX_FILE_NAME_SIZE + 1];
    FILE *output_file;
    string_c_t base_name = NULL;

    /*
    ** Open input file
    */
    fopen_p(&input_file, opts->input_file_name, (string_c_t)_F_RO_BIN);
    if (input_file == NULL) {
        printf("error-> unable to open input file: %s (%d)\n", opts->input_file_name, errno);
        EXIT_APPLICATION(EXIT_FAILURE);
    }

    /*
    ** Allocate memory for file chunk buffer
    */
    if ((input_file_chunk_buffer = (char *)malloc(input_file_chunk_buffer_size + 1)) == NULL) {
        printf("error-> memory allocation failed (%d)\n", errno);
        EXIT_APPLICATION(EXIT_FAILURE);
    }

    /*
    ** Process each byte from input file until either I/O error or when at end of file (EOF)
    */
    for (;;) {
        memset(input_file_chunk_buffer, 0, input_file_chunk_buffer_size);

        /*
        ** Read chunk of bytes from input file
        */
        bytes_read = fread(input_file_chunk_buffer, 1, input_file_chunk_buffer_size, input_file);
        if (bytes_read > 0) {
            memset(output_file_name, 0, sizeof(output_file_name));

            /*
            ** Format output file name using output file path, input file name and part number; else format using input file name and part number
            */
            if (strlen(opts->output_file_path)) {
                basename_p(opts->input_file_name, &base_name);
                strfmt_p(output_file_name, _MAX_FILE_NAME_SIZE, _BASE_NAME_PART_NUMBER_FORMAT, opts->output_file_path, base_name, ++part);
            }
            else {
                strfmt_p(output_file_name, _MAX_FILE_NAME_SIZE, _FILE_NAME_PART_NUMBER_FORMAT, opts->input_file_name, ++part);
            }

            /*
            ** Show file names (if required)
            */
            if (opts->show_file_names == TRUE) {
                printf("%s\n", output_file_name);
            }

            /*
            ** Open output file
            */
            fopen_p(&output_file, output_file_name, (string_c_t)_F_RW_BIN);
            if (output_file == NULL) {
                printf("error-> unable to open output file: %s (%d)\n", output_file_name, errno);
                EXIT_APPLICATION(EXIT_FAILURE);
            }

            /*
            ** Write chunk of bytes to output file and close output file
            */
            fwrite(input_file_chunk_buffer, 1, bytes_read, output_file);
            
            /*
            ** Check for output file write error
            */
            if (ferror(output_file)) {
                printf("error-> unable to write to output file: %s (%d)\n", output_file_name, errno);
                fclose(output_file); 
                EXIT_APPLICATION(EXIT_FAILURE);
            }
    
            /*
            ** Close output file
            */
            fclose(output_file);    
        }
        else {
            break;
        }
    }
}