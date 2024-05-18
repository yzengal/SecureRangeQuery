#include "grid.h"

namespace INDEX {

constexpr size_t ipow(size_t base, int exp, size_t result) {
    return exp < 1 ? result : ipow(base*base, exp/2, (exp % 2) ? result*base : result);
}

}