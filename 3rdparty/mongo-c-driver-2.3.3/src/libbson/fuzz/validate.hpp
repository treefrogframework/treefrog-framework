#include <bson/bson.h>

#include <cstdint>
#include <cstring>
#include <vector>

/**
 * @brief Do a BSON validation on a single test case.
 *
 * The test case input consists of a single byte specifying the validation flags,
 * followed by element bytes to be validated.
 *
 * The low six bits of the head byte are treated as flags. If any other bits are
 * set, then the case will be rejected from the corpus to prevent redundant cases since those
 * bits cannot effect the outcome.
 *
 * The low five bits are cast into bson_validate_flags_t, and the sixth bit tells us whether we
 * will pass an error offset pointer.
 *
 * The `data` points to the inner contents of a bson document, not to the document header. We
 * construct a buffer that contains a valid BSON header and trailer to wrap around these bytes before
 * we do the validation.
 */
inline int
validate_one_input(const uint8_t *data, size_t size)
{
   if (size < 1) {
      // We need at least one byte
      return -1;
   }
   const int six_bits = 0b111111;
   const uint8_t head_byte = data[0];
   if ((head_byte & six_bits) != head_byte) {
      // Don't allow any more than six bits to be set in the first byte
      return -1;
   }
   // Grab the five low bits as control flags
   const bson_validate_flags_t flags = static_cast<bson_validate_flags_t>(data[0] & (six_bits >> 1));
   // If bit 6 is set, pass an offset pointer
   const bool pass_offset = head_byte & (1 << 5);

   // Treat the first byte as the flags, and the BSON data is what remains:
   const uint8_t *bson_doc_data = data + 1;
   const size_t bson_data_size = size - 1;
   if (bson_data_size > static_cast<size_t>(INT32_MAX)) {
      // This is very unlikely to happen, but reject this
      return -1;
   }

   // vec that will hold the entire document
   std::vector<std::uint8_t> bson_doc;
   // Prepare the region. +4 for header, +1 for null terminator
   bson_doc.resize(bson_data_size + 5, 0);
   // Insert the header
   const uint32_t header_i32 = static_cast<uint32_t>(bson_doc.size());
   const uint32_t header_i32le = BSON_UINT32_TO_LE(header_i32);
   std::memcpy(bson_doc.data(), &header_i32le, sizeof header_i32le);
   // Insert the inner data:
   std::memcpy(bson_doc.data() + 4, bson_doc_data, bson_data_size);
   // std::vector will have already added a null terminator on resize()

   bson_t b = BSON_INITIALIZER;
   if (!bson_init_static(&b, bson_doc.data(), bson_doc.size())) {
      // Basic header/trailer validation failed. This should not occur since we constructed it
      // correctly ourselves.
      abort();
   }
   size_t offset = 0;
   size_t *offset_ptr = pass_offset ? &offset : NULL;
   bson_validate(&b, (bson_validate_flags_t)flags, offset_ptr);
   return 0;
}
