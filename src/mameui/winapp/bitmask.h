// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_BITMASK_H
#define MAMEUI_WINAPP_BITMASK_H

#pragma once

// Bit array type
using Bits = struct bit_array
{
	unsigned int m_nSize;
	std::vector<unsigned char> m_lpBits;
};
using LPBITS = std::shared_ptr<Bits>;


// Linked list type
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

// Bit functions
LPBITS NewBits(unsigned int nLength /* in bits */);
void DeleteBits(LPBITS lpBits);
bool TestBit(LPBITS lpBits, unsigned int nBit);
void SetBit(LPBITS lpBits, unsigned int nBit);
void ClearBit(LPBITS lpBits, unsigned int nBit);
void SetAllBits(LPBITS lpBits, bool bSet);
int FindBit(LPBITS lpBits, int nStartPos, bool bSet);

// Linked list functions

#endif // MAMEUI_WINAPP_BITMASK_H
