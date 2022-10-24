// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_STRINGTOKENIZER_H
#define MAMEUI_LIB_UTIL_MUI_STRINGTOKENIZER_H

#pragma once

template <typename CharT>
class mui_stringtokenizer_t
{
public:
	using string_type = std::basic_string<CharT>;
	using string_view_type = std::basic_string_view<CharT>;
	using size_type = typename string_type::size_type;

	// --- Forward Iterator ---
	class iterator
	{
	public:
		using value_type = string_view_type;
		using reference = const string_view_type&;
		using pointer = const string_view_type*;
		using iterator_category = std::input_iterator_tag;
		using difference_type = std::ptrdiff_t;

		iterator(mui_stringtokenizer_t* tokenizer, bool is_end = false)
			: m_tokenizer(tokenizer), m_is_end(is_end)
		{
			if (!is_end && m_tokenizer && m_tokenizer->has_next())
				m_current_token = m_tokenizer->next_token();
			else
				m_is_end = true;
		}

		reference operator*() const { return m_current_token; }
		pointer operator->() const { return &m_current_token; }

		iterator& operator++()
		{
			if (m_tokenizer && m_tokenizer->has_next())
				m_current_token = m_tokenizer->next_token();
			else
				m_is_end = true;
			return *this;
		}

		iterator operator++(int)
		{
			iterator temp = *this;
			++(*this);
			return temp;
		}

		bool operator==(const iterator& other) const
		{
			return m_is_end == other.m_is_end && m_tokenizer == other.m_tokenizer;
		}

		bool operator!=(const iterator& other) const
		{
			return !(*this == other);
		}

	private:
		mui_stringtokenizer_t* m_tokenizer;
		string_view_type m_current_token;
		bool m_is_end;
	};

	// --- Reverse Iterator ---
	class reverse_iterator
	{
	public:
		using value_type = string_view_type;
		using reference = const string_view_type&;
		using pointer = const string_view_type*;
		using iterator_category = std::input_iterator_tag;
		using difference_type = std::ptrdiff_t;

		reverse_iterator(mui_stringtokenizer_t tokenizer, bool is_end = false)
			: m_tokenizer(std::move(tokenizer)), m_is_end(is_end)
		{
			if (!is_end && m_tokenizer.has_previous())
				m_current_token = m_tokenizer.previous_token();
			else
				m_is_end = true;
		}

		reference operator*() const { return m_current_token; }
		pointer operator->() const { return &m_current_token; }

		reverse_iterator& operator++()
		{
			if (m_tokenizer.has_previous())
				m_current_token = m_tokenizer.previous_token();
			else
				m_is_end = true;
			return *this;
		}

		reverse_iterator operator++(int)
		{
			reverse_iterator temp = *this;
			++(*this);
			return temp;
		}

		bool operator==(const reverse_iterator& other) const
		{
			return m_is_end == other.m_is_end;
		}

		bool operator!=(const reverse_iterator& other) const
		{
			return !(*this == other);
		}

	private:
		mui_stringtokenizer_t m_tokenizer;
		string_view_type m_current_token;
		bool m_is_end;
	};

	// --- Constructor ---
	mui_stringtokenizer_t(string_view_type input = {}, string_view_type delimiters = {})
	{
		if (!input.empty())
			str(input);

		if (!delimiters.empty())
			this->set_delimiters(delimiters);
	}

	// --- Iterators ---
	iterator begin() { reset_position(); return iterator(this); }
	iterator end() { return iterator(this, true); }

	reverse_iterator rbegin()
	{
		auto copy = *this;
		copy.m_delimiter_pos = m_input_string.size();
		return reverse_iterator(copy);
	}

	reverse_iterator rend()
	{
		auto copy = *this;
		return reverse_iterator(copy, true);
	}

	iterator next() { return iterator(this); }

	reverse_iterator prev()
	{
		auto copy = *this;
		copy.m_delimiter_pos = m_input_string.size();
		return reverse_iterator(copy);
	}

	// --- Tokenization ---
	string_view_type next_token()
	{
		if (m_delimiter_pos >= m_input_string.size()) return {};

		while (m_delimiter_pos < m_input_string.size() && is_delimiter(m_input_string[m_delimiter_pos]))
			++m_delimiter_pos;

		if (m_delimiter_pos >= m_input_string.size()) return {};

		size_type token_start = m_delimiter_pos;

		while (m_delimiter_pos < m_input_string.size() && !is_delimiter(m_input_string[m_delimiter_pos]))
			++m_delimiter_pos;

		return this->m_input_string.substr(token_start, m_delimiter_pos - token_start);
	}

	const CharT* next_token_cstr()
	{
		const string_view_type &view = next_token();
		if (view.empty())
			return nullptr;

		m_token.assign(view.begin(), view.end());

		return m_token.c_str();
	}

	string_view_type previous_token()
	{
		if (m_delimiter_pos == 0 || m_input_string.empty()) return {};

		size_type pos = (m_delimiter_pos > 0 && m_delimiter_pos <= m_input_string.size())
			? m_delimiter_pos - 1 : m_input_string.size() - 1;

		while (pos > 0 && is_delimiter(m_input_string[pos]))
			--pos;

		if (pos == 0 && is_delimiter(m_input_string[pos]))
			return {};

		size_type token_end = pos;

		while (pos > 0 && !is_delimiter(m_input_string[pos - 1]))
			--pos;

		m_delimiter_pos = pos;
		return this->m_input_string.substr(pos, token_end - pos + 1);
	}

	const CharT* prev_token_cstr()
	{
		const string_view_type& view = previous_token();
		if (view.empty())
			return nullptr;

		m_token.assign(view.begin(), view.end());

		return m_token.c_str();
	}

	// --- State ---
	bool has_next() const
	{
		size_type temp = m_delimiter_pos;
		while (temp < m_input_string.size() && is_delimiter(m_input_string[temp]))
			++temp;
		return temp < m_input_string.size();
	}

	bool has_previous() const
	{
		return m_delimiter_pos > 0;
	}

	void reset_position()
	{
		m_delimiter_pos = 0;
		m_token.clear();
	}

	void set_input(string_view_type new_input, string_view_type new_delimiters = {})
	{
		if (new_input.empty())
			return;

		this->str(new_input);

		if (new_delimiters.empty())
			return;

		this->set_delimiters(new_delimiters);
	}

	void str(string_view_type new_input)
	{
		m_input_string = new_input;
		m_token.clear();
		m_delimiter_pos = 0;
	}

	string_view_type str() const
	{
		return m_input_string;
	}

	void set_delimiters(string_view_type new_delimiters)
	{
		m_delimiters.clear();
		for (CharT ch : new_delimiters)
			m_delimiters.insert(ch);
	}

private:
	bool is_delimiter(CharT ch) const
	{
		return m_delimiters.count(ch) > 0;
	}

	string_view_type m_input_string;
	string_type m_token;
	std::unordered_set<CharT> m_delimiters;
	size_type m_delimiter_pos;
};

// --- Type Definitions ---

// mui_stringtokenizer
using mui_stringtokenizer = mui_stringtokenizer_t<char>;
extern template class mui_stringtokenizer_t<char>;

// mui_wstringtokenizer
using mui_wstringtokenizer = mui_stringtokenizer_t<wchar_t>;
extern template class mui_stringtokenizer_t<wchar_t>;

#if 0 // remove this if you happen to use TCHARs in your code
// mui_tstringtokenizer
#if defined(UNICODE) || defined(_UNICODE)
using mui_tstringtokenizer = mui_stringtokenizer_t<wchar_t>;
#else
using mui_tstringtokenizer = mui_stringtokenizer_t<char>;
#endif // UNICODE
#endif

#endif // MAMEUI_LIB_UTIL_MUI_STRINGTOKENIZER_H
