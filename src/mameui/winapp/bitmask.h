// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_BITMASK_H
#define MAMEUI_WINAPP_BITMASK_H

#pragma once

/* Bit array type */
using Bits = struct bit_array
{
	unsigned int m_nSize;
	std::vector<unsigned char> m_lpBits;
};
using LPBITS = std::shared_ptr<Bits>;

/* Bit functions */
LPBITS NewBits(unsigned int nLength /* in bits */);
void DeleteBits(LPBITS lpBits);
bool TestBit(LPBITS lpBits, unsigned int nBit);
void SetBit(LPBITS lpBits, unsigned int nBit);
void ClearBit(LPBITS lpBits, unsigned int nBit);
void SetAllBits(LPBITS lpBits, bool bSet);
int FindBit(LPBITS lpBits, int nStartPos, bool bSet);

/* Linked list type */
using Node = struct linked_list_node;
struct linked_list_node
{
	void *data;
	std::shared_ptr<Node> next;
	std::weak_ptr<Node> prev;
};
using LPNODE = std::shared_ptr<Node>;

/* Linked list functions */

#endif // MAMEUI_WINAPP_BITMASK_H
