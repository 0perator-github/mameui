// For licensing and usage information, read docs/winui_license.txt
// ============================================================================

#ifndef MAMEUI_WINAPP_BITMASK_H
#define MAMEUI_WINAPP_BITMASK_H

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <limits>

// Lightweight, high-performance dynamic bit buffer.
// - Uses word-sized storage (uint64_t) for fast bit ops and scanning
// - Header-only, C++17/C++20 compatible

class BitBuffer
{
public:
	using word_t = std::uint64_t; // Use 64-bit words for storage

	// Number of bits in a word
	static inline constexpr std::size_t bits_per_word = sizeof(word_t) * 8;

	// Special value indicating "not found"
	static inline constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

	explicit BitBuffer(std::size_t bits = 0) { resize(bits); }
	~BitBuffer() = default;

	std::size_t size() const noexcept { return m_bits; }

	void resize(std::size_t bits)
	{
		m_bits = bits;
		const std::size_t words = (bits + bits_per_word - 1) / bits_per_word;
		m_words.resize(words);
		if (bits == 0)
			return;

		clear_tail_unused_bits();
	}

	bool empty() const noexcept { return m_bits == 0; }

	void clear() noexcept
	{
		m_bits = 0;
		m_words.clear();
	}

	bool test(std::size_t pos) const noexcept
	{
		if (pos >= m_bits)
			return false;

		const std::size_t wi = pos / bits_per_word;
		const unsigned off = static_cast<unsigned>(pos % bits_per_word);

		return ((m_words[wi] >> off) & word_t(1)) != 0;
	}

	void set(std::size_t pos) noexcept
	{
		if (pos >= m_bits) return;
		const std::size_t wi = pos / bits_per_word;
		const unsigned off = static_cast<unsigned>(pos % bits_per_word);
		m_words[wi] |= (word_t(1) << off);
	}

	void reset(std::size_t pos) noexcept
	{
		if (pos >= m_bits) return;
		const std::size_t wi = pos / bits_per_word;
		const unsigned off = static_cast<unsigned>(pos % bits_per_word);
		m_words[wi] &= ~(word_t(1) << off);
	}

	void reset_all() noexcept
	{
		std::fill(m_words.begin(), m_words.end(), word_t(0));
	}

	void set_all() noexcept
	{
		std::fill(m_words.begin(), m_words.end(), ~word_t(0));
		clear_tail_unused_bits();
	}

	void set_all(bool v) noexcept
	{
		if (v)
			set_all();
		else
			reset_all();
	}

	void flip(std::size_t pos) noexcept
	{
		if (pos >= m_bits) return;
		const std::size_t wi = pos / bits_per_word;
		const unsigned off = static_cast<unsigned>(pos % bits_per_word);
		m_words[wi] ^= (word_t(1) << off);
	}

	void flip_all() noexcept
	{
		for (auto &w : m_words)
			w = ~w;
		clear_tail_unused_bits();
	}

	// Find next bit with value 'v' starting at 'start'. Returns npos if not found.
	std::size_t find_next(std::size_t start, bool v) const noexcept
	{
		if (start >= m_bits) return npos;
		const std::size_t start_w = start / bits_per_word;
		const unsigned start_off = static_cast<unsigned>(start % bits_per_word);

		const std::size_t word_count = m_words.size();
		if (word_count == 0) return npos;

		auto ctz64 = [](word_t x) -> int {
#if defined(_MSC_VER)
			unsigned long idx;
			if (x == 0) return 64;
#if defined(_M_X64) || defined(_M_ARM64)
			_BitScanForward64(&idx, x);
			return static_cast<int>(idx);
#else
			if (_BitScanForward(&idx, static_cast<unsigned long>(x & 0xFFFFFFFF))) return static_cast<int>(idx);
			_BitScanForward(&idx, static_cast<unsigned long>(x >> 32));
			return static_cast<int>(idx + 32);
#endif
#elif defined(__GNUG__) || defined(__clang__)
			return x ? __builtin_ctzll(x) : 64;
#else
			if (x == 0) return 64;
			int i = 0;
			while (((x & 1) == 0) && i < 64) { x >>= 1; ++i; }
			return i;
#endif
			};

		const unsigned last_bits = static_cast<unsigned>(m_bits % bits_per_word);
		const word_t last_mask = (last_bits == 0) ? ~word_t(0) : ((word_t(1) << last_bits) - 1);

		word_t w = m_words[start_w];
		if (start_off)
			w &= (~word_t(0) << start_off);
		if (!v) w = ~w;

		if (start_w == word_count - 1)
			w &= last_mask;

		if (w)
			return start_w * bits_per_word + static_cast<std::size_t>(ctz64(w));

		for (std::size_t wi = start_w + 1; wi < word_count; ++wi)
		{
			word_t ww = m_words[wi];
			if (!v) ww = ~ww;
			if (wi == word_count - 1)
				ww &= last_mask;
			if (ww)
				return wi * bits_per_word + static_cast<std::size_t>(ctz64(ww));
		}

		return npos;
	}

private:
	void clear_tail_unused_bits() noexcept
	{
		if (m_bits == 0) return;
		const unsigned tail = static_cast<unsigned>(m_bits % bits_per_word);
		if (tail == 0) return;
		const std::size_t last = m_words.size() - 1;
		const word_t mask = (word_t(1) << tail) - 1;
		m_words[last] &= mask;
	}

	std::size_t m_bits = 0;
	std::vector<word_t> m_words;
};

// Linked list type (kept minimal)
using Node = struct linked_list_node;
struct linked_list_node
{
	void* data;
	std::shared_ptr<Node> next;
	std::weak_ptr<Node> prev;
};
using LPNODE = std::shared_ptr<Node>;

template<typename T>
inline void set_bit(T& target, bool condition, T bit_mask)
{
	static_assert(std::is_integral<T>::value, "T must be an integral type");
	if (condition)
		target |= bit_mask;
	else
		target &= ~bit_mask;
}

template <typename T1, typename T2>
inline bool is_flag_set(T1 flags, T2 bit_mask)
{
	using ctype = std::common_type_t<T1, T2>;
	static_assert(std::is_integral<ctype>::value, "Arguments must be integral types");

	return (static_cast<ctype>(flags) & static_cast<ctype>(bit_mask)) != 0;
}

#endif // MAMEUI_WINAPP_BITMASK_H
