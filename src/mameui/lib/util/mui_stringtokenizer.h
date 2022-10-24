//	license:BSD-3-Clause
//	copyright-holders:0perator
//	===========================================================================

#ifndef MAMEUI_LIB_UTIL_MUI_STRINGTOKENIZER_H
#define MAMEUI_LIB_UTIL_MUI_STRINGTOKENIZER_H

#pragma once

namespace mameui::util::string_util
{
	// numeric_parser<>
	// Generic numeric parsing helpers parameterized by character type.
	// Specializations provide convenient conversions from string to numeric
	// types returning std::optional to signal parse failures.
	template<typename CharT>
	struct numeric_parser;

	// Specialization for char
	// Uses std::stoi/stoll/stoull family for conversions and catches exceptions.
	template<>
	struct numeric_parser<char>
	{
		// Parse int32_t from std::string, return nullopt on failure.
		static std::optional<int32_t> parse_int(const std::string &str)
		{
			try { return std::stoi(str); }
			catch (...) { return std::nullopt; }
		}
		// Parse int64_t from std::string, return nullopt on failure.
		static std::optional<int64_t> parse_long(const std::string &str)
		{
			try { return std::stoll(str); }
			catch (...) { return std::nullopt; }
		}
		// Parse uint32_t from std::string, ensuring the parsed value fits.
		static std::optional<uint32_t> parse_uint(const std::string &str)
		{
			try
			{
				auto val = std::stoull(str);
				if (val <= std::numeric_limits<uint32_t>::max())
					return static_cast<uint32_t>(val);
			}
			catch (...) {}
			return std::nullopt;
		}
		// Parse uint64_t from std::string, return nullopt on failure.
		static std::optional<uint64_t> parse_ulong(const std::string &str)
		{
			try { return std::stoull(str); }
			catch (...) { return std::nullopt; }
		}
	};

	// Specialization for wchar_t
	// Mirrors the char specialization but for std::wstring inputs.
	template<>
	struct numeric_parser<wchar_t>
	{
		// Parse int32_t from std::wstring, return nullopt on failure.
		static std::optional<int32_t> parse_int(const std::wstring &str)
		{
			try { return std::stoi(str); }
			catch (...) { return std::nullopt; }
		}
		// Parse int64_t from std::wstring, return nullopt on failure.
		static std::optional<int64_t> parse_long(const std::wstring &str)
		{
			try { return std::stoll(str); }
			catch (...) { return std::nullopt; }
		}
		// Parse uint32_t from std::wstring, ensuring the parsed value fits.
		static std::optional<uint32_t> parse_uint(const std::wstring &str)
		{
			try
			{
				auto val = std::stoull(str);
				if (val <= std::numeric_limits<uint32_t>::max())
					return static_cast<uint32_t>(val);
			}
			catch (...) {}
			return std::nullopt;
		}
		// Parse uint64_t from std::wstring, return nullopt on failure.
		static std::optional<uint64_t> parse_ulong(const std::wstring &str)
		{
			try { return std::stoull(str); }
			catch (...) { return std::nullopt; }
		}
	};

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
		// Type aliases for convenience and readability
		using string_type = std::basic_string<CharT>;
		using string_view_type = std::basic_string_view<CharT>;
		using size_type = typename string_type::size_type;
		using delimiters_type = std::unordered_set<CharT>;

		// Configuration controlling cursor/iterator behaviour.
		// - escape_support: interpret escape characters
		// - quote_support: allow quoted tokens
		// - skip_empty_tokens: whether empty tokens are omitted
		struct cursor_config
		{
			bool escape_support = true;
			bool quote_support = false;
			bool skip_empty_tokens = true;
		};

	private:
		// snapshot_t
		// Immutable-ish snapshot object shared between iterators to allow
		// thread-safe traversal over a stable view of the input and settings.
		struct snapshot_t
		{
			string_type input;
			delimiters_type delims;
			cursor_config config;
		};

		// Shared pointer to the snapshot; allows cheap copying of iterators.
		using snapshot_ptr = std::shared_ptr<snapshot_t>; // made mutable

	public:
		//============================================================
		// iterator
		//============================================================
		class iterator
		{
		public:
			// STL iterator type aliases
			using iterator_category = std::input_iterator_tag;
			using value_type = string_type;
			using reference = const string_type&;
			using pointer = const string_type*;
			using difference_type = std::ptrdiff_t;

			iterator() = default;

			// Construct iterator bound to a snapshot at position pos.
			// On construction we immediately advance to the first token.
			iterator(snapshot_ptr snapshot, size_type pos = 0)
				: m_snapshot(std::move(snapshot)), m_pos(pos)
			{
				++(*this);
			}

			// Dereference to obtain current token.
			reference operator*() const { return m_token; }
			pointer operator->() const { return &m_token; }

			// c_str() - Returns a null-terminated C-style string
			// representation of the current token.
			// The returned pointer is valid until the iterator is modified.
			const CharT* c_str()
			{
				m_cstr.clear();
				m_cstr.insert(m_cstr.end(), m_token.begin(), m_token.end());
				m_cstr.push_back(CharT(0));
				return m_cstr.data();
			}

			// Typed accessors that attempt numeric conversion of the token.
			std::optional<int32_t> as_int() const { return numeric_parser<CharT>::parse_int(m_token); }
			std::optional<int64_t> as_long() const { return numeric_parser<CharT>::parse_long(m_token); }
			std::optional<uint32_t> as_uint() const { return numeric_parser<CharT>::parse_uint(m_token); }
			std::optional<uint64_t> as_ulong() const { return numeric_parser<CharT>::parse_ulong(m_token); }

			// advance_as_cstr() - Returns a null-terminated C-style string
			// representation of the current token and advances the iterator.
			// If skip_empty_tokens is enabled and token is empty returns nullptr.
			const CharT* advance_as_cstr()
			{
				if (m_token.empty() && m_snapshot->config.skip_empty_tokens)
					return nullptr;

				const CharT* result = c_str();
				++(*this);

				return result;
			}

			// advance_as_int() - Convert current token to int and advance.
			std::optional<int32_t> advance_as_int()
			{
				auto val = as_int();
				if (!val) return std::nullopt;
				++(*this);

				return val;
			}

			// advance_as_long() - Convert current token to long and advance.
			std::optional<int64_t> advance_as_long()
			{
				auto val = as_long();
				if (!val) return std::nullopt;
				++(*this);

				return val;
			}

			// advance_as_uint() - Convert current token to unsigned int and advance.
			std::optional<uint32_t> advance_as_uint()
			{
				auto val = as_uint();
				if (!val) return std::nullopt;
				++(*this);

				return val;
			}

			// advance_as_ulong() - Convert current token to unsigned long and advance.
			std::optional<uint64_t> advance_as_ulong()
			{
				auto val = as_ulong();
				if (!val) return std::nullopt;
				++(*this);

				return val;
			}

			// advance_as_string() - Return current token as string and advance.
			// If skip_empty_tokens is enabled and token is empty returns empty string.
			string_type advance_as_string()
			{
				if (m_token.empty() && m_snapshot->config.skip_empty_tokens)
					return string_type{};

				string_type result = m_token;
				++(*this);

				return result;
			}

			// Pre-increment: move to next token.
			iterator &operator++()
			{
				extract_next();
				return *this;
			}

			// Post-increment: standard semantics.
			iterator operator++(int)
			{
				iterator temp = *this;
				++(*this);
				return temp;
			}

			// Equality compares both snapshot identity and position.
			bool operator==(const iterator &other) const
			{
				return m_pos == other.m_pos && m_snapshot == other.m_snapshot;
			}

			bool operator!=(const iterator &other) const
			{
				return !(*this == other);
			}

			// peek() - Non-destructively look at the next token without advancing.
			// Returns std::nullopt if no token is available or if the next token
			// is empty and skip_empty_tokens is enabled.
			std::optional<string_type> peek() const
			{
				if (!m_snapshot || m_pos >= m_snapshot->input.size())
					return std::nullopt;

				iterator tmp = *this;
				tmp.extract_next();

				return tmp.m_token.empty() && m_snapshot->config.skip_empty_tokens ? std::nullopt : std::make_optional(tmp.m_token);
			}

			// set_delimiters() - Replace the delimiter set used by this iterator's snapshot.
			void set_delimiters(string_view_type new_delims)
			{
				m_snapshot->delims = stringtokenizer_t::parse_delims(new_delims);
			}

		private:
			// extract_next()
			// Core token extraction logic advances m_pos and sets m_token.
			// Handles escape, quotes and delimiter skipping according to config.
			void extract_next()
			{
				m_token.clear();
				if (!m_snapshot || m_pos >= m_snapshot->input.size())
				{
					m_pos = m_snapshot ? m_snapshot->input.size() + 1 : 0;
					return;
				}

				bool in_quotes = false, escape = false;
				const auto &input = m_snapshot->input;
				const auto &delims = m_snapshot->delims;

				while (m_pos < input.size())
				{
					CharT ch = input[m_pos++];

					// Handle escape character
					if (m_snapshot->config.escape_support && ch == CharT('\\') && !escape)
					{
						escape = true;
						continue;
					}
					if (escape)
					{
						m_token += ch;
						escape = false;
						continue;
					}

					// Handle quoted strings
					if (m_snapshot->config.quote_support && ch == CharT('"'))
					{
						in_quotes = !in_quotes;
						continue;
					}

					// Delimiter found, break unless we're inside quotes
					if (!in_quotes && delims.count(ch))
						break;

					m_token += ch;
				}

				if (m_token.empty() && m_snapshot->config.skip_empty_tokens)
					extract_next();
			}

			// Member variables for iterator state
			snapshot_ptr m_snapshot;
			size_type m_pos = 0;
			string_type m_token;
			mutable std::vector<CharT> m_cstr;
		};

		//============================================================
		// reverse_iterator
		//============================================================
		class reverse_iterator
		{
		public:
			// Type aliases
			using iterator_category = std::input_iterator_tag;
			using value_type = string_type;
			using reference = const string_type&;
			using pointer = const string_type*;
			using difference_type = std::ptrdiff_t;

			reverse_iterator() = default;

			// Initialize reverse_iterator to point to the end of the input.
			reverse_iterator(snapshot_ptr snapshot, size_type pos)
				: m_snapshot(std::move(snapshot)), m_pos(pos)
			{
				--(*this);
			}

			reference operator*() const { return m_token; }
			pointer operator->() const { return &m_token; }

			const CharT* c_str()
			{
				m_cstr.clear();
				m_cstr.insert(m_cstr.end(), m_token.begin(), m_token.end());
				m_cstr.push_back(CharT(0));

				return m_cstr.data();
			}

			std::optional<int32_t> as_int() const { return numeric_parser<CharT>::parse_int(m_token); }
			std::optional<int64_t> as_long() const { return numeric_parser<CharT>::parse_long(m_token); }
			std::optional<uint32_t> as_uint() const { return numeric_parser<CharT>::parse_uint(m_token); }
			std::optional<uint64_t> as_ulong() const { return numeric_parser<CharT>::parse_ulong(m_token); }

			// retreat_as_cstr() - Returns a null-terminated C-style string
			// representation of the current token and moves the iterator backward.
			const CharT* retreat_as_cstr()
			{
				if (m_token.empty() && m_snapshot->config.skip_empty_tokens)
					return nullptr;

				const CharT* result = c_str();
				--(*this);

				return result;
			}

			// retreat_as_int() - Convert current token to int and retreat.
			std::optional<int32_t> retreat_as_int()
			{
				auto val = as_int();

				if (!val)
					return std::nullopt;

				--(*this);

				return val;
			}

			// retreat_as_long() - Convert current token to long and retreat.
			std::optional<int64_t> retreat_as_long()
			{
				auto val = as_long();
				if (!val)
					return std::nullopt;

				--(*this);

				return val;
			}

			// retreat_as_uint() - Convert current token to unsigned int and retreat.
			std::optional<uint32_t> retreat_as_uint()
			{
				auto val = as_uint();
				if (!val)
					return std::nullopt;

				--(*this);

				return val;
			}

			// retreat_as_ulong() - Convert current token to unsigned long and retreat.
			std::optional<uint64_t> retreat_as_ulong()
			{
				auto val = as_ulong();
				if (!val)
					return std::nullopt;

				--(*this);

				return val;
			}

			// retreat_as_string() - Return current token as string and retreat.
			string_type retreat_as_string()
			{
				if (m_token.empty() && m_snapshot->config.skip_empty_tokens)
					return string_type{};

				string_type result = m_token;
				--(*this);

				return result;
			}

			// Pre-decrement: move to previous token.
			reverse_iterator &operator--()
			{
				extract_prev();

				return *this;
			}

			// Post-decrement: standard semantics.
			reverse_iterator operator--(int)
			{
				reverse_iterator temp = *this;
				--(*this);

				return temp;
			}

			// Equality compares both snapshot identity and position.
			bool operator==(const reverse_iterator &other) const
			{
				return m_pos == other.m_pos && m_snapshot == other.m_snapshot;
			}

			bool operator!=(const reverse_iterator &other) const
			{
				return !(*this == other);
			}

			// peek() - Non-destructively look at the next token without retreating.
			// Returns std::nullopt if no token is available or if the next token
			// is empty and skip_empty_tokens is enabled.
			std::optional<string_type> peek() const
			{
				if (!m_snapshot || m_pos >= m_snapshot->input.size())
					return std::nullopt;

				reverse_iterator tmp = *this;
				tmp.extract_prev();

				return tmp.m_token.empty() && m_snapshot->config.skip_empty_tokens ? std::nullopt : std::make_optional(tmp.m_token);
			}

			// set_delimiters() - Replace the delimiter set used by this iterator's snapshot.
			void set_delimiters(string_view_type new_delims) {
				m_snapshot->delims = stringtokenizer_t::parse_delims(new_delims);
			}

		private:
			// extract_prev()
			// Core token extraction logic for reverse traversal.
			void extract_prev()
			{
				m_token.clear();
				if (!m_snapshot || m_pos == 0)
				{
					m_pos = 0;
					return;
				}

				const auto &input = m_snapshot->input;
				const auto &delims = m_snapshot->delims;
				bool in_quotes = false, escape = false;
				size_type start = m_pos;

				// Iterate backwards to find the boundaries of the current token.
				while (m_pos > 0)
				{
					CharT ch = input[--m_pos];
					if (m_snapshot->config.escape_support && ch == CharT('\\') && !escape)
					{
						escape = true;
						continue;
					}
					if (escape)
					{
						escape = false;
						continue;
					}
					if (m_snapshot->config.quote_support && ch == CharT('"'))
					{
						in_quotes = !in_quotes;
						continue;
					}
					if (!in_quotes && delims.count(ch))
					{
						++m_pos;
						break;
					}
				}

				m_token = input.substr(m_pos, start - m_pos);

				if (m_token.empty() && m_snapshot->config.skip_empty_tokens)
					extract_prev();
			}

			// Member variables for reverse_iterator state
			snapshot_ptr m_snapshot;
			size_type m_pos = 0;
			string_type m_token;
			mutable std::vector<CharT> m_cstr;
		};

		stringtokenizer_t() = default;

		stringtokenizer_t(string_view_type input, string_view_type delims)
		{
			set_input(input, delims);
		}

		iterator begin() const { return iterator(m_snapshot, 0); }
		iterator end() const { return iterator(m_snapshot, m_snapshot ? m_snapshot->input.size() + 1 : 0); }

		size_type size() const noexcept
		{
			if (!m_snapshot)
				return 0;

			size_type count = 0;
			for (auto it = begin(); it != end(); ++it)
				++count;

			return count;
		}

		void set_input(string_view_type input, string_view_type delims)
		{
			auto snap = std::make_shared<snapshot_t>();
			snap->input = input;
			snap->delims = parse_delims(delims);
			snap->config = m_config;
			m_snapshot = std::move(snap);
		}

		void set_config(const cursor_config &cfg) { m_config = cfg; }

		void set_delimiters(string_view_type new_delims)
		{
			if (m_snapshot)
			{
				m_snapshot->delims = parse_delims(new_delims);
			}
		}

		static stringtokenizer_t from_multisz(const CharT* input, size_t max_len = SIZE_MAX)
		{
			if (!input || *input == CharT('\0'))
				return stringtokenizer_t();

			const CharT* ptr = input;
			size_t count = 0;

			while ((count + 1) < max_len && (*ptr || *(ptr + 1)))
			{
				++ptr;
				++count;
			}

			size_t total_len = std::min(static_cast<size_t>(ptr - input + 2), max_len);

			stringtokenizer_t tok;
			auto snap = std::make_shared<snapshot_t>();
			snap->input.assign(input, total_len);
			snap->delims = { CharT('\0') };
			snap->config = tok.m_config;
			tok.m_snapshot = std::move(snap);
			return tok;
		}

		static stringtokenizer_t from_multisz(const string_view_type input)
		{
			return from_multisz(input.data());
		}

		std::vector<std::basic_string<CharT>> to_vector() const
		{
			std::vector<std::basic_string<CharT>> tokens;
			for (auto it = begin(); it != end(); ++it)
			{
				tokens.emplace_back(*it);
			}
			return tokens;
		}


	private:
		static delimiters_type parse_delims(string_view_type d)
		{
			delimiters_type s;
			size_t i = 0;
			while (i < d.size()) {
				if (d[i] == CharT('\\') && i + 1 < d.size())
				{
					switch (d[i + 1])
					{
					case CharT('n') : s.insert(CharT('\n')); i += 2; break;
					case CharT('r'): s.insert(CharT('\r')); i += 2; break;
					case CharT('t') : s.insert(CharT('\t')); i += 2; break;
					case CharT('\\'): s.insert(CharT('\\')); i += 2; break;
					case CharT('"') : s.insert(CharT('"')); i += 2; break;
					case CharT('0') : s.insert(CharT('\0'));  i += 2; break;
					default: s.insert(d[i]); ++i; break;
					}
				}
				else
				{
					s.insert(d[i]);
					++i;
				}
			}
			return s;
		}

		snapshot_ptr m_snapshot;
		cursor_config m_config;
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

} // namespace mameui::util::string_util

#endif // MAMEUI_LIB_UTIL_MUI_STRINGTOKENIZER_H
