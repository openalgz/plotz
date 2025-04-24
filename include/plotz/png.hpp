#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/**
 * PNG Lossless Encoder Implementation
 *
 * An implementation of a PNG encoder supporting RGB and RGBA formats
 * that adheres strictly to the PNG specification.
 *
 * This implementation includes:
 * - PNG chunk structure (IHDR, IDAT, IEND)
 * - PNG filtering (None, Sub, Up, Average, Paeth)
 * - DEFLATE compression with fixed Huffman codes
 * - zlib wrapper (including Adler-32 checksum)
 * - No external dependencies
 */

// PNG signature (8 bytes)
static const uint8_t PNG_SIGNATURE[8] = {137, 80, 78, 71, 13, 10, 26, 10};

// PNG chunk types
#define PNG_IHDR 0x49484452 // "IHDR"
#define PNG_IDAT 0x49444154 // "IDAT"
#define PNG_IEND 0x49454E44 // "IEND"

// PNG color types
#define PNG_COLOR_TYPE_RGB  2
#define PNG_COLOR_TYPE_RGBA 6

// PNG filter types
#define PNG_FILTER_NONE    0
#define PNG_FILTER_SUB     1
#define PNG_FILTER_UP      2
#define PNG_FILTER_AVERAGE 3
#define PNG_FILTER_PAETH   4

// PNG compression type (always 0 for DEFLATE)
#define PNG_COMPRESSION_TYPE 0

// PNG filter method (always 0 for adaptive filtering)
#define PNG_FILTER_METHOD 0

// PNG interlace method (0 for no interlacing)
#define PNG_INTERLACE_NONE 0

// Structure for PNG chunk
typedef struct {
   uint32_t length;    // Length of data (excluding length, type, and CRC)
   uint32_t type;      // Chunk type
   uint8_t *data;      // Chunk data
   uint32_t crc;       // CRC for type and data
} PNGChunk;

// Structure for PNG image
typedef struct {
   uint32_t width;     // Image width
   uint32_t height;    // Image height
   uint8_t bit_depth;  // Bit depth (8 for most cases)
   uint8_t color_type; // Color type (2 for RGB, 6 for RGBA)
   uint8_t *data;      // Image data (RGB or RGBA)
} PNGImage;

// Maximum window size for LZ77
#define LZ77_WINDOW_SIZE 32768  // 32KB, maximum allowed in DEFLATE

// Structure to hold an LZ77 token
typedef struct {
   uint16_t distance;   // Distance to copy from
   uint16_t length;     // Length to copy
   uint8_t literal;     // Literal byte (if length == 0)
} LZ77Token;

// DEFLATE literals/lengths Huffman codes (simplified fixed codes)
typedef struct {
   uint16_t code;   // Huffman code
   uint8_t bits;    // Code length in bits
} HuffmanCode;

// Fixed Huffman codes for literals/lengths and distances
HuffmanCode fixed_literal_length_codes[286];
HuffmanCode fixed_distance_codes[30];

// Tables for length and distance encoding
typedef struct {
   uint16_t base;    // Base value
   uint8_t extra;    // Extra bits required
} CodeTableEntry;

// Length code table
CodeTableEntry length_table[29] = {
   {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0},
   {11, 1}, {13, 1}, {15, 1}, {17, 1}, {19, 2}, {23, 2}, {27, 2}, {31, 2},
   {35, 3}, {43, 3}, {51, 3}, {59, 3}, {67, 4}, {83, 4}, {99, 4}, {115, 4},
   {131, 5}, {163, 5}, {195, 5}, {227, 5}, {258, 0}
};

// Distance code table
CodeTableEntry distance_table[30] = {
   {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 1}, {7, 1}, {9, 2}, {13, 2},
   {17, 3}, {25, 3}, {33, 4}, {49, 4}, {65, 5}, {97, 5}, {129, 6}, {193, 6},
   {257, 7}, {385, 7}, {513, 8}, {769, 8}, {1025, 9}, {1537, 9}, {2049, 10},
   {3073, 10}, {4097, 11}, {6145, 11}, {8193, 12}, {12289, 12}, {16385, 13}, {24577, 13}
};

// Bit buffer for writing compressed data
typedef struct {
   uint8_t *buffer;         // Output buffer
   size_t capacity;         // Buffer capacity
   size_t size;             // Current buffer size
   uint32_t bit_buffer;     // Bit buffer for writing bits
   uint8_t bit_count;       // Number of bits in the bit buffer
} BitBuffer;

// Reverse the bits in a value (for the specified number of bits)
uint32_t reverse_bits(uint32_t value, uint8_t bit_count) {
   uint32_t result = 0;
   for (uint8_t i = 0; i < bit_count; i++) {
      result = (result << 1) | (value & 1);
      value >>= 1;
   }
   return result;
}

// Initialize the fixed Huffman codes
void init_fixed_huffman_codes(void) {
   // Literal/length fixed code assignment per DEFLATE spec:
   // 0-143: 8 bits, codes 00110000 through 10111111 (in other words, codes 48-191)
   // 144-255: 9 bits, codes 110010000 through 111111111 (in other words, codes 400-511)
   // 256-279: 7 bits, codes 0000000 through 0010111 (in other words, codes 0-23)
   // 280-287: 8 bits, codes 11000000 through 11000111 (in other words, codes 192-199)
   
   for (int i = 0; i <= 143; i++) {
      fixed_literal_length_codes[i].code = i + 48;
      fixed_literal_length_codes[i].bits = 8;
   }
   
   for (int i = 144; i <= 255; i++) {
      fixed_literal_length_codes[i].code = (i - 144) + 400;
      fixed_literal_length_codes[i].bits = 9;
   }
   
   for (int i = 256; i <= 279; i++) {
      fixed_literal_length_codes[i].code = (i - 256);
      fixed_literal_length_codes[i].bits = 7;
   }
   
   for (int i = 280; i <= 287; i++) {
      fixed_literal_length_codes[i].code = (i - 280) + 192;
      fixed_literal_length_codes[i].bits = 8;
   }
   
   // Distance fixed code assignment:
   // All distance codes are 5 bits
   for (int i = 0; i < 30; i++) {
      fixed_distance_codes[i].code = i;
      fixed_distance_codes[i].bits = 5;
   }
}

// CRC table for faster CRC calculation
static uint32_t crc_table[256];

// Initialize the CRC table
void init_crc_table(void) {
   for (uint32_t i = 0; i < 256; i++) {
      uint32_t c = i;
      for (int j = 0; j < 8; j++) {
         if (c & 1) {
            c = 0xEDB88320 ^ (c >> 1);
         } else {
            c = c >> 1;
         }
      }
      crc_table[i] = c;
   }
}

// Calculate CRC for a byte array
uint32_t calculate_crc(const uint8_t *data, size_t length) {
   uint32_t crc = 0xFFFFFFFF;
   
   for (size_t i = 0; i < length; i++) {
      crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
   }
   
   return crc ^ 0xFFFFFFFF;
}

// Calculate Adler-32 checksum for zlib
uint32_t calculate_adler32(const uint8_t *data, size_t length) {
   uint32_t a = 1, b = 0;
   const uint32_t MOD_ADLER = 65521; // Largest prime smaller than 65536
   
   for (size_t i = 0; i < length; i++) {
      a = (a + data[i]) % MOD_ADLER;
      b = (b + a) % MOD_ADLER;
   }
   
   return (b << 16) | a;
}

// Initialize a bit buffer
BitBuffer* create_bit_buffer(size_t initial_capacity) {
   BitBuffer *buffer = (BitBuffer*)malloc(sizeof(BitBuffer));
   if (!buffer) return NULL;
   
   buffer->buffer = (uint8_t*)malloc(initial_capacity);
   if (!buffer->buffer) {
      free(buffer);
      return NULL;
   }
   
   buffer->capacity = initial_capacity;
   buffer->size = 0;
   buffer->bit_buffer = 0;
   buffer->bit_count = 0;
   
   return buffer;
}

// Expand bit buffer if needed
bool expand_bit_buffer(BitBuffer *buffer, size_t additional_bytes) {
   if (buffer->size + additional_bytes > buffer->capacity) {
      size_t new_capacity = buffer->capacity * 2;
      if (new_capacity < buffer->size + additional_bytes) {
         new_capacity = buffer->size + additional_bytes;
      }
      
      uint8_t *new_buffer = (uint8_t*)realloc(buffer->buffer, new_capacity);
      if (!new_buffer) return false;
      
      buffer->buffer = new_buffer;
      buffer->capacity = new_capacity;
   }
   
   return true;
}

// Write bits to the bit buffer
void write_bits(BitBuffer *buffer, uint32_t bits, uint8_t bit_count) {
   // In DEFLATE, bits are written LSB first
   buffer->bit_buffer |= (bits << buffer->bit_count);
   buffer->bit_count += bit_count;
   
   while (buffer->bit_count >= 8) {
      if (buffer->size >= buffer->capacity && !expand_bit_buffer(buffer, 1)) {
         return; // Failed to expand buffer
      }
      
      buffer->buffer[buffer->size++] = buffer->bit_buffer & 0xFF;
      buffer->bit_buffer >>= 8;
      buffer->bit_count -= 8;
   }
}

// Flush remaining bits to the bit buffer
void flush_bit_buffer(BitBuffer *buffer) {
   if (buffer->bit_count > 0) {
      if (buffer->size < buffer->capacity || expand_bit_buffer(buffer, 1)) {
         buffer->buffer[buffer->size++] = buffer->bit_buffer & 0xFF;
      }
      buffer->bit_buffer = 0;
      buffer->bit_count = 0;
   }
}

// Free the bit buffer
void free_bit_buffer(BitBuffer *buffer) {
   if (buffer) {
      if (buffer->buffer) {
         free(buffer->buffer);
      }
      free(buffer);
   }
}

// Write bytes to the bit buffer
void write_bytes(BitBuffer *buffer, const uint8_t *data, size_t length) {
   // First, flush any pending bits
   flush_bit_buffer(buffer);
   
   // Then append the data
   if (!expand_bit_buffer(buffer, length)) {
      return; // Failed to expand buffer
   }
   
   memcpy(buffer->buffer + buffer->size, data, length);
   buffer->size += length;
}

// Write a 4-byte integer in big-endian order
void write_uint32(BitBuffer *buffer, uint32_t value) {
   if (!expand_bit_buffer(buffer, 4)) {
      return; // Failed to expand buffer
   }
   
   buffer->buffer[buffer->size++] = (value >> 24) & 0xFF;
   buffer->buffer[buffer->size++] = (value >> 16) & 0xFF;
   buffer->buffer[buffer->size++] = (value >> 8) & 0xFF;
   buffer->buffer[buffer->size++] = value & 0xFF;
}

// PNG filtering functions

// Paeth predictor function as described in the PNG spec
static uint8_t paeth_predictor(int a, int b, int c) {
   int p = a + b - c;
   int pa = abs(p - a);
   int pb = abs(p - b);
   int pc = abs(p - c);
   
   if (pa <= pb && pa <= pc) return a;
   else if (pb <= pc) return b;
   else return c;
}

// Apply the "None" filter (Type 0)
static void apply_none_filter(uint8_t *filtered_row, const uint8_t *row, size_t bytes_per_pixel, size_t row_width) {
   filtered_row[0] = PNG_FILTER_NONE;
   memcpy(filtered_row + 1, row, row_width);
}

// Apply the "Sub" filter (Type 1)
static void apply_sub_filter(uint8_t *filtered_row, const uint8_t *row, size_t bytes_per_pixel, size_t row_width) {
   filtered_row[0] = PNG_FILTER_SUB;
   
   // First bytes_per_pixel bytes are copied as is
   for (size_t i = 0; i < bytes_per_pixel; i++) {
      filtered_row[i + 1] = row[i];
   }
   
   // Remaining bytes are filtered
   for (size_t i = bytes_per_pixel; i < row_width; i++) {
      filtered_row[i + 1] = row[i] - row[i - bytes_per_pixel];
   }
}

// Apply the "Up" filter (Type 2)
static void apply_up_filter(uint8_t *filtered_row, const uint8_t *row, const uint8_t *prev_row,
                            size_t bytes_per_pixel, size_t row_width) {
   filtered_row[0] = PNG_FILTER_UP;
   
   for (size_t i = 0; i < row_width; i++) {
      filtered_row[i + 1] = row[i] - (prev_row ? prev_row[i] : 0);
   }
}

// Apply the "Average" filter (Type 3)
static void apply_average_filter(uint8_t *filtered_row, const uint8_t *row, const uint8_t *prev_row,
                                 size_t bytes_per_pixel, size_t row_width) {
   filtered_row[0] = PNG_FILTER_AVERAGE;
   
   for (size_t i = 0; i < row_width; i++) {
      uint8_t a = (i < bytes_per_pixel) ? 0 : row[i - bytes_per_pixel];
      uint8_t b = prev_row ? prev_row[i] : 0;
      filtered_row[i + 1] = row[i] - ((a + b) / 2);
   }
}

// Apply the "Paeth" filter (Type 4)
static void apply_paeth_filter(uint8_t *filtered_row, const uint8_t *row, const uint8_t *prev_row,
                               size_t bytes_per_pixel, size_t row_width) {
   filtered_row[0] = PNG_FILTER_PAETH;
   
   for (size_t i = 0; i < row_width; i++) {
      uint8_t a = (i < bytes_per_pixel) ? 0 : row[i - bytes_per_pixel];
      uint8_t b = prev_row ? prev_row[i] : 0;
      uint8_t c = (prev_row && i >= bytes_per_pixel) ? prev_row[i - bytes_per_pixel] : 0;
      
      filtered_row[i + 1] = row[i] - paeth_predictor(a, b, c);
   }
}

// Select the best filter for a row based on heuristic
static uint8_t select_best_filter(uint8_t *filtered_rows[], const uint8_t *row, const uint8_t *prev_row,
                                  size_t bytes_per_pixel, size_t row_width) {
   // Apply all filters
   apply_none_filter(filtered_rows[0], row, bytes_per_pixel, row_width);
   apply_sub_filter(filtered_rows[1], row, bytes_per_pixel, row_width);
   apply_up_filter(filtered_rows[2], row, prev_row, bytes_per_pixel, row_width);
   apply_average_filter(filtered_rows[3], row, prev_row, bytes_per_pixel, row_width);
   apply_paeth_filter(filtered_rows[4], row, prev_row, bytes_per_pixel, row_width);
   
   // Select the filter with the smallest sum of absolute values
   uint32_t min_sum = UINT32_MAX;
   uint8_t best_filter = 0;
   
   for (int f = 0; f < 5; f++) {
      uint32_t sum = 0;
      for (size_t i = 1; i <= row_width; i++) {
         sum += abs((int8_t)filtered_rows[f][i]);
      }
      
      if (sum < min_sum) {
         min_sum = sum;
         best_filter = f;
      }
   }
   
   return best_filter;
}

// Create IHDR chunk
PNGChunk* create_ihdr_chunk(const PNGImage *image) {
   PNGChunk *chunk = (PNGChunk*)malloc(sizeof(PNGChunk));
   if (!chunk) return NULL;
   
   chunk->length = 13; // IHDR data is always 13 bytes
   chunk->type = PNG_IHDR;
   chunk->data = (uint8_t*)malloc(chunk->length);
   if (!chunk->data) {
      free(chunk);
      return NULL;
   }
   
   // Write IHDR data (all in big-endian order)
   uint8_t *p = chunk->data;
   
   // Width (4 bytes)
   *p++ = (image->width >> 24) & 0xFF;
   *p++ = (image->width >> 16) & 0xFF;
   *p++ = (image->width >> 8) & 0xFF;
   *p++ = image->width & 0xFF;
   
   // Height (4 bytes)
   *p++ = (image->height >> 24) & 0xFF;
   *p++ = (image->height >> 16) & 0xFF;
   *p++ = (image->height >> 8) & 0xFF;
   *p++ = image->height & 0xFF;
   
   // Bit depth (1 byte)
   *p++ = image->bit_depth;
   
   // Color type (1 byte)
   *p++ = image->color_type;
   
   // Compression method (1 byte, always 0 for DEFLATE)
   *p++ = PNG_COMPRESSION_TYPE;
   
   // Filter method (1 byte, always 0 for adaptive filtering)
   *p++ = PNG_FILTER_METHOD;
   
   // Interlace method (1 byte, 0 for no interlacing)
   *p++ = PNG_INTERLACE_NONE;
   
   // Calculate CRC
   uint8_t type_bytes[4] = {
      uint8_t((chunk->type >> 24) & 0xFF),
      uint8_t((chunk->type >> 16) & 0xFF),
      uint8_t((chunk->type >> 8) & 0xFF),
      uint8_t(chunk->type & 0xFF)
   };
   
   // Create a temporary buffer for CRC calculation
   uint8_t *crc_buffer = (uint8_t*)malloc(4 + chunk->length);
   if (!crc_buffer) {
      free(chunk->data);
      free(chunk);
      return NULL;
   }
   
   memcpy(crc_buffer, type_bytes, 4);
   memcpy(crc_buffer + 4, chunk->data, chunk->length);
   
   chunk->crc = calculate_crc(crc_buffer, 4 + chunk->length);
   free(crc_buffer);
   
   return chunk;
}

// Create IEND chunk
PNGChunk* create_iend_chunk(void) {
   PNGChunk *chunk = (PNGChunk*)malloc(sizeof(PNGChunk));
   if (!chunk) return NULL;
   
   chunk->length = 0; // IEND has no data
   chunk->type = PNG_IEND;
   chunk->data = NULL;
   
   // Calculate CRC for IEND (just the type name)
   uint8_t type_bytes[4] = {
      uint8_t((chunk->type >> 24) & 0xFF),
      uint8_t((chunk->type >> 16) & 0xFF),
      uint8_t((chunk->type >> 8) & 0xFF),
      uint8_t(chunk->type & 0xFF)
   };
   
   chunk->crc = calculate_crc(type_bytes, 4);
   
   return chunk;
}

// Write a chunk to a file
bool write_chunk(FILE *file, const PNGChunk *chunk) {
   // Write length (big-endian)
   uint8_t length_bytes[4] = {
      uint8_t((chunk->length >> 24) & 0xFF),
      uint8_t((chunk->length >> 16) & 0xFF),
      uint8_t((chunk->length >> 8) & 0xFF),
      uint8_t(chunk->length & 0xFF)
   };
   
   if (fwrite(length_bytes, 1, 4, file) != 4) return false;
   
   // Write type (big-endian)
   uint8_t type_bytes[4] = {
      uint8_t((chunk->type >> 24) & 0xFF),
      uint8_t((chunk->type >> 16) & 0xFF),
      uint8_t((chunk->type >> 8) & 0xFF),
      uint8_t(chunk->type & 0xFF)
   };
   
   if (fwrite(type_bytes, 1, 4, file) != 4) return false;
   
   // Write data (if any)
   if (chunk->length > 0 && chunk->data) {
      if (fwrite(chunk->data, 1, chunk->length, file) != chunk->length) return false;
   }
   
   // Write CRC (big-endian)
   uint8_t crc_bytes[4] = {
      uint8_t((chunk->crc >> 24) & 0xFF),
      uint8_t((chunk->crc >> 16) & 0xFF),
      uint8_t((chunk->crc >> 8) & 0xFF),
      uint8_t(chunk->crc & 0xFF)
   };
   
   if (fwrite(crc_bytes, 1, 4, file) != 4) return false;
   
   return true;
}

// Free a chunk
void free_chunk(PNGChunk *chunk) {
   if (chunk) {
      if (chunk->data) {
         free(chunk->data);
      }
      free(chunk);
   }
}

// Find the best match for LZ77 compression
static bool find_match(const uint8_t *data, size_t data_size, size_t pos,
                       uint16_t *match_distance, uint16_t *match_length) {
   // Minimum match length (3 bytes as per DEFLATE spec)
   const size_t MIN_MATCH = 3;
   // Maximum match length (258 bytes as per DEFLATE spec)
   const size_t MAX_MATCH = 258;
   
   // If we don't have at least MIN_MATCH bytes left, don't bother looking
   if (pos + MIN_MATCH > data_size) return false;
   
   // The current maximum match length found
   uint16_t max_length = 0;
   // The distance of the current maximum match
   uint16_t max_distance = 0;
   
   // How far back to look (limited by pos and LZ77_WINDOW_SIZE)
   size_t window_start = (pos > LZ77_WINDOW_SIZE) ? pos - LZ77_WINDOW_SIZE : 0;
   
   // Search for matches
   for (size_t i = window_start; i < pos; i++) {
      // Initial match check
      if (data[i] != data[pos]) continue;
      
      // Count consecutive matching bytes
      size_t length = 0;
      while (pos + length < data_size &&
             data[i + length] == data[pos + length] &&
             length < MAX_MATCH) {
         length++;
      }
      
      // If this match is better than what we've found so far, record it
      if (length >= MIN_MATCH && length > max_length) {
         max_length = length;
         max_distance = pos - i;
         
         // If we've found a match of maximum possible length, stop searching
         if (length == MAX_MATCH) break;
      }
   }
   
   // If we found a match, return it
   if (max_length >= MIN_MATCH) {
      *match_distance = max_distance;
      *match_length = max_length;
      return true;
   }
   
   return false;
}

// Compress data using DEFLATE with fixed Huffman codes
BitBuffer* deflate_compress_fixed(const uint8_t *data, size_t data_size) {
   BitBuffer *output = create_bit_buffer(data_size); // Initial capacity estimate
   if (!output) return NULL;
   
   // Write DEFLATE header for fixed Huffman codes
   // BFINAL = 1 (last block), BTYPE = 01 (fixed Huffman codes)
   write_bits(output, 0b011, 3);
   
   // Process the input data
   size_t pos = 0;
   while (pos < data_size) {
      uint16_t match_distance, match_length;
      
      if (find_match(data, data_size, pos, &match_distance, &match_length)) {
         // Match found - encode as length and distance pair
         // Map match length to a length code (257-285)
         uint16_t length_code = 0;
         uint16_t length_extra_bits = 0;
         uint8_t length_extra_bits_count = 0;
         
         for (int i = 0; i < 29; i++) {
            if (match_length >= length_table[i].base &&
                match_length <= length_table[i].base + (1 << length_table[i].extra) - 1) {
               length_code = i + 257;
               length_extra_bits_count = length_table[i].extra;
               if (length_extra_bits_count > 0) {
                  length_extra_bits = match_length - length_table[i].base;
               }
               break;
            }
         }
         
         // Map match distance to a distance code (0-29)
         uint16_t distance_code = 0;
         uint16_t distance_extra_bits = 0;
         uint8_t distance_extra_bits_count = 0;
         
         for (int i = 0; i < 30; i++) {
            if (match_distance >= distance_table[i].base &&
                match_distance <= distance_table[i].base + (1 << distance_table[i].extra) - 1) {
               distance_code = i;
               distance_extra_bits_count = distance_table[i].extra;
               if (distance_extra_bits_count > 0) {
                  distance_extra_bits = match_distance - distance_table[i].base;
               }
               break;
            }
         }
         
         // Write length code with reverse bits (LSB first for DEFLATE)
         write_bits(output, reverse_bits(fixed_literal_length_codes[length_code].code,
                                         fixed_literal_length_codes[length_code].bits),
                    fixed_literal_length_codes[length_code].bits);
         
         // Write length extra bits (if any)
         if (length_extra_bits_count > 0) {
            write_bits(output, length_extra_bits, length_extra_bits_count);
         }
         
         // Write distance code with reverse bits (LSB first for DEFLATE)
         write_bits(output, reverse_bits(fixed_distance_codes[distance_code].code,
                                         fixed_distance_codes[distance_code].bits),
                    fixed_distance_codes[distance_code].bits);
         
         // Write distance extra bits (if any)
         if (distance_extra_bits_count > 0) {
            write_bits(output, distance_extra_bits, distance_extra_bits_count);
         }
         
         // Advance past the matched data
         pos += match_length;
      } else {
         // No match - encode literal byte with reverse bits (LSB first for DEFLATE)
         write_bits(output, reverse_bits(fixed_literal_length_codes[data[pos]].code,
                                         fixed_literal_length_codes[data[pos]].bits),
                    fixed_literal_length_codes[data[pos]].bits);
         pos++;
      }
   }
   
   // Write end-of-block marker (code 256) with reverse bits (LSB first for DEFLATE)
   write_bits(output, reverse_bits(fixed_literal_length_codes[256].code,
                                   fixed_literal_length_codes[256].bits),
              fixed_literal_length_codes[256].bits);
   
   // Flush any remaining bits
   flush_bit_buffer(output);
   
   return output;
}

// Create zlib header
void write_zlib_header(BitBuffer *buffer) {
   // CMF byte:
   // - Bits 0-3: Compression method (8 = DEFLATE)
   // - Bits 4-7: Compression info (7 = 32K window size)
   uint8_t cmf = (7 << 4) | 8;
   
   // FLG byte:
   // - Bits 0-4: FCHECK (check bits for CMF and FLG)
   // - Bit 5: FDICT (0 = no preset dictionary)
   // - Bits 6-7: FLEVEL (2 = compressor used default compression)
   uint8_t flg = 2 << 6;
   
   // Ensure that CMF and FLG, when viewed as a 16-bit number, are a multiple of 31
   uint16_t cmf_flg = (cmf << 8) | flg;
   uint8_t fcheck = 31 - (cmf_flg % 31);
   flg |= fcheck;
   
   // Write the header bytes
   write_bytes(buffer, &cmf, 1);
   write_bytes(buffer, &flg, 1);
}

// Create zlib footer (Adler-32 checksum)
void write_zlib_footer(BitBuffer *buffer, uint32_t adler32) {
   // Write Adler-32 in big-endian order
   uint8_t adler_bytes[4] = {
      uint8_t((adler32 >> 24) & 0xFF),
      uint8_t((adler32 >> 16) & 0xFF),
      uint8_t((adler32 >> 8) & 0xFF),
      uint8_t(adler32 & 0xFF)
   };
   
   write_bytes(buffer, adler_bytes, 4);
}

// Create IDAT chunk with compressed data
PNGChunk* create_idat_chunk(const uint8_t *filtered_data, size_t filtered_data_size) {
   // Calculate Adler-32 checksum of the uncompressed data
   uint32_t adler32 = calculate_adler32(filtered_data, filtered_data_size);
   
   // Create zlib container
   BitBuffer *zlib_data = create_bit_buffer(filtered_data_size);
   if (!zlib_data) return NULL;
   
   // Write zlib header
   write_zlib_header(zlib_data);
   
   // Compress the filtered data using DEFLATE
   BitBuffer *compressed_data = deflate_compress_fixed(filtered_data, filtered_data_size);
   if (!compressed_data) {
      free_bit_buffer(zlib_data);
      return NULL;
   }
   
   // Append compressed data to zlib container
   write_bytes(zlib_data, compressed_data->buffer, compressed_data->size);
   free_bit_buffer(compressed_data);
   
   // Write zlib footer (Adler-32 checksum)
   write_zlib_footer(zlib_data, adler32);
   
   // Create IDAT chunk
   PNGChunk *chunk = (PNGChunk*)malloc(sizeof(PNGChunk));
   if (!chunk) {
      free_bit_buffer(zlib_data);
      return NULL;
   }
   
   chunk->length = zlib_data->size;
   chunk->type = PNG_IDAT;
   chunk->data = (uint8_t*)malloc(chunk->length);
   
   if (!chunk->data) {
      free(chunk);
      free_bit_buffer(zlib_data);
      return NULL;
   }
   
   // Copy compressed data to chunk
   memcpy(chunk->data, zlib_data->buffer, zlib_data->size);
   
   // Calculate CRC
   uint8_t type_bytes[4] = {
      uint8_t((chunk->type >> 24) & 0xFF),
      uint8_t((chunk->type >> 16) & 0xFF),
      uint8_t((chunk->type >> 8) & 0xFF),
      uint8_t(chunk->type & 0xFF)
   };
   
   // Create a temporary buffer for CRC calculation
   uint8_t *crc_buffer = (uint8_t*)malloc(4 + chunk->length);
   if (!crc_buffer) {
      free(chunk->data);
      free(chunk);
      free_bit_buffer(zlib_data);
      return NULL;
   }
   
   memcpy(crc_buffer, type_bytes, 4);
   memcpy(crc_buffer + 4, chunk->data, chunk->length);
   
   chunk->crc = calculate_crc(crc_buffer, 4 + chunk->length);
   free(crc_buffer);
   
   // Clean up
   free_bit_buffer(zlib_data);
   
   return chunk;
}

// Encode a PNG image
bool encode_png(const PNGImage *image, const char *filename) {
   // Initialize tables
   init_crc_table();
   init_fixed_huffman_codes();
   
   // Open output file
   FILE *file = fopen(filename, "wb");
   if (!file) return false;
   
   // Write PNG signature
   if (fwrite(PNG_SIGNATURE, 1, 8, file) != 8) {
      fclose(file);
      return false;
   }
   
   // Create and write IHDR chunk
   PNGChunk *ihdr = create_ihdr_chunk(image);
   if (!ihdr) {
      fclose(file);
      return false;
   }
   
   if (!write_chunk(file, ihdr)) {
      free_chunk(ihdr);
      fclose(file);
      return false;
   }
   
   free_chunk(ihdr);
   
   // Calculate bytes per pixel
   size_t bytes_per_pixel = (image->color_type == PNG_COLOR_TYPE_RGB) ? 3 : 4;
   
   // Calculate row width in bytes
   size_t row_width = image->width * bytes_per_pixel;
   
   // Allocate memory for filtered data
   size_t filtered_data_size = image->height * (1 + row_width);
   uint8_t *filtered_data = (uint8_t*)malloc(filtered_data_size);
   if (!filtered_data) {
      fclose(file);
      return false;
   }
   
   // Allocate memory for filtered row buffers (for all 5 filter types)
   uint8_t **filtered_rows = (uint8_t**)malloc(5 * sizeof(uint8_t*));
   if (!filtered_rows) {
      free(filtered_data);
      fclose(file);
      return false;
   }
   
   for (int i = 0; i < 5; i++) {
      filtered_rows[i] = (uint8_t*)malloc(1 + row_width);
      if (!filtered_rows[i]) {
         for (int j = 0; j < i; j++) {
            free(filtered_rows[j]);
         }
         free(filtered_rows);
         free(filtered_data);
         fclose(file);
         return false;
      }
   }
   
   // Apply filtering to each row
   size_t filtered_offset = 0;
   const uint8_t *prev_row = NULL;
   
   for (uint32_t y = 0; y < image->height; y++) {
      const uint8_t *row = image->data + y * row_width;
      
      // Select the best filter for this row
      uint8_t best_filter = select_best_filter(filtered_rows, row, prev_row, bytes_per_pixel, row_width);
      
      // Copy the best filtered row to the filtered_data buffer
      memcpy(filtered_data + filtered_offset, filtered_rows[best_filter], 1 + row_width);
      filtered_offset += 1 + row_width;
      
      // Update prev_row for the next iteration
      prev_row = row;
   }
   
   // Free filtered row buffers
   for (int i = 0; i < 5; i++) {
      free(filtered_rows[i]);
   }
   free(filtered_rows);
   
   // Create and write IDAT chunk
   PNGChunk *idat = create_idat_chunk(filtered_data, filtered_data_size);
   free(filtered_data);
   
   if (!idat) {
      fclose(file);
      return false;
   }
   
   if (!write_chunk(file, idat)) {
      free_chunk(idat);
      fclose(file);
      return false;
   }
   
   free_chunk(idat);
   
   // Create and write IEND chunk
   PNGChunk *iend = create_iend_chunk();
   if (!iend) {
      fclose(file);
      return false;
   }
   
   if (!write_chunk(file, iend)) {
      free_chunk(iend);
      fclose(file);
      return false;
   }
   
   free_chunk(iend);
   
   // Close file
   fclose(file);
   
   return true;
}

// Create a PNG image from RGB or RGBA pixel data
PNGImage* create_png_image(uint32_t width, uint32_t height, uint8_t color_type, const uint8_t *pixel_data) {
   if (color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA) {
      return NULL; // Only RGB and RGBA are supported
   }
   
   PNGImage *image = (PNGImage*)malloc(sizeof(PNGImage));
   if (!image) return NULL;
   
   image->width = width;
   image->height = height;
   image->bit_depth = 8; // Always 8 bits per channel
   image->color_type = color_type;
   
   // Calculate data size
   size_t bytes_per_pixel = (color_type == PNG_COLOR_TYPE_RGB) ? 3 : 4;
   size_t data_size = width * height * bytes_per_pixel;
   
   image->data = (uint8_t*)malloc(data_size);
   if (!image->data) {
      free(image);
      return NULL;
   }
   
   // Copy pixel data
   memcpy(image->data, pixel_data, data_size);
   
   return image;
}

// Free a PNG image
void free_png_image(PNGImage *image) {
   if (image) {
      if (image->data) {
         free(image->data);
      }
      free(image);
   }
}
