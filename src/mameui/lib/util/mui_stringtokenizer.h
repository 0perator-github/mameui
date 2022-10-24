// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_STRINGTOKENIZER_H
#define MAMEUI_LIB_UTIL_MUI_STRINGTOKENIZER_H

#pragma once

template<typename CharT>
struct numeric_parser;

template<>
struct numeric_parser<char>
{
	static std::optional<int32_t> parse_int(const std::string &str)
	{
		try
		{
			return std::stoi(str);
		}
		catch (...) {
			return std::nullopt;
		}
	}

	static std::optional<int64_t> parse_long(const std::string &str)
	{
		try
		{
			return std::stoll(str);
		}
		catch (...) {
			return std::nullopt;
		}
	}

	static std::optional<uint32_t> parse_uint(const std::string &str)
	{
		try
		{
			auto tmp = std::stoull(str);
			if (tmp <= std::numeric_limits<uint32_t>::max())
				return static_cast<uint32_t>(tmp);
		}
		catch (...) {}
		return std::nullopt;
	}
};

template<>
struct numeric_parser<wchar_t>
{
	static std::optional<int32_t> parse_int(const std::wstring &str)
	{
		try
		{
			return std::stoi(str);
		}
		catch (...)
		{
			return std::nullopt;
		}
	}

	static std::optional<int64_t> parse_long(const std::wstring &str)
	{
		try
		{
			return std::stoll(str);
		}
		catch (...)
		{
			return std::nullopt;
		}
	}

	static std::optional<uint32_t> parse_uint(const std::wstring &str)
	{
		try {
			auto tmp = std::stoull(str);
			if (tmp <= std::numeric_limits<uint32_t>::max())
				return static_cast<uint32_t>(tmp);
		}
		catch (...) {}
		return std::nullopt;
	}
};

namespace mameui::util
{
	//===========================================================
	// stringtokenizer_t
	//
	// Thread-safe string tokenizer.
	// - Supports forward iteration
	// - Custom delimiter support
	// - Quoted string and escape character handling
	//
	// Template:
	//   CharT - character type (e.g., char, wchar_t)
	//===========================================================
	template<typename CharT>
	class stringtokenizer_t
	{
	public:
		// Type aliases
		using string_type = std::basic_string<CharT>;
		using string_view_type = std::basic_string_view<CharT>;
		using size_type = typename string_type::size_type;
		using delimiters_type = std::unordered_set<CharT>;

	private:
		// Immutable container for input string and delimiter set
		struct snapshot_t
		{
			string_type input;
			delimiters_type delimiters;

			snapshot_t() = default;
			snapshot_t(string_view_type in, delimiters_type delim)
				: input(in), delimiters(std::move(delim))
			{
			}
		};
		using snapshot_ptr = std::shared_ptr<const snapshot_t>; // Thread-safe, immutable snapshot of input state.

	public:
		//============================================================
		// stringtokenizer_cursor
		//
		// Cursor for iterating over tokens.
		// - Input iterator interface
		// - next_token() / next_token_cstr()
		// - remaining_tokens(), remaining_tokens_cstr()
		// - reset() to restart iteration
		// - set_quote_support() to enable/disable quote handling
		//============================================================
		class stringtokenizer_cursor
		{
		public:
			using iterator_category = std::input_iterator_tag;
			using value_type = string_type;
			using difference_type = std::ptrdiff_t;
			using pointer = const string_type*;
			using reference = const string_type&;

			// -- constructors --
			stringtokenizer_cursor() : m_pos(0), m_snapshot(nullptr), m_end_reached(true), m_support_quotes(true), m_skip_empty_tokens(true)
			{
			}

			explicit stringtokenizer_cursor(snapshot_ptr snapshot)
				: m_snapshot(std::move(snapshot)), m_pos(0), m_end_reached(false), m_support_quotes(true), m_skip_empty_tokens(true)
			{
				++(*this); // Initialize first token
			}

			// -- copy constructor and assignment operator --
			reference operator*() const { return m_current_token; }
			pointer operator->() const { return &m_current_token; }

			// -- increment operators --
			stringtokenizer_cursor& operator++()
			{
				if (!m_snapshot || m_end_reached)
					return end_state();

				const auto& input = m_snapshot->input;
				const auto& delims = m_snapshot->delimiters;
				auto sz = input.size();

				if (m_skip_empty_tokens)
				{
					skip_leading_delimiters(input, delims, sz);
					if (m_pos >= sz)
						return end_state();

					m_current_token = extract_token(input, delims, sz);
				}
				else
				{
					// If current char is a delimiter, that means empty token
					if (m_pos >= sz)
						return end_state();

					if (delims.count(input[m_pos]) > 0)
					{
						m_current_token.clear();
						++m_pos;
					}
					else
					{
						m_current_token = extract_token(input, delims, sz);
					}
				}

				return *this;
			}

			stringtokenizer_cursor operator++(int)
			{
				stringtokenizer_cursor tmp = *this;
				++(*this);
				return tmp;
			}

			// -- comparison operators --
			bool operator==(const stringtokenizer_cursor& other) const
			{
				return m_end_reached == other.m_end_reached &&
					m_snapshot == other.m_snapshot &&
					m_pos == other.m_pos;
			}

			bool operator!=(const stringtokenizer_cursor& other) const
			{
				return !(*this == other);
			}

			// -- public methods --

			// Check if there are more tokens available
			bool has_next() const
			{
				return !m_end_reached;
			}

			// Returns the next token as a string, or std::nullopt if no more tokens are available
			std::optional<string_type> next_token()
			{
				if (m_end_reached)
					return std::nullopt;

				auto tok = std::move(m_current_token);
				++(*this);

				return tok;
			}

			// Returns the next token as a C-style string (null-terminated)
			const CharT* next_token_cstr()
			{
				if (m_end_reached)
					return nullptr;

				m_cstr_storage.assign(m_current_token.begin(), m_current_token.end());
				m_cstr_storage.push_back(CharT(0));

				++(*this);
				return m_cstr_storage.data();
			}

			// Returns the next token as a 32-bit signed integer, or std::nullopt if invalid
			std::optional<int32_t> next_token_int()
			{
				if (m_end_reached)
					return std::nullopt;

				auto value = numeric_parser<CharT>::parse_int(m_current_token);
				++(*this);
				return value;
			}

			// Returns the next token as a 64-bit signed integer, or std::nullopt if invalid
			std::optional<int64_t> next_token_long()
			{
				if (m_end_reached)
					return std::nullopt;

				auto value = numeric_parser<CharT>::parse_long(m_current_token);
				++(*this);
				return value;
			}

			// Returns the next token as a 32-bit unsigned integer, or std::nullopt if invalid
			std::optional<uint32_t> next_token_uint()
			{
				if (m_end_reached)
					return std::nullopt;

				auto value = numeric_parser<CharT>::parse_uint(m_current_token);
				++(*this);
				return value;
			}

			std::vector<string_type> remaining_tokens()
			{
				std::vector<string_type> result;
				while (auto tok = next_token())
					result.push_back(std::move(*tok));
				return result;
			}

			std::vector<const CharT*> remaining_tokens_cstr()
			{
				m_cstr_storage.clear();
				m_cstr_pointers.clear();

				constexpr size_t kInitialReserve = 256;
				m_cstr_storage.reserve(kInitialReserve);

				for (const auto& token : remaining_tokens())
				{
					size_t old_size = m_cstr_storage.size();
					m_cstr_storage.insert(m_cstr_storage.end(), token.begin(), token.end());
					m_cstr_storage.push_back(CharT(0)); // null terminator
					m_cstr_pointers.push_back(m_cstr_storage.data() + old_size);
				}

				m_cstr_pointers.push_back(nullptr); // End marker
				return m_cstr_pointers;
			}

			void reset()
			{
				if (!m_snapshot)
				{
					end_state();
					return;
				}

				m_cstr_storage.clear();
				m_cstr_pointers.clear();
				m_pos = 0;
				m_end_reached = false;
				++(*this);
			}

			void set_quote_support(bool enabled)
			{
				m_support_quotes = enabled;
			}

			// New: Enable or disable skipping empty tokens (default true)
			void set_skip_empty_tokens(bool enabled)
			{
				m_skip_empty_tokens = enabled;
			}

		private:
			void skip_leading_delimiters(const string_type& input, const delimiters_type& delims, size_type sz)
			{
				while (m_pos < sz && delims.count(input[m_pos]) > 0)
					++m_pos;
			}

			string_type extract_token(const string_type& input, const delimiters_type& delims, size_type sz)
			{
				string_type token;
				bool in_quotes = false;
				bool escape = false;
				const CharT quote_char = CharT('"');
				const CharT escape_char = CharT('\\');

				while (m_pos < sz)
				{
					CharT ch = input[m_pos++];

					if (escape)
					{
						token += ch;
						escape = false;
					}
					else if (ch == escape_char)
					{
						escape = true;
					}
					else if (m_support_quotes && ch == quote_char)
					{
						if (!in_quotes)
						{
							in_quotes = true;
						}
						else if (m_pos < sz && input[m_pos] == quote_char)
						{
							token += quote_char;
							++m_pos;
						}
						else
						{
							in_quotes = false;
						}
					}
					else if (!in_quotes && delims.count(ch))
					{
						break; // End of token
					}
					else
					{
						token += ch;
					}
				}

				return token;
			}

			stringtokenizer_cursor& end_state()
			{
				m_end_reached = true;
				m_current_token.clear();
				return *this;
			}

			snapshot_ptr m_snapshot;
			size_type m_pos;
			string_type m_current_token;
			bool m_end_reached;
			bool m_support_quotes;
			bool m_skip_empty_tokens;

			std::vector<CharT> m_cstr_storage;
			std::vector<const CharT*> m_cstr_pointers;
		};

		// -- constructors --
		stringtokenizer_t() : m_snapshot(std::make_shared<snapshot_t>()) {}

		stringtokenizer_t(string_view_type input, string_view_type delimiters)
			: m_snapshot(std::make_shared<snapshot_t>(input, delimiters_to_set(delimiters)))
		{
		}

		// -- setters --

		// Set both the input string and delimiters
		void set_input(string_view_type input, string_view_type delimiters)
		{
			auto delim_set = delimiters_to_set(delimiters);
			auto new_snap = std::make_shared<snapshot_t>(input, std::move(delim_set));
			std::lock_guard<std::mutex> lock(m_snapshot_mutex);
			m_snapshot = std::move(new_snap);
		}

		// Only set the input string
		void set_input(string_view_type input)
		{
			std::lock_guard<std::mutex> lock(m_snapshot_mutex);
			auto old_snap = m_snapshot;
			auto new_snap = std::make_shared<snapshot_t>(input, old_snap ? old_snap->delimiters : delimiters_type{});
			m_snapshot = std::move(new_snap);
		}

		// Only set the delimiters
		void set_delimiters(string_view_type delimiters)
		{
			std::lock_guard<std::mutex> lock(m_snapshot_mutex);
			auto old_snap = m_snapshot;
			auto delim_set = delimiters_to_set(delimiters);
			auto new_snap = std::make_shared<snapshot_t>(old_snap ? old_snap->input : string_type{}, std::move(delim_set));
			m_snapshot = std::move(new_snap);
		}

		// -- getters --

		// Get the current input string
		string_type get_input() const
		{
			std::lock_guard<std::mutex> lock(m_snapshot_mutex);
			return m_snapshot ? m_snapshot->input : string_type{};
		}

		// Get the current delimiters as a string
		string_type get_delimiters() const
		{
			std::lock_guard<std::mutex> lock(m_snapshot_mutex);
			if (!m_snapshot)
				return string_type{};

			string_type result;
			result.reserve(m_snapshot->delimiters.size());
			for (CharT ch : m_snapshot->delimiters)
				result.push_back(ch);
			return result;
		}

		// -- iterators --

		// Create a cursor with default behavior (quotes enabled, skip empty tokens enabled)
		stringtokenizer_cursor cursor() const
		{
			std::lock_guard<std::mutex> lock(m_snapshot_mutex);
			return stringtokenizer_cursor(m_snapshot);
		}

		// Create a cursor with configurable options
		stringtokenizer_cursor cursor(bool quote_support, bool skip_empty_tokens) const
		{
			std::lock_guard<std::mutex> lock(m_snapshot_mutex);
			stringtokenizer_cursor new_cursor(m_snapshot);
			new_cursor.set_quote_support(quote_support);
			new_cursor.set_skip_empty_tokens(skip_empty_tokens);
			return new_cursor;
		}

		// Get an iterator that starts at the beginning of the tokens
		stringtokenizer_cursor begin() const
		{
			return cursor();
		}

		// Get an iterator that represents the end of the tokens
		stringtokenizer_cursor end() const
		{
			return stringtokenizer_cursor(); // default constructed end iterator
		}

	private:
		// Convert delimiter string to set
		static inline delimiters_type delimiters_to_set(string_view_type delimiters)
		{
			delimiters_type set;
			for (auto ch : delimiters)
				set.emplace(ch);
			return set;
		}

		mutable std::mutex m_snapshot_mutex;
		std::shared_ptr<snapshot_t> m_snapshot;
	};

	using stringtokenizer = stringtokenizer_t<char>;
	extern template class stringtokenizer_t<char>;

	using wstringtokenizer = stringtokenizer_t<wchar_t>;
	extern template class stringtokenizer_t<wchar_t>;

#if 0
#if defined(UNICODE) || defined(_UNICODE)
	using tstringtokenizer = stringtokenizer_t<wchar_t>;
	extern template class stringtokenizer_t<wchar_t>;
#else
	using tstringtokenizer = stringtokenizer_t<char>;
	extern template class stringtokenizer_t<char>;
#endif
#endif

} // namespace mameui::util

#endif // MAMEUI_LIB_UTIL_MUI_STRINGTOKENIZER_H
