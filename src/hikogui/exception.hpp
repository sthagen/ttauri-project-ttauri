// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file Utilities for throwing exceptions and terminating the application.
 */

#include "utility.hpp"
#include <exception>
#include <stdexcept>
#include <atomic>
#include <bit>
#include <format>

#pragma once

namespace hi { inline namespace v1 {

/** Message to show when the application is terminated.
 */
inline std::atomic<char const *> terminate_message = nullptr;

/** Set the message to display when the application terminates.
 *
 * The std::terminate() handler will display the __FILE__, __LINE__
 * number and the message to the console or a popup dialogue.
 *
 * @param ... The message to display.
 */
#define hi_set_terminate_message(...) \
    ::hi::terminate_message.store(__FILE__ ":" hi_stringify(__LINE__) ":" __VA_ARGS__, std::memory_order::relaxed)

/** The old terminate handler.
 *
 * This is the handler returned by `std::set_terminate()`.
 */
inline std::terminate_handler old_terminate_handler;

/** The HikoGUI terminate handler.
 *
 * This handler will print an error message on the console or pop-up a dialogue box.
 *
 * @note Use `hi_set_terminate_message()` to set a message.
 */
[[noreturn]] void terminate_handler() noexcept;

/** Exception thrown during parsing on an error.
 * This exception is often thrown due to an error in the syntax
 * in both text and binary files.
 *
 * The what-string should start with the location of the error in the file followed with ": " and the error message.
 * The what-string may be shown to the user, when the parser was working on user supplied files.
 *
 * The location for a text file will be: a path followed by line_nr (starting at line 1) and column_nr (starting at column 1).
 * The location for a binary: a path followed by an optional chunk names, followed by a byte number within the chunk.
 *
 * If there are nested errors, such as an error in an included file, then the what-string may be multiple-lines, where the
 * nested error appears later in the what-string.
 */
class parse_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;

    /** Create a parse error at a specific point in UTF-8 encoded text.
     *
     * @param first An iterator to the start of the text.
     * @param last An iterator to the position of the character where the error occurred.
     * @param tab_size The number of spaces a tab character indents.
     * @param msg A std::format msg.
     * @param args The arguments for std::format.
     */
    template<typename It, typename... Args>
    [[nodiscard]] constexpr parse_error(It first, It last, int tab_size, char const *msg, Args const&...args) noexcept :
        runtime_error(make_what(first, last, tab_size, msg, args...))
    {
    }

    /** Create a parse error at a specific point in UTF-8 encoded text.
     *
     * @param first An iterator to the start of the text.
     * @param last An iterator to the position of the character where the error occurred.
     * @param msg A std::format msg.
     * @param args The arguments for std::format.
     */
    template<typename It, typename... Args>
    [[nodiscard]] constexpr parse_error(It first, It last, char const *msg, Args const&...args) noexcept :
        parse_error(first, last, 8, msg, args...)
    {
    }

    /** Get the line and column position of an iterator in a UTF-8 string.
     *
     * @param first An iterator to the first character of the text.
     * @param last An iterator to the character for which the position need to be found.
     * @param tab_size The number of spaces of indentation for a tab-character.
     * @return Zero based indices for the line and column number.
     */
    template<typename It>
    [[nodiscard]] static constexpr std::pair<size_t, size_t> get_line_position(It first, It last, size_t tab_size) noexcept
    {
        auto line_nr = 0_uz;
        auto column_nr = 0_uz;

        auto c32 = char32_t{};
        auto continue_count = int{};
        while (first != last) {
            hilet c8 = char_cast<char8_t>(*first++);

            if ((c8 & 0xc0) == 0x80) {
                --continue_count;
                c32 <<= 6;
                c32 |= c8 & 0x3f;

            } else if (c8 & 0x80) {
                continue_count = std::countl_one(c8) - 1;
                c32 = c8 & (0b00'111'111 >> continue_count);

            } else {
                continue_count = 0;
                c32 = c8;
            }

            if (not continue_count) {
                ++column_nr;
                switch (c32) {
                case '\n':
                case '\v':
                case '\f':
                case U'\u0085':
                case U'\u2028':
                case U'\u2029':
                    ++line_nr; [[fallthrough]]
                case '\r':
                    column_nr = 0;
                    break;
                case '\t':
                    column_nr /= tab_size;
                    column_nr *= tab_size;
                    break;
                default:;
                }
            }
        }

        return {line_nr, column_nr};
    }

private:
    template<typename It, typename... Args>
    [[nodiscard]] static constexpr std::string
    make_what(It first, It last, size_t tab_size, char const *msg, Args const&...args) noexcept
    {
        hilet[line_nr, column_nr] = get_line_position(first, last, tab_size);
        return {std::format("{}:{}: {}", line_nr + 1, column_nr + 1, std::format(msg, args...))};
    }
};

/** Exception thrown during execution of a dynamic operation.
 * This exception is often thrown on operation between multiple polymorphic objects
 * which do not support the combined operation.
 *
 * For example a datum object may contain floating point number for which
 * a shift-right or shift-left would be an invalid operation.
 */
class operation_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/** Exception thrown during I/O on an error.
 * This exception is often thrown due to an error with permission or existence of files.
 *
 * The what-string should start with the path of the object where the error happened.
 * Followed after ": " with a user-friendly error message. Optionally followed between single quotes
 * the operating system error string.
 */
class io_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/** Exception thrown during an operating system call.
 * This exception is often thrown due to an error with permission or incorrect given parameters
 *
 * The what-string should start with a user-friendly error message.
 * Optionally followed between single quotes the operating system error string.
 */
class os_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class gui_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class key_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class url_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class uri_error : public parse_error {
public:
    using parse_error::parse_error;
};

/** Cancel error is caused by user pressing cancel.
 * Cancels can be cause by a local user pressing cancel in a dialog box,
 * or by a remote user through a network connection.
 */
class cancel_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}} // namespace hi::v1
