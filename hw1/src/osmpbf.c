#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "global.h"
#include "protobuf.h"
#include "osm.h"
#include "debug.h"

typedef struct OSM_BBox {
    OSM_Lat min_lat;
    OSM_Lat max_lat;
    OSM_Lon min_lon;
    OSM_Lon max_lon;
} OSM_BBox;

typedef struct OSM_Map {
    OSM_BBox bbox;
    OSM_Node *nodes;
    int num_nodes;
    OSM_Way *ways;
    int num_ways;
} OSM_Map;

typedef struct OSM_Node {
    OSM_Id id;
    OSM_Lat lat;
    OSM_Lon lon;
} OSM_Node;

typedef struct OSM_Way {
    OSM_Id id;
    int num_refs;
} OSM_Way;

static int64_t zigzag_decode(int64_t n) { // helper function for zig zag decoding
    if (n%2 == 0) return (n/2);

    return -(n+1)/2;
}

/**
 * @brief Read map data in OSM PBF format from the specified input stream,
 * construct and return a corresponding OSM_Map object.  Storage required
 * for the map object and any related entities is allocated on the heap.
 * @param in  The input stream to read.
 * @return  If reading was successful, a pointer to the OSM_Map object constructed
 * from the input, otherwise NULL in case of any error.
 */

OSM_Map *OSM_read_Map(FILE *in) {
    OSM_Map *map = (OSM_Map *)malloc(sizeof(OSM_Map));
    if (!map) {
        return NULL;
    }

    map->num_nodes = 0;
    map->nodes = NULL;
    map->num_ways = 0;
    map->ways = NULL;

    unsigned char buffer[4];
    size_t len;

    while (fread(buffer, 1, 4, in) == 4) {
        len = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
        PB_Message msg;

        if (PB_read_message(in, len, &msg) != 1) {
            return NULL;
        }

        PB_Field *blob_type = PB_get_field(msg, 1, LEN_TYPE);

        if (blob_type && strcmp(blob_type->value.bytes.buf, "OSMHeader") == 0) {

            PB_Field *bbox_min_lon = PB_get_field(msg, 1, VARINT_TYPE);
            PB_Field *bbox_max_lon = PB_get_field(msg, 2, VARINT_TYPE);
            PB_Field *bbox_min_lat = PB_get_field(msg, 3, VARINT_TYPE);
            PB_Field *bbox_max_lat = PB_get_field(msg, 4, VARINT_TYPE);

            if (bbox_min_lon) map->bbox.min_lon = zigzag_decode(bbox_min_lon->value.i64) / 1e9;
            if (bbox_max_lon) map->bbox.max_lon = zigzag_decode(bbox_max_lon->value.i64) / 1e9;
            if (bbox_min_lat) map->bbox.min_lat = zigzag_decode(bbox_min_lat->value.i64) / 1e9;
            if (bbox_max_lat) map->bbox.max_lat = zigzag_decode(bbox_max_lat->value.i64) / 1e9;
        }
    }
    return map;
}

/**
 * @brief  Get the number of nodes in an OSM_Map object.
 *
 * @param  mp  The map object to query.
 * @return  The number of nodes.
 */

int OSM_Map_get_num_nodes(OSM_Map *mp) {
    if (mp == NULL) return -1;
    return mp->num_nodes;
}

/**
 * @brief  Get the number of ways in an OSM_Map object.
 *
 * @param  mp  The map object to query.
 * @return  The number of ways.
 */

int OSM_Map_get_num_ways(OSM_Map *mp) {
    if (mp == NULL) return -1;
    return mp->num_ways;
}

/**
 * @brief  Get the node at the specified index from an OSM_Map object.
 *
 * @param  mp  The map to be queried.
 * @param  index  The index of the node to be retrieved.
 * @param  return  The node at the specifed index, if the index was in
 * the valid range [0, num_nodes), otherwise NULL.
 */

OSM_Node *OSM_Map_get_Node(OSM_Map *mp, int index) {
    // To be implemented
    abort();
}

/**
 * @brief  Get the way at the specified index from an OSM_Map object.
 *
 * @param  mp  The map to be queried.
 * @param  index  The index of the way to be retrieved.
 * @param  return  The way at the specifed index, if the index was in
 * the valid range [0, num_ways), otherwise NULL.
 */

OSM_Way *OSM_Map_get_Way(OSM_Map *mp, int index) {
    // To be implemented
    abort();
}

/**
 * @brief  Get the bounding box, if any, of the specified OSM_Map object.
 *
 * @param  mp  The map object to be queried.
 * @return  The bounding box of the map object, if it has one, otherwise NULL.
 */

OSM_BBox *OSM_Map_get_BBox(OSM_Map *mp) {
    if (mp == NULL) return NULL;
    return &mp->bbox;
}

/**
 * @brief  Get the id of an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @return  The id of the node.
 */

int64_t OSM_Node_get_id(OSM_Node *np) {
    if (np == NULL) return -1;
    return np->id;
}

/**
 * @brief  Get the latitude of an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @return  The latitude of the node, in nanodegrees.
 */

int64_t OSM_Node_get_lat(OSM_Node *np) {
    if (np == NULL) return -1;
    return np->lat;
}

/**
 * @brief  Get the longitude of an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @return  The latitude of the node, in nanodegrees.
 */

int64_t OSM_Node_get_lon(OSM_Node *np) {
    if (np == NULL) return -1;
    return np->lon;
}

/**
 * @brief  Get the number of keys in an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @return  The number of keys (or key/value pairs) in the node.
 */

int OSM_Node_get_num_keys(OSM_Node *np) {
    // To be implemented
    abort();
}

/**
 * @brief  Get the key at a specified index in an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @param index  The index of the key.
 * @return  The key at the specified index, if the index is in the valid range
 * [0, num_keys), otherwise NULL.  The key is returned as a pointer to a
 * null-terminated string.
 */

char *OSM_Node_get_key(OSM_Node *np, int index) {
    // To be implemented
    abort();
}

/**
 * @brief  Get the value at a specified index in an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @param index  The index of the value.
 * @return  The value at the specified index, if the index is in the valid range
 * [0, num_keys), otherwise NULL.  The value is returned as a pointer to a
 * null-terminated string.
 */

char *OSM_Node_get_value(OSM_Node *np, int index) {
    // To be implemented
    abort();
}

/**
 * @brief  Get the id of an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @return  The id of the way.
 */

int64_t OSM_Way_get_id(OSM_Way *wp) {
    if (wp == NULL) return -1;
    return wp->id;
}

/**
 * @brief  Get the number of node references in an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @return  The number of node references contained in the way.
 */

int OSM_Way_get_num_refs(OSM_Way *wp) {
    if (wp == NULL) return -1;
    return wp->num_refs;
}

/**
 * @brief  Get the node reference at a specified index in an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @param index  The index of the node reference.
 * @return  The id of the node referred to at the specified index,
 * if the index is in the valid range [0, num_refs), otherwise NULL.
 */

OSM_Id OSM_Way_get_ref(OSM_Way *wp, int index) {
    // To be implemented
    abort();
}

/**
 * @brief  Get the number of keys in an OSM_Way object.
 *
 * @param np  The node object to be queried.
 * @return  The number of keys (or key/value pairs) in the way.
 */

int OSM_Way_get_num_keys(OSM_Way *wp) {
    // To be implemented
    abort();
}

/**
 * @brief  Get the key at a specified index in an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @param index  The index of the key.
 * @return  The key at the specified index, if the index is in the valid range
 * [0, num_keys), otherwise NULL.  The key is returned as a pointer to a
 * null-terminated string.
 */

char *OSM_Way_get_key(OSM_Way *wp, int index) {
    // To be implemented
    abort();
}

/**
 * @brief  Get the value at a specified index in an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @param index  The index of the value.
 * @return  The value at the specified index, if the index is in the valid range
 * [0, num_keys), otherwise NULL.  The value is returned as a pointer to a
 * null-terminated string.
 */

char *OSM_Way_get_value(OSM_Way *wp, int index) {
    // To be implemented
    abort();
}

/**
 * @brief  Get the minimum longitude coordinate of an OSM_BBox object.
 *
 * @param bbp the bounding box to be queried.
 * @return  the minimum longitude coordinate of the bounding box, in nanodegrees.
 */

int64_t OSM_BBox_get_min_lon(OSM_BBox *bbp) {
    if (bbp == NULL) return -1;
    return bbp->min_lon;
}

/**
 * @brief  Get the maximum longitude coordinate of an OSM_BBox object.
 *
 * @param bbp the bounding box to be queried.
 * @return  the maximum longitude coordinate of the bounding box, in nanodegrees.
 */

int64_t OSM_BBox_get_max_lon(OSM_BBox *bbp) {
    if (bbp == NULL) return -1;
    return bbp->max_lon;
}

/**
 * @brief  Get the maximum latitude coordinate of an OSM_BBox object.
 *
 * @param bbp the bounding box to be queried.
 * @return  the maximum latitude coordinate of the bounding box, in nanodegrees.
 */

int64_t OSM_BBox_get_max_lat(OSM_BBox *bbp) {
    if (bbp == NULL) return -1;
    return bbp->max_lat;
}

/**
 * @brief  Get the minimum latitude coordinate of an OSM_BBox object.
 *
 * @param bbp the bounding box to be queried.
 * @return  the minimum latitude coordinate of the bounding box, in nanodegrees.
 */

int64_t OSM_BBox_get_min_lat(OSM_BBox *bbp) {
    if (bbp == NULL) return -1;
    return bbp->min_lat;
}
