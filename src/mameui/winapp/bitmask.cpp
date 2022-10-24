// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************
// bitmask.cpp - Bitmask support routines - MSH 11/19/1998

// standard C++ headers
#include <memory>
#include <vector>

// standard windows headers

// MAME headers

// MAMEUI headers
#include "bitmask.h"

// --------------------------------------------------------------------------
// type declarations
// --------------------------------------------------------------------------

// Bit routines
static unsigned char maskTable[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

// --------------------------------------------------------------------------
// function definitions
// --------------------------------------------------------------------------

// Declares and initializes a new LPBITS struct which is
// then returned with all bits cleared. Note that nLength
// is the number of desired bits and if set to 0 will
// return an empty pointer indicating a failed condition.
// Parameters:
//  - nLength: The number of desired bits.
// Returns:
//  - A pointer to the new LPBITS structure, or an empty
//    pointer if nLength is 0.
// --------------------------------------------------------
LPBITS NewBits(unsigned int nLength)
{
	if (nLength == 0)
		return nullptr;

	unsigned int nSize = (nLength + 7) / 8;

	LPBITS lpBits = std::make_shared<Bits>();
	if (!lpBits)
		return nullptr;

	try { lpBits->m_lpBits.resize(nSize); }
	catch (const std::bad_alloc&) { return nullptr; }

	lpBits->m_nSize = nSize;

	return lpBits;
}

// Delete an LPBITS struct and free its memory.
// Parameters:
//  - lpBits: Pointer to the LPBITS structure to be
//            deleted.
// -------------------------------------------------------
void DeleteBits(LPBITS lpBits)
{
	if (lpBits)
		lpBits.reset();
}

// Test if the specified bit is set in the bit array.
// Parameters:
//  - lpBits: Pointer to the BITS structure containing
//            the bit array.
//  - nBit: The zero-based index of the bit to be tested.
// Returns:
//  - true if the specified bit is set; false otherwise.
// -------------------------------------------------------
bool TestBit(LPBITS lpBits, unsigned int nBit)
{
	if (!lpBits || lpBits->m_nSize == 0 || lpBits->m_lpBits.empty())
		return false;

	unsigned int offset = nBit >> 3;

	if (offset >= lpBits->m_nSize)
		return false;

	unsigned char mask = maskTable[nBit & 7];
	return (lpBits->m_lpBits[offset] & mask) != 0;
}


// Set the specified bit in the bit array.
// Parameters:
//  - lpBits: Pointer to the BITS structure containing
//            the bit array.
//  - nBit: The zero-based index of the bit to be set.
// -------------------------------------------------------
void SetBit(LPBITS lpBits, unsigned int nBit)
{
	if (!lpBits || lpBits->m_nSize == 0 || lpBits->m_lpBits.empty())
		return;

	unsigned int offset = nBit >> 3;

	if (offset >= lpBits->m_nSize)
		return;

	unsigned char mask = maskTable[nBit & 7];
	lpBits->m_lpBits[offset] |= mask;
}

// Clear the specified bit in the bit array.
// Parameters:
//  - lpBits: Pointer to the BITS structure containing
//            the bit array.
//  - nBit: The zero-based index of the bit to be
//          cleared.
// -------------------------------------------------------
void ClearBit(LPBITS lpBits, unsigned int nBit)
{
	if (!lpBits || lpBits->m_nSize == 0 || lpBits->m_lpBits.empty())
		return;

	unsigned int offset = nBit >> 3;

	if (offset >= lpBits->m_nSize)
		return;

	unsigned char mask = maskTable[nBit & 7];
	lpBits->m_lpBits[offset] &= ~mask;
}

// Depending on the value of 'bSet', all bits stored in
// 'lpBits' are cleared or set.
// Parameters:
//  - lpBits: Pointer to the BITS structure containing
//            the bit array.
//  - bSet: Boolean value indicating whether to set or
//          clear all bits.
// -------------------------------------------------------
void SetAllBits(LPBITS lpBits, bool bSet)
{
	if (!lpBits || lpBits->m_nSize == 0 || lpBits->m_lpBits.empty())
		return;

	std::fill(lpBits->m_lpBits.begin(), lpBits->m_lpBits.end(), bSet ? '\xFF' : '\0');
}

// Find the next bit that matches 'bSet' starting after
// 'nStartPos'.
// Parameters:
//  - lpBits: Pointer to the BITS structure containing
//            the bit array.
//  - nStartPos: The zero-based index to start the search
//               after.
//  - bSet: Boolean value indicating whether to search for a
//          set or unset bit.
// Returns:
//  - The zero-based index of the next matching bit, or
//    -1 if no matching bit is found.
// -------------------------------------------------------
int FindBit(LPBITS lpBits, int nStartPos, bool bSet)
{
	if (!lpBits || lpBits->m_nSize == 0 || lpBits->m_lpBits.empty())
		return -1;

	unsigned int end = lpBits->m_nSize * 8;

	if (nStartPos < 0)
		nStartPos = 0;

	for (unsigned int i = nStartPos; i < end; ++i)
	{
		bool res = TestBit(lpBits, i);
		if ((res && bSet) || (!res && !bSet))
			return i;
	}

	return -1;
}
