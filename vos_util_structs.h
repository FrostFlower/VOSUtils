/* This struct is described in the 17.1 and 18.0 versions of Stratus R068, available on stratadoc.stratus.com */

typedef struct $longmap
{
    short int                   version;
    short int 					 extent_options;
    long int                    extent_size;
    long int                    extent_blocks;
    short int                   max_key_len;
    char_varying (256)          work_dir;
}  create_index_options_tag;
