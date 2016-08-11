consolidate_record_files - or crf for short - is a simple VOS utility that
takes a pair of structured files (currently, only sequential files are
supported) and generates an indexed sequential file from them, using one
file as the source of keys and the other as a source of records.

No guarantees of usefulness, expressed or implied.

Released into the public domain.

Very much a work in progress. Primarily using this as a debug tool for myself.

Installation:

Run build.cm. Alternatively, use vcc, create_table, and make_message_file manually.

Usage:

Mandatory arguments (positional):

key_input_file: Filename of file to draw keys from

record_input_file: Filename of file to draw records from

output_file: Filename to output to. Must not exist.

index_name: Name of index to create.

Optional arguments:

-pad_index: If there are fewer records in index source file than record source file, 
	automatically generate padding keys
	
-pad_records: If there are fewer records in record source file than index source file,
	automatically generate padding records
	
-dae_file: Automatically create an extended sequential file with dynamic extents

-extent_size [blocks]: Define extent size manually for DAE files. Must be a power of 2.
	Currently untested.

-disable_duplicates: Disable duplicate keys in generated file.