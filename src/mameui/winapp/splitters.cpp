// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

 /***************************************************************************

  splitters.cpp

  Splitter GUI code. - Tree, splitter, list, splitter, pict

  Created 12/03/98 (C) by Mike Haaland (mhaaland@hypertech.com)

***************************************************************************/


// standard windows headers

// standard C++ headers

// MAME/MAMEUI headers
//#include "winapi_controls.h"
//#include "winapi_dialog_boxes.h"
#include "winapi_gdi.h"
//#include "winapi_input.h"
#include "winapi_menus.h"
//#include "winapi_shell.h"
//#include "winapi_storage.h"
#include "winapi_system_services.h"
#include "winapi_windows.h"

using namespace mameui::winapi;
//using namespace mameui::winapi::controls;

#include "splitters.h"
#include "mui_opts.h"

/* Local Variables */
static BOOL         bTracking = 0;
static int          numSplitters = 0;
static int          currentSplitter = 0;
static HZSPLITTER   *splitter;
static LPHZSPLITTER lpCurSpltr = 0;
static HCURSOR      hSplitterCursor = 0;

int *nSplitterOffset;

BOOL InitSplitters(void)
{
	/* load the cursor for the splitter */
	hSplitterCursor = menus::load_cursor(system_services::get_module_handle(0), menus::make_int_resource(IDC_CURSOR_HSPLIT));

	int nSplitterCount = GetSplitterCount();

	splitter = new HZSPLITTER[nSplitterCount];
	if (!splitter)
		goto error;

	nSplitterOffset = new int[nSplitterCount];
	if (!nSplitterOffset)
		goto error;

	return true;

error:
	SplittersExit();
	return false;
}

void SplittersExit(void)
{
	if (splitter)
		delete[] splitter;

	if (nSplitterOffset)
		delete[] nSplitterOffset;
}


/* Called with hWnd = Parent Window */
static void CalcSplitter(HWND hWnd, LPHZSPLITTER lpSplitter)
{
	POINT p = {0,0};
	(void)gdi::client_to_screen(hWnd, &p);

	(void)windows::get_window_rect(lpSplitter->m_hWnd, &lpSplitter->m_dragRect);
	RECT leftRect, rightRect;
	(void)windows::get_window_rect(lpSplitter->m_hWndLeft, &leftRect);
	(void)windows::get_window_rect(lpSplitter->m_hWndRight, &rightRect);

	gdi::offset_rect(&lpSplitter->m_dragRect, -p.x, -p.y);
	gdi::offset_rect(&leftRect, -p.x, -p.y);
	gdi::offset_rect(&rightRect, -p.x, -p.y);

	int dragWidth = lpSplitter->m_dragRect.right - lpSplitter->m_dragRect.left;

	lpSplitter->m_limitRect.left = leftRect.left + 20;
	lpSplitter->m_limitRect.right = rightRect.right - 20;
	lpSplitter->m_limitRect.top = lpSplitter->m_dragRect.top;
	lpSplitter->m_limitRect.bottom = lpSplitter->m_dragRect.bottom;
	if (lpSplitter->m_func)
		lpSplitter->m_func(hWnd, &lpSplitter->m_limitRect);

	if (lpSplitter->m_dragRect.left < lpSplitter->m_limitRect.left)
		lpSplitter->m_dragRect.left = lpSplitter->m_limitRect.left;

	if (lpSplitter->m_dragRect.right > lpSplitter->m_limitRect.right)
		lpSplitter->m_dragRect.left = lpSplitter->m_limitRect.right - dragWidth;

	lpSplitter->m_dragRect.right = lpSplitter->m_dragRect.left + dragWidth;
}

void AdjustSplitter2Rect(HWND hWnd, LPRECT lpRect)
{
	RECT pRect;

	(void)windows::get_client_rect(hWnd, &pRect);

	if (lpRect->right > pRect.right)
		lpRect->right = pRect.right;

	lpRect->right = MIN(lpRect->right - GetMinimumScreenShotWindowWidth(), lpRect->right);

	lpRect->left = MAX((pRect.right - (pRect.right - pRect.left)) / 2, lpRect->left);
}

void AdjustSplitter1Rect(HWND hWnd, LPRECT lpRect)
{
}

void RecalcSplitters(void)
{
	int i;

	for (i = 0; i < numSplitters; i++)
	{
		CalcSplitter(GetParent(splitter[i].m_hWnd), &splitter[i]);
		nSplitterOffset[i] = splitter[i].m_dragRect.left;
	}

	for (i = numSplitters - 1; i >= 0; i--)
	{
		CalcSplitter(GetParent(splitter[i].m_hWnd), &splitter[i]);
		nSplitterOffset[i] = splitter[i].m_dragRect.left;
	}

}

void OnSizeSplitter(HWND hWnd)
{
	static bool firstTime = true;
	bool changed = false;
	RECT rWindowRect;
	POINT p = {0,0};
	int i;
	int nSplitterCount = GetSplitterCount();

	if (firstTime)
	{
		for (i = 0; i < nSplitterCount; i++)
			nSplitterOffset[i] = GetSplitterPos(i);
		changed = true;
		firstTime = false;
	}

	(void)windows::get_window_rect(hWnd, &rWindowRect);

	for (i = 0; i < nSplitterCount; i++)
	{
		p.x = 0;
		p.y = 0;
		(void)gdi::client_to_screen(splitter[i].m_hWnd, &p);

		/* We must change if our window is not in the window rect */
		bool bMustChange = !gdi::pt_in_rect(&rWindowRect, p);

		/* We should also change if we are ahead the next splitter */
		if ((i < nSplitterCount-1) && (nSplitterOffset[i] >= nSplitterOffset[i+1]))
			bMustChange = true;

		/* ...or if we are behind the previous splitter */
		if ((i > 0) && (nSplitterOffset[i] <= nSplitterOffset[i-1]))
			bMustChange = true;
#ifdef MESS
		if ((i==1)&&(!BIT(GetWindowPanes(), 2))) // sw
			bMustChange = false;
		if ((i==2)&&(!BIT(GetWindowPanes(), 3))) // images
			bMustChange = false;
#endif
		if (bMustChange)
		{
			nSplitterOffset[i] = (rWindowRect.right - rWindowRect.left) * g_splitterInfo[i].dPosition;
			changed = true;
		}
	}

	if (changed)
	{
		ResizePickerControls(hWnd);
		RecalcSplitters();
		//UpdateScreenShot();
	}
}

void AddSplitter(HWND hWnd, HWND hWndLeft, HWND hWndRight, void (*func)(HWND hWnd,LPRECT lpRect))
{
	LPHZSPLITTER lpSpltr = &splitter[numSplitters];

	if (numSplitters >= GetSplitterCount())
		return;

	lpSpltr->m_hWnd = hWnd;
	lpSpltr->m_hWndLeft = hWndLeft;
	lpSpltr->m_hWndRight = hWndRight;
	lpSpltr->m_func = func;
	CalcSplitter(GetParent(hWnd), lpSpltr);

	numSplitters++;
}

static void OnInvertTracker(HWND hWnd, const RECT *rect)
{
	HDC hDC = GetDC(hWnd);
	HBRUSH hBrush = gdi::create_solid_brush(RGB(0xFF, 0xFF, 0xFF));
	HBRUSH hOldBrush = 0;

	if (hBrush != 0)
		hOldBrush = (HBRUSH)gdi::select_object(hDC, hBrush);
	PatBlt(hDC, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, DSTINVERT);
	if (hOldBrush != 0)
		(void)gdi::select_object(hDC, hOldBrush);
	(void)ReleaseDC(hWnd, hDC);
	(void)gdi::delete_brush(hBrush);
}

static void StartTracking(HWND hWnd, UINT hitArea)
{
	if (!bTracking && lpCurSpltr != 0 && hitArea == SPLITTER_HITITEM)
	{
		// Ensure we have an updated cursor structure
		CalcSplitter(hWnd, lpCurSpltr);
		// Draw the first splitter shadow
		OnInvertTracker(hWnd, &lpCurSpltr->m_dragRect);
		// Capture the mouse
		SetCapture(hWnd);
		// Set tracking to TRUE
		bTracking = true;
		SetCursor(hSplitterCursor);
	}
}

static void StopTracking(HWND hWnd)
{
	if (bTracking)
	{
		// erase the tracking image
		OnInvertTracker(hWnd, &lpCurSpltr->m_dragRect);
		// Release the mouse
		ReleaseCapture();
		// set tracking to false
		bTracking = false;
		SetCursor(menus::load_cursor(0, IDC_ARROW));
		// set the new splitter position
		nSplitterOffset[currentSplitter] = lpCurSpltr->m_dragRect.left;
		// Redraw the screen area
		ResizePickerControls(hWnd);
		UpdateScreenShot();
		gdi::invalidate_rect(GetMainWindow(), NULL, true);
	}
}

static UINT SplitterHitTest(HWND hWnd, POINTS p)
{
	POINT pt;
	pt.x = p.x;
	pt.y = p.y;

	// Check which area we hit
	(void)gdi::client_to_screen(hWnd, &pt);

	RECT  rect;
	for (size_t i = 0; i < numSplitters; i++)
	{
		(void)windows::get_window_rect(splitter[i].m_hWnd, &rect);
		if (gdi::pt_in_rect(&rect, pt))
		{
			lpCurSpltr = &splitter[i];
			currentSplitter = i;
			// We hit the splitter
			return SPLITTER_HITITEM;
		}
	}
	lpCurSpltr = 0;
	// We missed the splitter
	return SPLITTER_HITNOTHING;
}

void OnMouseMove(HWND hWnd, UINT nFlags, POINTS p)
{
	if (bTracking) // move the tracking image
	{
		POINT pt;
		pt.x = (int)p.x;
		pt.y = (int)p.y;

		(void)gdi::client_to_screen(hWnd, &pt);
		RECT rect;
		(void)windows::get_window_rect(hWnd, &rect);
		if (! gdi::pt_in_rect(&rect, pt))
		{
			if ((short)pt.x < (short)rect.left)
				pt.x = rect.left;
			if ((short)pt.x > (short)rect.right)
				pt.x = rect.right;
			pt.y = rect.top + 1;
		}

		(void)gdi::screen_to_client(hWnd, &pt);

		// Erase the old tracking image
		OnInvertTracker(hWnd, &lpCurSpltr->m_dragRect);

		// calc the new one based on p.x draw it
		int nWidth = lpCurSpltr->m_dragRect.right - lpCurSpltr->m_dragRect.left;
		lpCurSpltr->m_dragRect.right = pt.x + nWidth / 2;
		lpCurSpltr->m_dragRect.left  = pt.x - nWidth / 2;

		if (pt.x - nWidth / 2 > lpCurSpltr->m_limitRect.right)
		{
			lpCurSpltr->m_dragRect.right = lpCurSpltr->m_limitRect.right;
			lpCurSpltr->m_dragRect.left  = lpCurSpltr->m_dragRect.right - nWidth;
		}
		if (pt.x + nWidth / 2 < lpCurSpltr->m_limitRect.left)
		{
			lpCurSpltr->m_dragRect.left  = lpCurSpltr->m_limitRect.left;
			lpCurSpltr->m_dragRect.right = lpCurSpltr->m_dragRect.left + nWidth;
		}
		OnInvertTracker(hWnd, &lpCurSpltr->m_dragRect);
	}
	else
	{
		switch(SplitterHitTest(hWnd, p))
		{
		case SPLITTER_HITNOTHING:
			SetCursor(menus::load_cursor(0, IDC_ARROW));
			break;
		case SPLITTER_HITITEM:
			SetCursor(hSplitterCursor);
			break;
		}
	}
}

void OnLButtonDown(HWND hWnd, UINT nFlags, POINTS p)
{
	if (!bTracking) // See if we need to start a splitter drag
	{
		StartTracking(hWnd, SplitterHitTest(hWnd, p));
	}
}

void OnLButtonUp(HWND hWnd, UINT nFlags, POINTS p)
{
	if (bTracking)
	{
		StopTracking(hWnd);
	}
}

int GetSplitterCount(void)
{
	int nSplitterCount = 0;
	while(g_splitterInfo[nSplitterCount].dPosition > 0)
		nSplitterCount++;
	return nSplitterCount;
}

/* End of file */
