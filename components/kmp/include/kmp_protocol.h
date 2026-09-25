#pragma once

///////////////////////////////////////////////////////////////////////////////
//INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////
//TYPES
///////////////////////////////////////////////////////////////////////////////

typedef enum
{
    KMP_CODE_START_FROM_METER = 0x40,
    KMP_CODE_START_TO_METER = 0x80,
    KMP_CODE_STOP = 0x0D,
    KMP_CODE_ACK = 0x06,
    KMP_CODE_STUFFING = 0x1B
} kmp_code_t;

typedef enum
{
    KMP_STATE_WAIT_START,
    KMP_STATE_RECEIVING_DATA,
    KMP_STATE_RECEIVING_STUFFED_DATA
} kmp_state_t;

typedef enum
{
    KMP_PARSE_INCOMPLETE,
    KMP_PARSE_FRAME_READY,
    KMP_PARSE_INVALID,
    KMP_PARSE_TOO_LARGE
} kmp_parse_result_t;

typedef struct
{
    uint8_t *buffer;
    size_t capacity;
    size_t length;
    kmp_state_t state;
} kmp_parser_t;

///////////////////////////////////////////////////////////////////////////////
//FUNCTION PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initializes the KMP parser with a provided buffer.
 *
 * @param parser Pointer to the kmp_parser_t structure to initialize.
 * @param buffer Pointer to the buffer where parsed data will be stored.
 * @param capacity The total capacity of the provided buffer.
 */
void kmp_parser_init(kmp_parser_t *parser, uint8_t *buffer, size_t capacity);
/**
 * @brief Processes a chunk of data through the KMP parser.
 *
 * @param parser Pointer to the kmp_parser_t structure.
 * @param data Pointer to the input data to be processed.
 * @param length The length of the input data.
 * @param consumed Pointer to a size_t variable where the number of consumed bytes will be stored.
 * @return The result of the parsing operation (e.g., KMP_PARSE_FRAME_READY, KMP_PARSE_INVALID).
 */
kmp_parse_result_t kmp_parser_process(kmp_parser_t *parser, const uint8_t *data, size_t length, size_t *consumed);
