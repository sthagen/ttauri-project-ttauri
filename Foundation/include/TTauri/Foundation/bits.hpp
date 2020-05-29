

namespace TTauri {

/** Read a single bit of span of bytes
 * Bits are ordered LSB first.
 *
 * @param index The index of the bit in the byte span.
 */
[[nodiscard]] inline int get_bit(gsl::span<std::byte> bytes, size_t index) noexcept
{
    auto byte_index = index >> 3;
    auto bit_index = static_cast<uint8_t>(index & 7);

    return (bytes[byte_index] >> bit_index) & 1;
} 

/** Read a single bit of span of bytes
 * Bits are ordered LSB first.
 * Bits are copied as if the byte array is layed out from right to left, example:
 *
 *  7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    byte 1     |    byte 0     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *           :         :
 * index=6   +-+-+-+-+-+
 * length=5  | Return  |
 *           +-+-+-+-+-+
 *            4 3 2 1 0
 *
 * @param index The index of the bit in the byte span.
 * @param length the number of bits to return.
 */
[[nodiscard]] inline int get_bits(gsl::span<std::byte> bytes, size_t index, int length) noexcept
{
    auto value = 0;

    auto todo = length;
    auto done = 0;
    while (todo) {
        auto byte_index = index >> 3;
        auto bit_index = static_cast<int>(index & 7);

        auto available_bits = 8 - bit_index;
        auto nr_bits = available_bits < todo ? available_bits : todo;

        auto mask = (1 << nr_bits) - 1;

        auto tmp = static_cast<int>(bytes[byte_index] >> bit_index) & mask;
        value |= tmp << done;

        todo -= nr_bits;
        done += nr_bits;
        index += nr_bits;
    }

    return value;
} 

struct huffman_symbol {
    int symbol;
    int code;
    int length;
};

void huffman_symbol_table_from_length(std::vector<huffman_symbol> &table)
{
    // Sort the table based on the length of the code, followed by symbol
    std::sort(table.begin(), table.end(), [](let &a, let &b) {
        if (a.length == b.length) {
            return a.symbol < b.symbol;
        } else {
            return a.length < b.length;
        }
    });

    int code = 0;
    int length = 0;
    for (auto &&entry: table) {
        auto shift = entry.length - length;
        code <<= shift;

        entry.code = code;

        if (ttauri_likely(entry.length != 0)) {
            ++code;
        }
    }
}




}
