#include "./validate.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

int
main (int argc, char **argv)
{
   if (argc != 2) {
      std::fprintf (stderr, "Usage: %s <filepath>\n", argv[0]);
      return 2;
   }

   std::ifstream infile{argv[1], std::ios::binary};
   if (!infile) {
      std::fprintf (stderr, "Failed to open file [%s]: %s\n", argv[1], std::strerror (errno));
      return 3;
   }
   std::stringstream sbuf;
   sbuf << infile.rdbuf ();
   auto s = sbuf.str ();
   int rc = validate_one_input (reinterpret_cast<const std::uint8_t *> (s.data ()), s.size ());
   if (rc == 0) {
      std::fprintf (stderr, "Validation returned normally (no bug?)\n");
   } else if (rc == -1) {
      std::fprintf (stderr, "Test case was rejected by the fuzzer (not a valid corpus item?)\n");
   } else {
      std::fprintf (stderr, "Test case returned unexpected result code %d (huh?)\n", rc);
   }
   return rc;
}
