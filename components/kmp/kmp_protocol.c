///////////////////////////////////////////////////////////////////////////////
//INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "kmp_protocol.h"

///////////////////////////////////////////////////////////////////////////////
//DEFINES
///////////////////////////////////////////////////////////////////////////////

#define MIN_BUFFER_CAPACITY 6

///////////////////////////////////////////////////////////////////////////////
//TYPES
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//VARIABLES
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTION PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

static uint16_t crc16_update(uint16_t crc, uint8_t byte);
static uint16_t crc16(const uint8_t *data, size_t length);
static inline bool is_stuffable(uint8_t byte);
static inline void reset_parser(kmp_parser_t *parser);
static inline bool append_byte(kmp_parser_t *parser, uint8_t byte);
static inline bool verify_crc(const kmp_parser_t *parser);
static bool add_stuffed_byte(uint8_t *output, size_t capacity, size_t *length, uint8_t byte);

///////////////////////////////////////////////////////////////////////////////
//FUNCTIONS
///////////////////////////////////////////////////////////////////////////////


void kmp_parser_init(kmp_parser_t *parser, uint8_t *buffer, size_t capacity)
{
    assert(parser != NULL);
    assert(buffer != NULL);
    assert(capacity >= MIN_BUFFER_CAPACITY);

    parser->buffer = buffer;
    parser->capacity = capacity;
    reset_parser(parser);
}

kmp_parse_result_t kmp_parser_process(kmp_parser_t *parser, const uint8_t *data, size_t length, size_t *consumed)
{
    assert(parser != NULL);
    assert(data != NULL);
    assert(consumed != NULL);

    *consumed = 0;

    for (size_t i = 0; i < length; ++i)
    {
        const uint8_t byte = data[i];
        *consumed = i + 1;

        switch (parser->state)
        {
            case KMP_STATE_WAIT_START:
            {
                if (byte == KMP_CODE_START_FROM_METER)
                {
                    parser->state = KMP_STATE_RECEIVING_DATA;
                    parser->length = 0;
                }
                break;
            }

            case KMP_STATE_RECEIVING_DATA:
            {
                if (byte == KMP_CODE_STOP)
                {
                    if (!verify_crc(parser))
                    {
                        reset_parser(parser);
                        return KMP_PARSE_INVALID;
                    }

                    parser->length -= 2;
                    parser->state = KMP_STATE_WAIT_START;
                    return KMP_PARSE_FRAME_READY;
                }
                else if (byte == KMP_CODE_STUFFING)
                {
                    parser->state = KMP_STATE_RECEIVING_STUFFED_DATA;
                    break;
                }

                if (!append_byte(parser, byte))
                {
                    reset_parser(parser);
                    return KMP_PARSE_TOO_LARGE;
                }

                break;
            }

            case KMP_STATE_RECEIVING_STUFFED_DATA:
            {
                const uint8_t decoded = byte ^ 0xFF;

                if (!is_stuffable(decoded))
                {
                    reset_parser(parser);
                    return KMP_PARSE_INVALID;
                }

                if (!append_byte(parser, decoded))
                {
                    reset_parser(parser);
                    return KMP_PARSE_TOO_LARGE;
                }

                parser->state = KMP_STATE_RECEIVING_DATA;
                break;
            }

            default:
            {
                assert(false);
                reset_parser(parser);
                return KMP_PARSE_INVALID;
            }
        }
    }

    return KMP_PARSE_INCOMPLETE;
}

const char *kmp_parse_result_to_string(kmp_parse_result_t result)
{
    switch (result)
    {
        case KMP_PARSE_INCOMPLETE:
            return "INCOMPLETE";
        case KMP_PARSE_FRAME_READY:
            return "FRAME_READY";
        case KMP_PARSE_INVALID:
            return "INVALID";
        case KMP_PARSE_TOO_LARGE:
            return "TOO_LARGE";
        default:
            return "UNKNOWN";
    }
}

kmp_encode_result_t kmp_frame_encode(destination_address_t destination,
                                     const uint8_t *input,
                                     size_t input_length,
                                     uint8_t *output,
                                     size_t capacity,
                                     size_t *output_length)
{
    assert(input != NULL);
    assert(output != NULL);
    assert(output_length != NULL);

    size_t i = 0;

    if (i >= capacity)
    {
        return KMP_ENCODE_TOO_LARGE;
    }
    output[i++] = KMP_CODE_START_TO_METER;

    uint16_t crc = crc16_update(0x0000, (uint8_t)destination);
    if (!add_stuffed_byte(output, capacity, &i, (uint8_t)destination))
    {
        return KMP_ENCODE_TOO_LARGE;
    }

    for (size_t j = 0; j < input_length; ++j)
    {
        crc = crc16_update(crc, input[j]);
        if (!add_stuffed_byte(output, capacity, &i, input[j]))
        {
            return KMP_ENCODE_TOO_LARGE;
        }
    }

    // MSB first
    if (!add_stuffed_byte(output, capacity, &i, (uint8_t)(crc >> 8)) ||
        !add_stuffed_byte(output, capacity, &i, (uint8_t)(crc & 0xFF)))
    {
        return KMP_ENCODE_TOO_LARGE;
    }

    if (i >= capacity)
    {
        return KMP_ENCODE_TOO_LARGE;
    }

    output[i++] = KMP_CODE_STOP;
    *output_length = i;
    return KMP_ENCODE_OK;
}

///////////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static inline void reset_parser(kmp_parser_t *parser)
{
    parser->state = KMP_STATE_WAIT_START;
    parser->length = 0;
}

static inline bool append_byte(kmp_parser_t *parser, uint8_t byte)
{
    if (parser->length >= parser->capacity)
    {
        return false;
    }

    parser->buffer[parser->length++] = byte;
    return true;
}

static uint16_t crc16_update(uint16_t crc, uint8_t byte)
{
    crc ^= (uint16_t)byte << 8;

    for (int j = 0; j < 8; ++j)
    {
        if (crc & 0x8000)
        {
            crc = (crc << 1) ^ 0x1021;
        }
        else
        {
            crc <<= 1;
        }
    }

    return crc;
}

static uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0x0000;

    for (size_t i = 0; i < length; ++i)
    {
        crc = crc16_update(crc, data[i]);
    }

    return crc;
}

static inline bool is_stuffable(uint8_t byte)
{
    switch (byte)
    {
        case KMP_CODE_START_FROM_METER:
        case KMP_CODE_START_TO_METER:
        case KMP_CODE_STOP:
        case KMP_CODE_ACK:
        case KMP_CODE_STUFFING:
            return true;
        default:
            return false;
    }
}

static inline bool verify_crc(const kmp_parser_t *parser)
{
    // Minimum: 1 destination byte + 2 CRC bytes
    if (parser->length < 3)
    {
        return false;
    }

    const uint16_t calculated_crc = crc16(parser->buffer, parser->length - 2);
    const uint16_t received_crc =
        ((uint16_t)parser->buffer[parser->length - 2] << 8) | parser->buffer[parser->length - 1];

    return calculated_crc == received_crc;
}

static bool add_stuffed_byte(uint8_t *output, size_t capacity, size_t *length, uint8_t byte)
{
    if (is_stuffable(byte))
    {
        // Check if the output buffer has room for a stuffed byte
        if (*length + 2 > capacity)
        {
            return false;
        }

        output[(*length)++] = KMP_CODE_STUFFING;
        output[(*length)++] = byte ^ 0xFF;
    }
    else
    {
        if (*length >= capacity)
        {
            return false;
        }

        output[(*length)++] = byte;
    }

    return true;
}
