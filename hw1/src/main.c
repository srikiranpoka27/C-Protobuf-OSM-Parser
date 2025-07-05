#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "osm.h"
#include "debug.h"

int main(int argc, char **argv)
{
    if (process_args(argc, argv, NULL) != 0) {
        USAGE(*argv, EXIT_FAILURE);
    }

    if (help_requested) {
        USAGE(*argv, EXIT_SUCCESS);
    }

    if (osm_input_file) {
        FILE *in = fopen(osm_input_file, "rb");

        if (in == NULL) {
            fprintf(stderr, "Cannot read the input file %s\n", osm_input_file);
            USAGE(*argv, EXIT_FAILURE);
        }


        OSM_Map *map = OSM_read_Map(in);

        if (map == NULL) {
            fprintf(stderr, "Cannot read the map!\n");
        }

        if (process_args(argc, argv, map) != 0) {
            USAGE(*argv, EXIT_FAILURE);
        }

        fclose(in);
    }
    USAGE(*argv, EXIT_SUCCESS);
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
