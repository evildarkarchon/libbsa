#pragma once

namespace libbsa::formats::ba2 {

/// BA2 archive subtype selected by the `BTDX` subtype field.
enum class ba2_subtype {
    gnrl,
    dx10,
};

}  // namespace libbsa::formats::ba2
