// Every constant of mcpplibs.mwrap.openssl.macros against its macro, in a translation unit that includes the
// headers (the side header, first) and imports the module: same type, same value. The comparisons are
// written by build.mcpp from src/surface.tsv (constants_check.inc), one static_assert a constant.
#include <mwrap/openssl/macros.hpp>
#include <string_view>
#include <type_traits>
#include <gtest/gtest.h>

import mcpplibs.mwrap.openssl;

namespace {
template <class A, class B>
constexpr bool same(A a, B b) {
    if constexpr (!std::is_same_v<A, B>) {
        return false;
    } else if constexpr (std::is_pointer_v<A> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<A>>, char>) {
        return std::string_view(a) == std::string_view(b);
    } else {
        return a == b;
    }
}
}  // namespace

#include "constants_check.inc"

TEST(Constants, EveryMappingIsItsMacro) { SUCCEED() << constants_compared << " constants compared at compile time"; }
