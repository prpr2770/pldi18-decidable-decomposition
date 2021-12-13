#include "common.h"

void assert(bool b, const std::string& msg) {
  if (!b) {
    throw failed_assertion(msg);
  }
}
