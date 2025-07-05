#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "global.h"
#include "osm.h"
#include "debug.h"

/* Variable to be set by process_args if the '-h' flag is seen. */
int help_requested = 0;

/* Variable to be set by process_args to any filename specified with '-f'. */
char *osm_input_file = NULL;

/**
 * @brief  Validate command-line arguments with possible simultaneous execution
 * of queries against a map.
 * @details  This function traverses the command-line arguments specified by
 * argc and argv and verifies that they represent a valid invocation of the
 * program.  In addition, this function determines whether '-h' has been given
 * as the first argument and, if so, sets the global variable help_requested
 * to a nonzero value.  It also checks whether there is an occurrence of
 * '-f filename' and, if so, sets the global variable osm_input_file to the
 * specified filename.
 * @param argc  Argument count, as passed to main.
 * @param argv  Argument vector, as passed to main.
 * @param mp  If non-NULL, this is a pointer to a map to be used for processing
 * the queries specified by the option arguments.  If NULL, then only argument
 * validation is performed and no query processing is done.
 * @return 0  if the arguments are valid and, if mp was non-NULL, then there were
 * no errors in processing they specified.  If the arguments are invalid, or
 * if there were errors in processing the queries, then -1 is returned.
 */

int process_args(int argc, char **argv, OSM_Map *mp) {
    if (argc<2) {
        USAGE(*argv, EXIT_FAILURE);
        return 0;
    }

    int file_available = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            help_requested = 1;
            return 0;
        } else if (strcmp(argv[i], "-f") == 0) {
            if (file_available) {
                fprintf(stderr, "-f can only be used once\n");
                return -1;
            }
            if (i+1 >= argc || argv[i+1][0] == '-') {
                fprintf(stderr, "-f should be followed by a file name");
                return -1;
            }
            osm_input_file = argv[i+1];
            file_available = 1;
            i++;
        } else if (strcmp(argv[i], "-n") == 0) {
            if (osm_input_file == NULL) {
                fprintf(stderr, "No input file given \n");
                return -1;
            }

            if (i+1 >= argc || argv[i+1][0] == '-') {
                fprintf(stderr, "-n should be followed by the node id\n");
                return -1;
            }

        } else if (strcmp(argv[i], "-w") == 0) {
            if (osm_input_file == NULL) {
                fprintf(stderr, "No input file given \n");
                return -1;
            }

            if (i+1 >= argc || argv[i+1][0] == '-') {
                fprintf(stderr, "-w should be followed by the way id\n");
                return -1;
            }

        } else if (strcmp(argv[i], "-s") == 0) {
            if (osm_input_file == NULL) {
                fprintf(stderr, "No input file given \n");
                return -1;
            }

            if (i+1 < argc && argv[i+1][0] != '-') {
                fprintf(stderr, "-s can only be followed by other query arguments\n");
                return -1;
            }

            printf("nodes: %d, ways: %d\n", OSM_Map_get_num_nodes(mp), OSM_Map_get_num_ways(mp));
        } else if (strcmp(argv[i], "-b") == 0) {
            if (osm_input_file == NULL) {
                fprintf(stderr, "No input file given \n");
                return -1;
            }

            if (i+1 < argc && argv[i+1][0] != '-') {
                fprintf(stderr, "-b can only be followed by other query arguments\n");
                return -1;
            }

            OSM_BBox *bbox = (OSM_BBox *)malloc(sizeof(OSM_BBox *));
            bbox = OSM_Map_get_BBox(mp);


            int64_t max_lon = OSM_BBox_get_max_lon(bbox);
            int64_t min_lon = OSM_BBox_get_min_lon(bbox);
            int64_t max_lat = OSM_BBox_get_max_lat(bbox);
            int64_t min_lat = OSM_BBox_get_min_lat(bbox);

            printf ("max lon: %" PRId64 ", ", max_lon);
            printf ("min lon: %" PRId64 ", ", min_lon);
            printf ("max lat: %" PRId64 ", ", max_lat);
            printf ("min lat: %" PRId64 "\n", min_lat);
        }
    }
    return 0;
}
