static const short int OUTPUT_FIXED = 1; /* unused for now */
static const short int OUTPUT_RELATIVE = 2; /* unused for now */
static const short int OUTPUT_SEQUENTIAL = 3; /* Regular sequential file (2GB limit) */
static const short int OUTPUT_ESEQ = 65; /* Extended sequential file (always DAE in CRF) */

static const short int SEPARATE_KEY = 2; /* We'll be using a non-embedded separate key. */

static const short int SETLOCKDONTWAIT = 1;

static const short int IO_INPUT = 1;
static const short int IO_OUTPUT = 2;
static const short int IO_APPEND = 3;
static const short int IO_UPDATE = 4;

static const short int AM_SEQUENTIAL = 1;
static const short int AM_RANDOM = 2;
static const short int AM_INDEXED = 3;
