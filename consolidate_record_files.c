
#include "vos_util.h"

long get_records_count(char_varying(256) input_file, short int *output_error_code);
void create_separate_key_index(char_varying(256) output_file, char_varying(32) index_name, short int disable_duplicates, short int dae, short int extent_size, short int *output_error_code);
short int isPowerOfTwo(short int numToTest);
void doConsolidate(short int key_input_port, short int record_input_port, short int output_port, char_varying(32) index_name, short int pad_index, short int pad_records, short int *output_error_code);
void format_padding(short int input_number, char *buffer, int buffer_size);

char_varying(32) caller_name = "consolidate_record_files"; /* Our program's name */
char_varying(32) error_message;

int main(int argc, char *argv[])
{
    short int pad_index = 0; /* Pad indexes with blank ones if required due to length mismatch */
    short int pad_records = 0; /* Pad records with blank ones if required due to length mismatch */
    short int dae_file = 0; /* Create a DAE file? */
    short int output_organization = 0; /* File organization of the output file */
    short int output_record_size = 0; /* Record size of output file. Ignore for sequential files. */
    short int disable_duplicates = 0; /* explicitly disable duplicate keys in index */

    short int key_input_port = 0; /* VOS I/O is port-based. We use these to hold the port ID. */
    short int record_input_port = 0;
    short int output_port = 0;
    short int error_code = 0; /* Error code. We use this for most VOS syscalls. */
    short int port_switches = 0; /* This allows us to do stuff like reserving a port for future use. We don't do that, but the API expects it. */
    short int extent_size = 0; /* For use with DAE files */

    char_varying(256) message_file_path = ""; /* message file is program name, same dir */
    char_varying(256) key_input_file; /* Pull keys from this structured file */
    char_varying(256) record_input_file; /* And records from this one */
    char_varying(256) output_file; /* And put it all together in here */
    char_varying(256) key_input_file_desc; /* Descriptor for key_input_file */
    char_varying(256) record_input_file_desc; /* Descriptor for record_input_file */

    char_varying(256) output_file_desc; /* Descriptor for output_file */
    char_varying(256) pad_index_desc; /* Descriptor for pad_index */
    char_varying(256) pad_records_desc; /* Descriptor for pad_records */
    char_varying(256) dae_file_desc; /* Descriptor for dae_file */

    char_varying(256) disable_duplicates_desc; /* Descriptor for allow_duplicates */
    char_varying(256) end_descriptor = "end"; /* Tell s$parse_command that we're finished */
    char_varying(256) port_name; /* We never do anything with port names, but the API expects it. */
    char_varying(32) index_name; /* Name of the index to create */
    char_varying(256) index_name_desc; /* Descriptor for index_name */

    char_varying(256) extent_size_desc; /* Descriptor for extent_size */

    long key_records_count;
    long record_records_count;

    s$use_cmd_message_file(&message_file_path, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
    }

    key_input_file_desc = "key_input_file:pathname,req"; /* Require this. Positional. */
    record_input_file_desc = "record_input_file:pathname, req"; /* And this. */
    output_file_desc = "output_file:pathname, req"; /* And this. */
    index_name_desc = "index_name:string,req"; /* Name of the index to add */
    pad_index_desc = "switch(-pad_index),=0"; /* But not this. Optional switch. */
    pad_records_desc = "switch(-pad_records),=0"; /* This too. */
    dae_file_desc = "switch(-dae_file),=0"; /* Switch for DAE file output */
    extent_size_desc = "option(-extent_size),number,word";
    disable_duplicates_desc = "switch(-disable_duplicates),=0"; /* Switch for blocking duplicate indexes */

    /* Get our param info. */
    s$parse_command(&caller_name, &error_code,
                    &key_input_file_desc, &key_input_file,
                    &record_input_file_desc, &record_input_file,
                    &output_file_desc, &output_file,
                    &index_name_desc, &index_name,
                    &pad_index_desc, &pad_index,
                    &pad_records_desc, &pad_records,
                    &dae_file_desc, &dae_file,
                    &disable_duplicates_desc, &disable_duplicates,
                    &extent_size_desc, &extent_size,
                    &end_descriptor);

    if (error_code != 0)
    {
        /* Something really, really weird happened. Output a nice message and exit. */
        s$error(&error_code, &caller_name, &error_message);
        return error_code; /* Goodbye, program. */
    }

    key_records_count = get_records_count(key_input_file, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
        return error_code;
    }

    record_records_count = get_records_count(record_input_file, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
        return error_code;
    }

    if (pad_records == 0)
    {
        if (key_records_count > record_records_count)
        {
            s$error(&e$too_many_indexes, &caller_name, &error_message);
            return e$too_many_indexes;
        }
    }

    if (pad_index == 0)
    {
        if (record_records_count > key_records_count)
        {
            s$error(&e$too_many_records, &caller_name, &error_message);
            return e$too_many_records;
        }
    }

    if (dae_file == 0)
    {
        output_organization = OUTPUT_SEQUENTIAL;
        output_record_size = 0; /* Ignored for sequential files */
    }
    else
    {
        output_organization = OUTPUT_ESEQ;
        if (extent_size != 0)
        {
            if ((extent_size > 7) && (isPowerOfTwo(extent_size))) output_record_size = extent_size;
            else
            {
                s$error(&e$invalid_extent_size, &caller_name, &error_message);
                return e$invalid_extent_size;
            }
        }
        else output_record_size = 8;
    }

    s$create_file(&output_file, &output_organization, &output_record_size, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
        return error_code;
    }

    create_separate_key_index(output_file, index_name, disable_duplicates, dae_file, extent_size, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
    }

    /* Get a port for key input */
    s$attach_port(&port_name, &key_input_file, &port_switches, &key_input_port, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
        return error_code;
    }

    /* And again, for record input */
    s$attach_port(&port_name, &record_input_file, &port_switches, &record_input_port, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
        return error_code;
    }

    /* Finally, for the output file. */
    s$attach_port(&port_name, &output_file, &port_switches, &output_port, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
        return error_code;
    }

    doConsolidate(key_input_port, record_input_port, output_port, index_name, pad_index, pad_records, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
    }
    error_code = 0; /* still need to clean up */

    s$detach_port(&key_input_port, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
    }
    error_code = 0;

    s$detach_port(&record_input_port, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
    }
    error_code = 0;

    s$detach_port(&output_port, &error_code);
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
    }

    printf("Execution complete.\n");
}

void create_separate_key_index(char_varying(256) output_file, char_varying(32) index_name, short int disable_duplicates, short int dae, short int extent_size, short int *output_error_code)
{
    create_index_options_tag ciot;

    short int index_switches = SEPARATE_KEY;
    short int number_components = 0; /* Not used for separate keys */
    short int positions[1] = {0}; /* Also not used for separate keys */
    short int lengths[1] = {0}; /* see above */
    short int collation = 4; /* we're just going with a reasonable default here - ASCII value based */
    char_varying(1280) duplicate_key; /* we won't be using this, since the index should be empty at creation time */
    short int error_code;

    if (disable_duplicates == 0) index_switches++; /* duplicate enable is in bit 1 of create_index2's switches param */
    ciot.version = 1; /* structure version - R068 specifies that this must be 1 */
    ciot.extent_options = 0;

    if (dae != 0)
    {
        printf("Using dynamic extents...\n");
        ciot.extent_options = 1;
        if (extent_size != 0) ciot.extent_size = extent_size;
        else ciot.extent_size = 8;
        ciot.extent_blocks = ciot.extent_size;
    }
    else
    {
        ciot.extent_size = 1;
        ciot.extent_blocks = 0; /* not used */
    }

    ciot.max_key_len = 1280;
    s$get_current_dir(&ciot.work_dir);

    s$create_index2(&output_file, &number_components, &positions, &lengths, &index_name, &index_switches,
                    &collation, &duplicate_key, &ciot, &error_code);

    *output_error_code = error_code;
}

void doConsolidate(short int key_input_port, short int record_input_port, short int output_port, char_varying(32) index_name, short int pad_index, short int pad_records, short int *output_error_code)
{
    short int file_organization = 0; /* File already exists - this doesn't matter */
    short int input_io_type = IO_INPUT; /* input */
    short int locking_mode = SETLOCKDONTWAIT;
    short int input_access_mode = AM_SEQUENTIAL;
    short int maximum_length = 16; /* should be ignored, setting a sane value just in case */
    short int error_code;
    short int buffer_length = 1280;
    short int record_length;

    char_varying(1280) record_key;
    short int output_io_type = IO_OUTPUT;
    short int output_access_mode = AM_INDEXED;

    char *key_input_buffer = malloc(1280);
    char *record_input_buffer = malloc(1280);
    char *output_buffer = malloc (1280);

    int pad_count = 0; /* we use this if we need to pad the keys or records */
    short int key_eof; /* if the key - which is read first - goes EOF, we track it here */

    /* open key input, then record input, then output */
    /* note that there's no real reason to pass index_name in the input opens - but
     * the function expects one, so we might as well, rather than passing a blank string */
    s$open(&key_input_port, &file_organization, &maximum_length, &input_io_type, &locking_mode,
           &input_access_mode, &index_name, &error_code);
    if (error_code != 0)
    {
        *output_error_code = error_code;
        free(key_input_buffer);
        free(record_input_buffer);
        free(output_buffer);
        return; /* let s$error deal with it on the other side */
    }

    s$open(&record_input_port, &file_organization, &maximum_length, &input_io_type, &locking_mode,
           &input_access_mode, &index_name, &error_code);
    if (error_code != 0)
    {
        *output_error_code = error_code;
        s$close(&key_input_port, &error_code);
        free(key_input_buffer);
        free(record_input_buffer);
        free(output_buffer);
        return;
    }

    /* finally, open our newly-created indexed file for output */
    s$open(&output_port, &file_organization, &maximum_length, &output_io_type, &locking_mode,
           &output_access_mode, &index_name, &error_code);
    if (error_code != 0)
    {
        *output_error_code = error_code;
        s$close(&key_input_port, &error_code);
        s$close(&record_input_port, &error_code);
        free(key_input_buffer);
        free(record_input_buffer);
        free(output_buffer);
        return;
    }

    /* ... and make the maximum length go beyond 64 */
    s$set_key_access(&output_port, &buffer_length, &error_code);
    if (error_code != 0)
    {
        *output_error_code = error_code;
        s$close(&key_input_port, &error_code);
        s$close(&record_input_port, &error_code);
        s$close(&output_port, &error_code);
        free(key_input_buffer);
        free(record_input_buffer);
        free(output_buffer);
        return;
    }

    /* Now all our files are open. What happens next is that we iterate over the inputs,
     * and do keyed writes to the output, with one buffer as the rec and one as the key. */
    while (1)
    {
        s$seq_read(&key_input_port, &buffer_length, &record_length, key_input_buffer, &error_code);
        if (error_code != 0)
        {
            if (error_code == 1025) /* EOF */
            {
                key_eof = 1;
                if (pad_index != 0)
                {
                    format_padding(pad_count, key_input_buffer, buffer_length); /* start putting numbers in, since we ran out of keys */
                    pad_count++;
                }
                else break; /* if we're at EOF, and padding isn't enabled, that means we're done */
            }
            else
            {
                break;
            }
        }
        s$seq_read(&record_input_port, &buffer_length, &record_length, record_input_buffer, &error_code);
        if (error_code != 0)
        {
            if (error_code == 1025) /* EOF */
            {
                if (key_eof) break; /* ran out of things to write. very sad. */
                if (pad_records != 0)
                {
                    format_padding(pad_count, record_input_buffer, buffer_length);
                    pad_count++;
                }
                else break; /* EOF, no padding, all done - probably shouldn't normally happen */
                error_code = 0;
            }
            else
            {
                break;
            }
        }

        strcpy_vstr_nstr(&record_key, key_input_buffer);
        buffer_length = strlen(record_input_buffer);
        s$keyed_write(&output_port, &record_key, &buffer_length, record_input_buffer, &error_code);
        if (error_code != 0)
        {
            break;
        }
    }

    if (error_code == 1025) error_code = 0; /* 1025 just means we finished the file, usually */

    /* this is NOT a good way of handling errors! */
    /* Unfortunately, without try/finally in C, we don't have much alternative */
    /* Output the error code, then turn off the alarm and attempt to close the file. */
    /* We're done here. */
    if (error_code != 0)
    {
        s$error(&error_code, &caller_name, &error_message);
        error_code = 0;
    }

    s$close(&key_input_port, &error_code);
    s$close(&record_input_port, &error_code);
    s$close(&output_port, &error_code);
    free(key_input_buffer);
    free(record_input_buffer);
    free(output_buffer);
    *output_error_code = error_code;
}

/* best pretend you didn't see this. at least it's fast. */
short int isPowerOfTwo(short int numToTest)
{
    return (numToTest == 8 || numToTest == 16 || numToTest == 32 || numToTest == 64 ||
            numToTest == 128 || numToTest == 256);
}

void format_padding(short int input_number, char *buffer, int buffer_size)
{
    snprintf(buffer, buffer_size, "padding_%d", input_number);
}

/* R068 doesn't seem to show a syscall for this, so let's use s$get_file_status */
/* s$get_file_status also returns a bunch of fun stuff we don't use! It is a large function. */
long get_records_count(char_varying(256) input_file, short int *output_error_code)
{
    FILE_STATUS_STRUCT gfit;
    short int error_code = 0;
    long records_count = 0;

    gfit.version = 9; /* we don't use the additional fields in v11 */
    s$get_file_status(&input_file, &gfit, &error_code);
    if (gfit.file_organization > 3)
    {
        error_code = e$stream_file; /* Not just triggered by stream files, but roll with it */
    }
    records_count = gfit.record_count;
    *output_error_code = error_code;

    return records_count;
}

