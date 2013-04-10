/**
 *  @file       main.cpp
 *  @author     Michael A. Uman
 *  @date       April 8th, 2013
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <string>
#include <sys/stat.h>
#include <errno.h>
#include <byteswap.h>
#include <popt.h>

int nSwapLong = 0;
int nSwapWord = 0;
int nDisplayOff = 1;
int nAscii = 1;
int nCase = 0;

const char* szStartOffset = 0L;
const char* szByteCount = 0L;
const char* szInputFilename = 0L;
const char* szDisplayStart = 0L;
const char* szTitle = 0L;
const char* szOutfile = 0L;
//const char* szCase = 0L;



enum formenum {
    FORM_FOUR_BYTES = 0,
    FORM_THREE_BYTES = 1,
    FORM_TWO_BYTES = 2,
    FORM_ONE_BYTE = 3,
    FORM_OFFSET = 4,
};

const char* formats[][2] = {
        { "%02X %02X %02X %02X", "%02x %02x %02x %02x", },
        { "%02X %02X %02X",      "%02x %02x %02x", },
        { "%02X %02X",           "%02x %02x", },
        { "%02X",                "%02x", },
        { "%08X: ",              "%08x: ", },
};


/**
 *  Parse command line options.
 */

int parse_commandline(int argc, const char* argv[]) {
    int rc;
    static struct poptOption options[] = {
        { "swaplong",   NULL, POPT_ARG_NONE, &nSwapLong, 0,       "Swap longs", },
        { "swapword",   NULL, POPT_ARG_NONE, &nSwapWord, 0,       "Swap words", },
        { "startoff",   NULL, POPT_ARG_STRING, &szStartOffset, 0, "Start Offset", },
        { "count",      NULL, POPT_ARG_STRING, &szByteCount, 0,   "Byte Count", },
        { "displayoff", NULL, POPT_ARG_INT, &nDisplayOff, 0,      "Display offset", "on/off", },
        { "displaystart", NULL, POPT_ARG_STRING, &szDisplayStart, 0, "Display Start", "Offset", },
        { "title",      NULL, POPT_ARG_STRING, &szTitle, 0,       "Dump title", "Title Text", },
        { "ascii",      NULL, POPT_ARG_INT, &nAscii, 0,           "ASCII Dump", "1/0", },
        { "case",       NULL, POPT_ARG_INT, &nCase, 0,         "Hex Case", "0=upper,1=lower", },
        { NULL,         'o',  POPT_ARG_STRING, &szOutfile, 0,     "Output file", "filename", },

        POPT_AUTOHELP
        POPT_TABLEEND
    };
    poptContext poptcon;

    poptcon = poptGetContext("muhd", argc, argv, options, 0L);
    poptSetOtherOptionHelp( poptcon, "[OPTIONS]* <input filename>");

    while ((rc = poptGetNextOpt( poptcon )) > 0) {
        switch (rc) {
        default:
            break;
        }
    }

    szInputFilename = poptGetArg( poptcon );

    poptFreeContext( poptcon );

    if ((nSwapLong==1) && (nSwapWord == 1)) {
        fprintf(stderr, "ERROR: Must specify only one swap option...\n");
        return -1;
    }

    /* Make sure input file was specified and exists */
    if (szInputFilename == 0) {
        fprintf(stderr, "ERROR: Must specify input filename!\n");
        return -1;
    } else {
        struct stat statbuf;

        if (stat( szInputFilename, &statbuf) != 0) {
            fprintf(stderr, "ERROR: Input file doesn't exist!\n");
            return -1;
        }
    }

    return 0;
}

/**
 *
 */

int main(int argc, const char* argv[]) {
    FILE*   oFP = stdout;
    FILE*   iFP = 0L;
    size_t  line_byte_count = 0L;
    char    asciiBuf[17];
    size_t  total_bytes_read = 0L;
    size_t  display_offset = 0L;

    /* Parse all the commandline options into global variables */
    if (parse_commandline( argc, argv ) != 0) {
        fprintf(stderr, "ERROR: Unable to parse options!\n");
        return -1;
    }

#ifdef  _DEBUG
    fprintf(stderr, "Input file = %s\n", szInputFilename);
#endif

    /* Prepare the ASCII buffer */
    memset(asciiBuf, ' ', 16);
    asciiBuf[16] = 0;

    /* Open the input file */
    if ((iFP = fopen(szInputFilename, "r")) == 0) {
        fprintf(stderr, "ERROR: Unable to open file [%s]...\n", strerror(errno));
        return -1;
    }

    /* If user specified an output file, open it for writing... */
    if (szOutfile != 0L) {
        oFP = fopen( szOutfile, "w");
        if (oFP == 0L) {
            fprintf(stderr, "ERROR: Unable to open output file '%s'.\n", szOutfile);
            return -1;
        }
    }

    /* If user specified a start offset, retreive it from the string */
    if (szStartOffset != 0) {
        unsigned int    nTmp = 0;
        size_t          startOffset= 0;

        if ((strlen(szStartOffset) > 3) && (szStartOffset[0] == '0') && (szStartOffset[1] == 'x')) {
            sscanf(szStartOffset, "0x%08x", &nTmp);
            startOffset = nTmp;
        } else {
            startOffset = atoi(szStartOffset);
        }

        fseek(iFP, startOffset, SEEK_SET);
        display_offset = startOffset;
    }

    if (szDisplayStart != 0) {
        unsigned int    nTmp = 0;

        if ((strlen(szDisplayStart) > 3) && (szDisplayStart[0] == '0') && (szDisplayStart[1] == 'x')) {
            sscanf(szDisplayStart, formats[FORM_OFFSET][nCase], &nTmp);
            display_offset = nTmp;
        } else {
            display_offset = atoi(szDisplayStart);
        }
    }

    /* If the user specified a title string, display it... */
    if (szTitle != 0L) {
        fprintf(oFP, "== %s ==\n", szTitle);
    }

    if (nDisplayOff != 0) {
        fprintf(oFP,"%08lX: ", display_offset);
    }

    while (!feof( iFP )) {
        uint32_t        buffer = 0;
        size_t          br; // bytes read
        unsigned char*  ptr = (unsigned char *)&buffer;

        /* Read 4 bytes at a time */
        br = fread( &buffer, 1, sizeof(buffer), iFP);

        if (nSwapLong) {
            switch (br) {
            case 4:
                //fprintf(oFP, "%02X %02X %02X %02X", ptr[3], ptr[2], ptr[1], ptr[0]);
                fprintf(oFP, formats[FORM_FOUR_BYTES][nCase], ptr[3], ptr[2], ptr[1], ptr[0]);
                asciiBuf[line_byte_count]   = isprint(ptr[3])?ptr[3]:'.';
                asciiBuf[line_byte_count+1] = isprint(ptr[2])?ptr[2]:'.';
                asciiBuf[line_byte_count+2] = isprint(ptr[1])?ptr[1]:'.';
                asciiBuf[line_byte_count+3] = isprint(ptr[0])?ptr[0]:'.';
                break;
            case 3:
                fprintf(oFP, formats[FORM_THREE_BYTES][nCase], ptr[0], ptr[1], ptr[2]);
                asciiBuf[line_byte_count]   = isprint(ptr[2])?ptr[2]:'.';
                asciiBuf[line_byte_count+1] = isprint(ptr[1])?ptr[1]:'.';
                asciiBuf[line_byte_count+2] = isprint(ptr[0])?ptr[0]:'.';
                break;
            case 2:
                fprintf(oFP, formats[FORM_TWO_BYTES][nCase], ptr[1], ptr[0]);
                asciiBuf[line_byte_count]   = isprint(ptr[1])?ptr[1]:'.';
                asciiBuf[line_byte_count+1] = isprint(ptr[0])?ptr[0]:'.';
                break;
            case 1:
                fprintf(oFP, formats[FORM_ONE_BYTE][nCase], ptr[0]);
                asciiBuf[line_byte_count]   = isprint(ptr[0])?ptr[0]:'.';
                break;
            default:
                break;
            }

        } else if (nSwapWord) {
            /* SWAP WORDS */
        } else {

            switch (br) {
            case 4:
                fprintf(oFP, formats[FORM_FOUR_BYTES][nCase], ptr[0], ptr[1], ptr[2], ptr[3]);
                asciiBuf[line_byte_count]   = isprint(ptr[0])?ptr[0]:'.';
                asciiBuf[line_byte_count+1] = isprint(ptr[1])?ptr[1]:'.';
                asciiBuf[line_byte_count+2] = isprint(ptr[2])?ptr[2]:'.';
                asciiBuf[line_byte_count+3] = isprint(ptr[3])?ptr[3]:'.';
                break;
            case 3:
                fprintf(oFP, formats[FORM_THREE_BYTES][nCase], ptr[0], ptr[1], ptr[2]);
                asciiBuf[line_byte_count]   = isprint(ptr[0])?ptr[0]:'.';
                asciiBuf[line_byte_count+1] = isprint(ptr[1])?ptr[1]:'.';
                asciiBuf[line_byte_count+2] = isprint(ptr[2])?ptr[2]:'.';
                break;
            case 2:
                fprintf(oFP, formats[FORM_TWO_BYTES][nCase], ptr[0], ptr[1]);
                asciiBuf[line_byte_count]   = isprint(ptr[0])?ptr[0]:'.';
                asciiBuf[line_byte_count+1] = isprint(ptr[1])?ptr[1]:'.';
                break;
            case 1:
                fprintf(oFP, formats[FORM_ONE_BYTE][nCase], ptr[0]);
                asciiBuf[line_byte_count]   = isprint(ptr[0])?ptr[0]:'.';
                break;
            }

        }

        /* Update byte counters */
        line_byte_count  += br;
        total_bytes_read += br;
        display_offset   += br;

        if (line_byte_count == 16) {
            if (nAscii != 0)
                fprintf(oFP, " : %16s\n", asciiBuf);
            else
                fprintf(oFP, "\n");

            if (nDisplayOff != 0) {
                char ch = fgetc(iFP);

                /* If not the end of file, display next offset header */
                if (!feof(iFP)) {
                    fprintf(oFP, formats[FORM_OFFSET][nCase], display_offset);
                    ungetc(ch, iFP);
                }
            }

            line_byte_count = 0;
            memset(asciiBuf, ' ', 16);
        } else {
            if (!feof(iFP)) {
                fprintf(oFP, " ");
            }
        }

//        printf("bytes read = %ld %08x\n", br, bswap_32(buffer));
    }

   // printf("\nline_byte_count %d\n", line_byte_co

    if (line_byte_count != 0) {
         if ((line_byte_count % 4) != 0)
            fprintf(oFP, " ");

        for (size_t x = 0 ; x < (16-line_byte_count) ; x++) {
            fprintf(oFP, "  ");
            if (x < 16)
                fprintf(oFP, " ");
        }
        if (nAscii != 0)
            fprintf(oFP, ": %16s\n", asciiBuf);
        else
            fprintf(oFP, "\n");

    }

    /* Close the output file. */
    if (szOutfile != 0L) {
        fclose(oFP);
    }

    fclose(iFP);

    return 0;
}
//abcd
