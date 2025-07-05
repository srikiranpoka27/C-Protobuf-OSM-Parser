#include <stdio.h>
#include <stdlib.h>

#include "protobuf.h"
#include "zlib_inflate.h"
#include "debug.h"

/**
 * @brief  Read data from an input stream, interpreting it as a protocol buffer
 * message.
 * @details  This function assumes that the input stream "in" contains at least
 * len bytes of data.  The data is read from the stream, interpreted as a
 * protocol buffer message, and a pointer to the resulting PB_Message object is
 * returned.
 *
 * @param in  The input stream from which to read data.
 * @param len  The number of bytes of data to read from the input stream.
 * @param msgp  Pointer to a caller-provided variable to which to assign the
 * resulting PB_Message.
 * @return 0 in case of an immediate end-of-file on the input stream without
 * any error and no input bytes having been read, -1 if there was an error
 * or unexpected end-of-file after reading a non-zero number of bytes,
 * otherwise the number n > 0 of bytes read if no error occurred.
 */

int PB_read_varint(FILE *in, uint64_t *value) {
    uint64_t result = 0;
    int shift = 0;
    size_t bytes_read = 0;
    uint8_t byte;

    do {
        if (fread(&byte, 1, 1, in) != 1) {
            return -1;
        }
        result |= ((uint64_t)(byte & 0x7F)) << shift;
        shift += 7;
        bytes_read++;
    } while (byte & 0x80);

    *value = result;
    return bytes_read;
}

int PB_read_message(FILE *in, size_t len, PB_Message *msgp) {
    if (feof(in)) {
        return 0;
    }

    PB_Field *head = malloc(sizeof(PB_Field));
    if (!head) return -1;

    head->type = SENTINEL_TYPE;
    head->next = head;
    head->prev = head;  // default empty PB_Message has only one node which is of SENTINEL_TYPE

    size_t bytes_read = 0;

    while (bytes_read < len) {
        PB_Field *curr_field = malloc(sizeof(PB_Field));
        if (!curr_field) return -1;


        int bytes = PB_read_field(in, curr_field);

        if (bytes < 1) return -1;

        curr_field->next = head;
        curr_field->prev = head->prev;
        head->prev->next = curr_field;
        head->prev = curr_field;

        bytes_read += bytes;
    }

    *msgp = head;

    return 1;
}

/**
 * @brief  Read data from a memory buffer, interpreting it as a protocol buffer
 * message.
 * @details  This function assumes that buf points to a memory area containing
 * len bytes of data.  The data is interpreted as a protocol buffer message and
 * a pointer to the resulting PB_Message object is returned.
 *
 * @param buf  The memory buffer containing the compressed data.
 * @param len  The length of the compressed data.
 * @param msgp  Pointer to a caller-provided variable to which to assign the
 * resulting PB_Message.
 * @return 0 in case of success, -1 in case any error occurred.
 */

int PB_read_embedded_message(char *buf, size_t len, PB_Message *msgp) {
    if (buf == NULL) return -1;
    if (len == 0) return 0;

    FILE *in = fmemopen(buf, len, "rb");
    if (!in) return -1;

    int result = PB_read_message(in, len, msgp);

    fclose(in);

    return result;
}

/**
 * @brief  Read zlib-compressed data from a memory buffer, inflating it
 * and interpreting it as a protocol buffer message.
 * @details  This function assumes that buf points to a memory area containing
 * len bytes of zlib-compressed data.  The data is inflated, then the
 * result is interpreted as a protocol buffer message and a pointer to
 * the resulting PB_Message object is returned.
 *
 * @param buf  The memory buffer containing the compressed data.
 * @param len  The length of the compressed data.
 * @param msgp  Pointer to a caller-provided variable to which to assign the
 * resulting PB_Message.
 * @return 0 in case of success, -1 in case any error occurred.
 */

int PB_inflate_embedded_message(char *buf, size_t len, PB_Message *msgp) {
    if (buf == NULL) return -1;
    if (len == 0) return 0;

    FILE *input = fmemopen(buf, len, "r");
    if (!input) return -1;

    char *outbuf = NULL;
    size_t outlen = 0;

    FILE *output = open_memstream(&outbuf, &outlen);
    if (!output) {
        fclose(input);
        return -1;
    }

    zlib_inflate(input, output);
    fclose(input);
    fflush(output);
    fclose(output);

    int result = PB_read_embedded_message(outbuf, outlen, msgp);
    return result;
}

/**
 * @brief  Read a single field of a protocol buffers message and initialize
 * a PB_Field structure.
 * @details  This function reads data from the input stream in and interprets
 * it as a single field of a protocol buffers message.  The information read,
 * consisting of a tag that specifies a wire type and field number,
 * as well as content that depends on the wire type, is used to initialize
 * the caller-supplied PB_Field structure pointed at by the parameter fieldp.
 * @param in  The input stream from which data is to be read.
 * @param fieldp  Pointer to a caller-supplied PB_Field structure that is to
 * be initialized.
 * @return 0 in case of an immediate end-of-file on the input stream without
 * any error and no input bytes having been read, -1 if there was an error
 * or unexpected end-of-file after reading a non-zero number of bytes,
 * otherwise the number n > 0 of bytes read if no error occurred.
 */

int PB_read_field(FILE *in, PB_Field *fieldp) {
    if (feof(in)) {
        return 0;
    }

    size_t bytes_read = 0;

    int tag_bytes = PB_read_tag(in, &fieldp->type, &fieldp->number);

    if (tag_bytes < 1) {
        return -1;
    }

    bytes_read += tag_bytes;

    int value_bytes = PB_read_value(in, fieldp->type, &fieldp->value);

    if (value_bytes < 1) {
        return -1;
    }

    bytes_read += value_bytes;

    return bytes_read;
}

/**
 * @brief  Read the tag portion of a protocol buffers field and return the
 * wire type and field number.
 * @details  This function reads a varint-encoded 32-bit tag from the
 * input stream in, separates it into a wire type (from the three low-order bits)
 * and a field number (from the 29 high-order bits), and stores them into
 * caller-supplied variables pointed at by parameters typep and fieldp.
 * If the wire type is not within the legal range [0, 5], an error is reported.
 * @param in  The input stream from which data is to be read.
 * @param typep  Pointer to a caller-supplied variable in which the wire type
 * is to be stored.
 * @param fieldp  Pointer to a caller-supplied variable in which the field
 * number is to be stored.
 * @return 0 in case of an immediate end-of-file on the input stream without
 * any error and no input bytes having been read, -1 if there was an error
 * or unexpected end-of-file after reading a non-zero number of bytes,
 * otherwise the number n > 0 of bytes read if no error occurred.
 */

int PB_read_tag(FILE *in, PB_WireType *typep, int32_t *fieldp) {
    if (feof(in)) {
        return 0;
    }

    uint64_t tag;

    size_t bytes_read = PB_read_varint(in, &tag);

    if (bytes_read < 1) {
        return -1;
    }

    *typep = tag & 0x07;
    *fieldp = (tag >> 3);
    return bytes_read;
}

/**
 * @brief  Reads and returns a single value of a specified wire type from a
 * specified input stream.
 * @details  This function reads bytes from the input stream in and interprets
 * them as a single protocol buffers value of the wire type specified by the type
 * parameter.  The number of bytes actually read is variable, and will depend on
 * the wire type and on the particular value read.  The data read is used to
 * initialize the caller-supplied variable pointed at by the valuep parameter.
 * In the case of wire type LEN_TYPE, heap storage will be allocated that is
 * sufficient to hold the number of bytes read and a pointer to this storage
 * will be stored at valuep->bytes.buf.
 * @param in  The input stream from which data is to be read.
 * @param type  The wire type of the value to be read.
 * @param valuep  Pointer to a caller-supplied variable that is to be initialized
 * with the data read.
 * @return 0 in case of an immediate end-of-file on the input stream without
 * any error and no input bytes having been read, -1 if there was an error
 * or unexpected end-of-file after reading a non-zero number of bytes,
 * otherwise the number n > 0 of bytes read if no error occurred.
 */

int PB_read_value(FILE *in, PB_WireType type, union value *valuep) {
    if (feof(in)) {
        return 0;
    }

    size_t bytes_read = 0;

    switch (type) {
        case VARINT_TYPE:
            bytes_read = PB_read_varint(in, &valuep->i64);
            return bytes_read;
        case I64_TYPE:
            bytes_read = fread(&valuep->i64, 1, 8, in);
            if (bytes_read != 8) {
                return -1;
            }
            return bytes_read;
        case LEN_TYPE:
            uint64_t size;
            int bytes = PB_read_varint(in, &size);
            if (bytes < 1) {
                return -1;
            }

            bytes_read += bytes;

            valuep->bytes.size = (size_t)size;
            valuep->bytes.buf = malloc(size);
            bytes_read += size;
            if (fread(valuep->bytes.buf, 1, size, in) != size) {
                return -1;
            }

            return bytes_read;
        case I32_TYPE:
            bytes_read = fread(&valuep->i32, 1, 4, in);
            if (bytes_read != 4) {
                return -1;
            }
            return bytes_read;
        default:
            return -1;
    }

}

/**
 * @brief Get the next field with a specified number from a PB_Message object,
 * scanning the fields in a specified direction starting from a specified previous field.
 * @details  This function iterates through the fields of a PB_Message object,
 * until the first field with the specified number has is encountered or the end of
 * the list of fields is reached.  The list of fields is traversed, either in the
 * forward direction starting from the first field after prev if dir is FORWARD_DIR,
 * or the backward direction starting from the first field before prev if dir is BACKWARD_DIR.
 * When the a field with the specified number is encountered (or, if fnum is ANY_FIELD
 * any field is encountered), the wire type of that field is checked to see if it matches
 * the wire type specified by the type parameter.  Unless ANY_TYPE was passed, an error
 * is reported if the wire type of the field is not equal to the wire type specified.
 * If ANY_TYPE was passed, then this check is not performed.  In case of a mismatch,
 * an error is reported and NULL is returned, otherwise the matching field is returned.
 *
 * @param prev  The field immediately before the first field to be examined.
 * If dir is FORWARD_DIR, then this will be the field immediately preceding the first
 * field to be examined, and if dir is BACKWARD_DIR, then this will be the field
 * immediately following the first field to be examined.
 * @param fnum  Field number to look for.  Unless ANY_FIELD is passed, fields that do
 * not have this number are skipped over.  If ANY_FIELD is passed, then no fields are
 * skipped.
 * @type type  Wire type expected for a matching field.  If the first field encountered
 * with the specified number does not match this type, then an error is reported.
 * The special value ANY_TYPE matches any wire type, disabling this error check.
 * @dir  Direction in which to traverse the fields.  If dir is FORWARD_DIR, then traversal
 * is in the forward direction and if dir is BACKWARD_DIR, then traversal is in the
 * backward direction.
 * @return  The first matching field, or NULL if no matching fields are found, or the
 * first field that matches the specified field number does not match the specified
 * wire type.
 */

PB_Field *PB_next_field(PB_Field *prev, int fnum, PB_WireType type, PB_Direction dir) {
    PB_Field *curr = (dir == FORWARD_DIR) ? prev->next : prev->prev;

    while (curr->type != SENTINEL_TYPE) {
        if (fnum == ANY_FIELD || curr->number == fnum) {
            if (type == ANY_TYPE || curr->type == type) {
                return curr;
            } else {
                fprintf(stderr, "Field number and type are not matching\n");
                return NULL;
            }
        }
        curr = (dir == FORWARD_DIR) ? curr->next : curr->prev;
    }
    return NULL;
}

/**
 * @brief Get a single field with a specified number from a PB_Message object.
 * @details  This is a convenience function for use when it is desired to get just
 * a single field with a specified field number from a PB_Message, rather than
 * iterating through a sequence of fields.  If there is more than one field having
 * the specified number, then the last such field is returned, as required by
 * the protocol buffers specification.
 *
 * @param msg  The PB_Message object from which to get the field.
 * @param fnum  The field number to get.
 * @param type  The wire type expected for the field, or ANY_TYPE if no particular
 * wire type is expected.
 * @return  A pointer to the field, if a field with the specified number exists
 * in the message, and (unless ANY_TYPE was passed) that the type of the field
 * matches the specified wire type.  If there is no field with the specified number,
 * or the last field in the message with the specified field number does not match
 * the specified wire type, then NULL is returned.
 */

PB_Field *PB_get_field(PB_Message msg, int fnum, PB_WireType type) {
    return PB_next_field(msg, fnum, type, BACKWARD_DIR);
}

/**
 * @brief  Output a human-readable representation of a message field
 * to a specified output stream.
 * @details  This function, which is intended only for debugging purposes,
 * outputs a human-readable representation of the message field object
 * pointed to by fp, to the output stream out.  The output may be in any
 * format deemed useful.
 */


/**
 * @brief  Replace packed fields in a message by their expansions.
 * @detail  This function traverses the fields in a message, looking for fields
 * with a specified field number.  For each such field that is encountered,
 * the content of the field is treated as a "packed" sequence of primitive values.
 * The original field must have wire type LEN_TYPE, otherwise an error is reported.
 * The content is unpacked to produce a list of normal (unpacked) fields,
 * each of which has the specified wire type, which must be a primitive type
 * (i.e. not LEN_TYPE) and the specified field number.
 * The message is then modified by splicing in the expanded list in place of
 * the original packed field.
 *
 * @param msg  The message whose fields are to be expanded.
 * @param fnum  The field number of the fields to be expanded.
 * @param type  The wire type expected for the expanded fields.
 * @return 0 in case of success, -1 in case of an error.
 * @modifies  the original message in case any fields are expanded.
 */

int PB_expand_packed_fields(PB_Message msg, int fnum, PB_WireType type) {
    PB_Field *curr = msg->next; //As msg is of SENINEL_TYPE, our execution starts from the next node

    while (curr->type != SENTINEL_TYPE) {
        if (curr->number == fnum) {
            if (curr->type == LEN_TYPE) {
                char *buf = curr->value.bytes.buf;
                size_t size = curr->value.bytes.size;
                FILE *stream = fmemopen(buf, size, "rb");

                if (!stream) return -1;

                while (size > 0) {
                    PB_Field *new_field = malloc(sizeof(PB_Field));

                    if (!new_field) {
                        fclose(stream);
                        return -1;
                    }

                    new_field->number = fnum;
                    new_field->type = type;

                    size_t bytes_read = PB_read_value(stream, type, &new_field->value);
                    if (bytes_read < 0 ) break;
                    size -= bytes_read;

                    new_field->next = msg;
                    new_field->prev = msg->prev;
                    msg->prev->next = new_field;
                    msg->prev = new_field;
                }
                fclose(stream);
            }
            curr = curr->next;
        }
    }
    return 1;
}

void PB_show_field(PB_Field *fp, FILE *out) {
    fprintf(out, "Field number - %d, Field type - %d", fp->number, fp->type);

    if (fp->type == VARINT_TYPE) {
        fprintf(out, " Value - %lu\n", fp->value.i64);
    } else if (fp->type == I64_TYPE) {
        fprintf(out, " Value - %lu\n", fp->value.i64);
    } else if (fp->type == LEN_TYPE) {
        fprintf(out, " Length - %zu\n", fp->value.bytes.size);
    } else if (fp->type == I32_TYPE) {
        fprintf(out, " Value - %u\n", fp->value.i32);
    }
}

/**
 * @brief  Output a human-readable representation of a message object
 * to a specified output stream.
 * @details  This function, which is intended only for debugging purposes,
 * outputs a human-readable representation of the message object msg to
 * the output stream out.  The output may be in any format deemed useful.
 */

void PB_show_message(PB_Message msg, FILE *out) {
    PB_Field *curr = msg->next; //As msg is of SENTINEL_TYPE

    while (curr->type != SENTINEL_TYPE) {
        PB_show_field(curr, out);
        curr = curr->next;
    }
}
