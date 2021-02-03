
#include <fmt/format.h>
#include <tuple>
#include "forward_value.hpp"

#pragma once

namespace tt {

/** Delayed formatting.
 * This class will capture all the arguments so that it may be passed
 * to another thread. Then call the function operator to do the actual formatting.
 */
template<typename... Values>
class delayed_format {
public:
    delayed_format(delayed_format &&) noexcept = default;
    delayed_format(delayed_format const &) noexcept = default;
    delayed_format &operator=(delayed_format &&) noexcept = default;
    delayed_format &operator=(delayed_format const &) noexcept = default;

    /** Construct a delayed format.
     *
     * All arguments are passed by forwarding-references so that values can be
     * moved into the storage of the delayed_format.
     *
     * Arguments passed by reference will be copied. Arguments passed by std::string_view
     * or std::span will be copied into a std::string or std::vector.
     *
     * Literal strings will not be copied, instead a pointer is taken.
     *
     * @param args The parameters to std::format, including the fmt paramater,
     *             but excluding the locale.
     */
    template<typename... Args>
    delayed_format(Args &&...args) noexcept : _values(forward_value<Args>{}(args)...)
    {
    }

    /** Format now.
     * @return The formatted string.
     */
    [[nodiscard]] std::string operator()() const noexcept
    {
        return std::apply(fmt::format<Values const &...>, _values);
    }

    /** Format now.
     * @param loc The locale to use for formatting.
     * @return The formatted string.
     */
    [[nodiscard]] std::string operator()(std::locale const &loc) const noexcept
    {
        tt_not_implemented();
    }

private:
    std::tuple<Values...> _values;
};

template<typename... Args>
delayed_format(Args &&...) -> delayed_format<forward_value_t<Args>...>;

} // namespace tt
