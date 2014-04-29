/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_vcl.hxx"

#include <string.h>
#include <stdlib.h>

#include <tools/svwin.h>
#include <tools/debug.hxx>

#include <win/wincomp.hxx>
#include <win/salbmp.h>
#include <win/saldata.hxx>
#include <win/salids.hrc>
#include <win/salgdi.h>
#include <win/salframe.h>

bool WinSalGraphics::supportsOperation( OutDevSupportType eType ) const
{
	static bool bAllowForTest(true);
    bool bRet = false;

    switch( eType )
    {
    case OutDevSupport_TransparentRect:
        bRet = mbVirDev || mbWindow;
        break;
    case OutDevSupport_B2DClip:
        bRet = true;
        break;
    case OutDevSupport_B2DDraw:
		bRet = bAllowForTest;
    default: break;
    }
    return bRet;
}

// =======================================================================

void WinSalGraphics::copyBits( const SalTwoRect& rPosAry, SalGraphics* pSrcGraphics )
{
	HDC 	hSrcDC;
	DWORD	nRop;

	if ( pSrcGraphics )
		hSrcDC = static_cast<WinSalGraphics*>(pSrcGraphics)->getHDC();
	else
		hSrcDC = getHDC();

	if ( mbXORMode )
		nRop = SRCINVERT;
	else
		nRop = SRCCOPY;

	if ( (rPosAry.mnSrcWidth  == rPosAry.mnDestWidth) &&
		 (rPosAry.mnSrcHeight == rPosAry.mnDestHeight) )
	{
		BitBlt( getHDC(),
				(int)rPosAry.mnDestX, (int)rPosAry.mnDestY,
				(int)rPosAry.mnDestWidth, (int)rPosAry.mnDestHeight,
				hSrcDC,
				(int)rPosAry.mnSrcX, (int)rPosAry.mnSrcY,
				nRop );
	}
	else
	{
		int nOldStretchMode = SetStretchBltMode( getHDC(), STRETCH_DELETESCANS );
		StretchBlt( getHDC(),
					(int)rPosAry.mnDestX, (int)rPosAry.mnDestY,
					(int)rPosAry.mnDestWidth, (int)rPosAry.mnDestHeight,
					hSrcDC,
					(int)rPosAry.mnSrcX, (int)rPosAry.mnSrcY,
					(int)rPosAry.mnSrcWidth, (int)rPosAry.mnSrcHeight,
					nRop );
		SetStretchBltMode( getHDC(), nOldStretchMode );
	}
}

// -----------------------------------------------------------------------

void ImplCalcOutSideRgn( const RECT& rSrcRect,
						 int nLeft, int nTop, int nRight, int nBottom,
						 HRGN& rhInvalidateRgn )
{
	HRGN hTempRgn;

	// Bereiche ausserhalb des sichtbaren Bereiches berechnen
	if ( rSrcRect.left < nLeft )
	{
		if ( !rhInvalidateRgn )
			rhInvalidateRgn = CreateRectRgnIndirect( &rSrcRect );
		hTempRgn = CreateRectRgn( -31999, 0, nLeft, 31999 );
		CombineRgn( rhInvalidateRgn, rhInvalidateRgn, hTempRgn, RGN_DIFF );
		DeleteRegion( hTempRgn );
	}
	if ( rSrcRect.top < nTop )
	{
		if ( !rhInvalidateRgn )
			rhInvalidateRgn = CreateRectRgnIndirect( &rSrcRect );
		hTempRgn = CreateRectRgn( 0, -31999, 31999, nTop );
		CombineRgn( rhInvalidateRgn, rhInvalidateRgn, hTempRgn, RGN_DIFF );
		DeleteRegion( hTempRgn );
	}
	if ( rSrcRect.right > nRight )
	{
		if ( !rhInvalidateRgn )
			rhInvalidateRgn = CreateRectRgnIndirect( &rSrcRect );
		hTempRgn = CreateRectRgn( nRight, 0, 31999, 31999 );
		CombineRgn( rhInvalidateRgn, rhInvalidateRgn, hTempRgn, RGN_DIFF );
		DeleteRegion( hTempRgn );
	}
	if ( rSrcRect.bottom > nBottom )
	{
		if ( !rhInvalidateRgn )
			rhInvalidateRgn = CreateRectRgnIndirect( &rSrcRect );
		hTempRgn = CreateRectRgn( 0, nBottom, 31999, 31999 );
		CombineRgn( rhInvalidateRgn, rhInvalidateRgn, hTempRgn, RGN_DIFF );
		DeleteRegion( hTempRgn );
	}
}

// -----------------------------------------------------------------------

void WinSalGraphics::copyArea( long nDestX, long nDestY,
							long nSrcX, long nSrcY,
							long nSrcWidth, long nSrcHeight,
							sal_uInt16 nFlags )
{
    bool    bRestoreClipRgn = false;
    HRGN    hOldClipRgn = 0;
    int     nOldClipRgnType = ERROR;
    HRGN    hInvalidateRgn = 0;

	// Muessen die ueberlappenden Bereiche auch invalidiert werden?
	if ( (nFlags & SAL_COPYAREA_WINDOWINVALIDATE) && mbWindow )
	{
        // compute and invalidate those parts that were either off-screen or covered by other windows
        //  while performing the above BitBlt
        // those regions then have to be invalidated as they contain useless/wrong data
		RECT	aSrcRect;
		RECT	aClipRect;
		RECT	aTempRect;
		RECT	aTempRect2;
		HRGN	hTempRgn;
		HWND	hWnd;
		int 	nRgnType;

        // restrict srcRect to this window (calc intersection)
		aSrcRect.left	= (int)nSrcX;
		aSrcRect.top	= (int)nSrcY;
		aSrcRect.right	= aSrcRect.left+(int)nSrcWidth;
		aSrcRect.bottom = aSrcRect.top+(int)nSrcHeight;
		GetClientRect( mhWnd, &aClipRect );
		if ( IntersectRect( &aSrcRect, &aSrcRect, &aClipRect ) )
		{
			// transform srcRect to screen coordinates
			POINT aPt;
			aPt.x = 0;
			aPt.y = 0;
			ClientToScreen( mhWnd, &aPt );
			aSrcRect.left	+= aPt.x;
			aSrcRect.top	+= aPt.y;
			aSrcRect.right	+= aPt.x;
			aSrcRect.bottom += aPt.y;
			hInvalidateRgn = 0;

            // compute the parts that are off screen (ie invisible)
            RECT theScreen;
            ImplSalGetWorkArea( NULL, &theScreen, NULL );  // find the screen area taking multiple monitors into account
			ImplCalcOutSideRgn( aSrcRect, theScreen.left, theScreen.top, theScreen.right, theScreen.bottom, hInvalidateRgn );

			// Bereiche die von anderen Fenstern ueberlagert werden berechnen
			HRGN hTempRgn2 = 0;
			HWND hWndTopWindow = mhWnd;
			// Find the TopLevel Window, because only Windows which are in
			// in the foreground of our TopLevel window must be considered
			if ( GetWindowStyle( hWndTopWindow ) & WS_CHILD )
			{
				RECT aTempRect3 = aSrcRect;
				do
				{
					hWndTopWindow = ::GetParent( hWndTopWindow );

					// Test, if the Parent clips our window
					GetClientRect( hWndTopWindow, &aTempRect );
					POINT aPt2;
					aPt2.x = 0;
					aPt2.y = 0;
					ClientToScreen( hWndTopWindow, &aPt2 );
					aTempRect.left	 += aPt2.x;
					aTempRect.top	 += aPt2.y;
					aTempRect.right  += aPt2.x;
					aTempRect.bottom += aPt2.y;
					IntersectRect( &aTempRect3, &aTempRect3, &aTempRect );
				}
				while ( GetWindowStyle( hWndTopWindow ) & WS_CHILD );

				// If one or more Parents clip our window, than we must
				// calculate the outside area
				if ( !EqualRect( &aSrcRect, &aTempRect3 ) )
				{
					ImplCalcOutSideRgn( aSrcRect,
										aTempRect3.left, aTempRect3.top,
										aTempRect3.right, aTempRect3.bottom,
										hInvalidateRgn );
				}
			}
            // retrieve the top-most (z-order) child window
			hWnd = GetWindow( GetDesktopWindow(), GW_CHILD );
			while ( hWnd )
			{
				if ( hWnd == hWndTopWindow )
					break;
				if ( IsWindowVisible( hWnd ) && !IsIconic( hWnd ) )
				{
					GetWindowRect( hWnd, &aTempRect );
					if ( IntersectRect( &aTempRect2, &aSrcRect, &aTempRect ) )
					{
                        // hWnd covers part or all of aSrcRect
						if ( !hInvalidateRgn )
							hInvalidateRgn = CreateRectRgnIndirect( &aSrcRect );

                        // get full bounding box of hWnd
						hTempRgn = CreateRectRgnIndirect( &aTempRect );

                        // get region of hWnd (the window may be shaped)
						if ( !hTempRgn2 )
							hTempRgn2 = CreateRectRgn( 0, 0, 0, 0 );
						nRgnType = GetWindowRgn( hWnd, hTempRgn2 );
						if ( (nRgnType != ERROR) && (nRgnType != NULLREGION) )
						{
                            // convert window region to screen coordinates
							OffsetRgn( hTempRgn2, aTempRect.left, aTempRect.top );
                            // and intersect with the window's bounding box
							CombineRgn( hTempRgn, hTempRgn, hTempRgn2, RGN_AND );
						}
                        // finally compute that part of aSrcRect which is not covered by any parts of hWnd
						CombineRgn( hInvalidateRgn, hInvalidateRgn, hTempRgn, RGN_DIFF );
						DeleteRegion( hTempRgn );
					}
				}
                // retrieve the next window in the z-order, i.e. the window below hwnd
				hWnd = GetWindow( hWnd, GW_HWNDNEXT );
			}
			if ( hTempRgn2 )
				DeleteRegion( hTempRgn2 );
			if ( hInvalidateRgn )
			{
                // hInvalidateRgn contains the fully visible parts of the original srcRect
				hTempRgn = CreateRectRgnIndirect( &aSrcRect );
                // subtract it from the original rect to get the occluded parts
				nRgnType = CombineRgn( hInvalidateRgn, hTempRgn, hInvalidateRgn, RGN_DIFF );
				DeleteRegion( hTempRgn );

				if ( (nRgnType != ERROR) && (nRgnType != NULLREGION) )
				{
                    // move the occluded parts to the destination pos
					int nOffX = (int)(nDestX-nSrcX);
					int nOffY = (int)(nDestY-nSrcY);
					OffsetRgn( hInvalidateRgn, nOffX-aPt.x, nOffY-aPt.y );

                    // by excluding hInvalidateRgn from the system's clip region
                    // we will prevent bitblt from copying useless data
                    // epsecially now shadows from overlapping windows will appear (#i36344)
                    hOldClipRgn = CreateRectRgn( 0, 0, 0, 0 );
                    nOldClipRgnType = GetClipRgn( getHDC(), hOldClipRgn );

                    bRestoreClipRgn = TRUE; // indicate changed clipregion and force invalidate
                    ExtSelectClipRgn( getHDC(), hInvalidateRgn, RGN_DIFF );
				}
			}
		}
	}

	BitBlt( getHDC(),
			(int)nDestX, (int)nDestY,
			(int)nSrcWidth, (int)nSrcHeight,
			getHDC(),
			(int)nSrcX, (int)nSrcY,
			SRCCOPY );

    if( bRestoreClipRgn )
    {
        // restore old clip region
        if( nOldClipRgnType != ERROR )
            SelectClipRgn( getHDC(), hOldClipRgn);
        DeleteRegion( hOldClipRgn );

        // invalidate regions that were not copied
        bool    bInvalidate = true;

		// Combine Invalidate Region with existing ClipRegion
        HRGN    hTempRgn = CreateRectRgn( 0, 0, 0, 0 );
		if ( GetClipRgn( getHDC(), hTempRgn ) == 1 )
        {
			int nRgnType = CombineRgn( hInvalidateRgn, hTempRgn, hInvalidateRgn, RGN_AND );
		    if ( (nRgnType == ERROR) || (nRgnType == NULLREGION) )
                bInvalidate = false;
        }
        DeleteRegion( hTempRgn );

		if ( bInvalidate )
		{
			InvalidateRgn( mhWnd, hInvalidateRgn, TRUE );
			// Hier loesen wir nur ein Update aus, wenn es der
			// MainThread ist, damit es beim Bearbeiten der
			// Paint-Message keinen Deadlock gibt, da der
			// SolarMutex durch diesen Thread schon gelockt ist
			SalData*	pSalData = GetSalData();
			DWORD		nCurThreadId = GetCurrentThreadId();
			if ( pSalData->mnAppThreadId == nCurThreadId )
				UpdateWindow( mhWnd );
		}

        DeleteRegion( hInvalidateRgn );
    }

}

// -----------------------------------------------------------------------

void ImplDrawBitmap( HDC hDC,
					 const SalTwoRect& rPosAry, const WinSalBitmap& rSalBitmap,
					 sal_Bool bPrinter, int nDrawMode )
{
	if( hDC )
	{
		HGLOBAL 	hDrawDIB;
		HBITMAP 	hDrawDDB = rSalBitmap.ImplGethDDB();
		WinSalBitmap*	pTmpSalBmp = NULL;
		sal_Bool		bPrintDDB = ( bPrinter && hDrawDDB );

		if( bPrintDDB )
		{
			pTmpSalBmp = new WinSalBitmap;
			pTmpSalBmp->Create( rSalBitmap, rSalBitmap.GetBitCount() );
			hDrawDIB = pTmpSalBmp->ImplGethDIB();
		}
		else
			hDrawDIB = rSalBitmap.ImplGethDIB();

		if( hDrawDIB )
		{
			PBITMAPINFO 		pBI = (PBITMAPINFO) GlobalLock( hDrawDIB );
			PBITMAPINFOHEADER	pBIH = (PBITMAPINFOHEADER) pBI;
			PBYTE				pBits = (PBYTE) pBI + *(DWORD*) pBI +
										rSalBitmap.ImplGetDIBColorCount( hDrawDIB ) * sizeof( RGBQUAD );
			const int			nOldStretchMode = SetStretchBltMode( hDC, STRETCH_DELETESCANS );

			StretchDIBits( hDC,
                           (int)rPosAry.mnDestX, (int)rPosAry.mnDestY,
                           (int)rPosAry.mnDestWidth, (int)rPosAry.mnDestHeight,
                           (int)rPosAry.mnSrcX, (int)(pBIH->biHeight - rPosAry.mnSrcHeight - rPosAry.mnSrcY),
                           (int)rPosAry.mnSrcWidth, (int)rPosAry.mnSrcHeight,
                           pBits, pBI, DIB_RGB_COLORS, nDrawMode );

			GlobalUnlock( hDrawDIB );
			SetStretchBltMode( hDC, nOldStretchMode );
		}
		else if( hDrawDDB && !bPrintDDB )
		{
			HDC 		hBmpDC = ImplGetCachedDC( CACHED_HDC_DRAW, hDrawDDB );
			COLORREF	nOldBkColor = RGB(0xFF,0xFF,0xFF);
			COLORREF	nOldTextColor = RGB(0,0,0);
			sal_Bool		bMono = ( rSalBitmap.GetBitCount() == 1 );

			if( bMono )
			{
				nOldBkColor = SetBkColor( hDC, RGB( 0xFF, 0xFF, 0xFF ) );
				nOldTextColor = ::SetTextColor( hDC, RGB( 0x00, 0x00, 0x00 ) );
			}

			if ( (rPosAry.mnSrcWidth  == rPosAry.mnDestWidth) &&
				 (rPosAry.mnSrcHeight == rPosAry.mnDestHeight) )
			{
				BitBlt( hDC,
						(int)rPosAry.mnDestX, (int)rPosAry.mnDestY,
						(int)rPosAry.mnDestWidth, (int)rPosAry.mnDestHeight,
						hBmpDC,
						(int)rPosAry.mnSrcX, (int)rPosAry.mnSrcY,
						nDrawMode );
			}
			else
			{
				const int nOldStretchMode = SetStretchBltMode( hDC, STRETCH_DELETESCANS );

				StretchBlt( hDC,
							(int)rPosAry.mnDestX, (int)rPosAry.mnDestY,
							(int)rPosAry.mnDestWidth, (int)rPosAry.mnDestHeight,
							hBmpDC,
							(int)rPosAry.mnSrcX, (int)rPosAry.mnSrcY,
							(int)rPosAry.mnSrcWidth, (int)rPosAry.mnSrcHeight,
							nDrawMode );

				SetStretchBltMode( hDC, nOldStretchMode );
			}

			if( bMono )
			{
				SetBkColor( hDC, nOldBkColor );
				::SetTextColor( hDC, nOldTextColor );
			}

			ImplReleaseCachedDC( CACHED_HDC_DRAW );
		}

		if( bPrintDDB )
			delete pTmpSalBmp;
	}
}

// -----------------------------------------------------------------------

void WinSalGraphics::drawBitmap(const SalTwoRect& rPosAry, const SalBitmap& rSalBitmap)
{
    bool bTryDirectPaint(!mbPrinter && !mbXORMode);

    if(bTryDirectPaint)
    {
        // only paint direct when no scaling and no MapMode, else the
        // more expensive conversions may be done for short-time Bitmap/BitmapEx
        // used for buffering only
        if(rPosAry.mnSrcWidth == rPosAry.mnDestWidth && rPosAry.mnSrcHeight == rPosAry.mnDestHeight)
        {
            bTryDirectPaint = false;
        }
    }

    // try to draw using GdiPlus directly
    if(bTryDirectPaint && tryDrawBitmapGdiPlus(rPosAry, rSalBitmap))
    {
        return;
    }

    // fall back old stuff
    ImplDrawBitmap(getHDC(), rPosAry, static_cast<const WinSalBitmap&>(rSalBitmap),
        mbPrinter,
        mbXORMode ? SRCINVERT : SRCCOPY );
}

// -----------------------------------------------------------------------

void WinSalGraphics::drawBitmap( const SalTwoRect& rPosAry,
							  const SalBitmap& rSSalBitmap,
							  SalColor nTransparentColor )
{
	DBG_ASSERT( !mbPrinter, "No transparency print possible!" );

    const WinSalBitmap& rSalBitmap = static_cast<const WinSalBitmap&>(rSSalBitmap);

	WinSalBitmap*	pMask = new WinSalBitmap;
	const Point aPoint;
	const Size	aSize( rSalBitmap.GetSize() );
	HBITMAP 	hMaskBitmap = CreateBitmap( (int) aSize.Width(), (int) aSize.Height(), 1, 1, NULL );
	HDC 		hMaskDC = ImplGetCachedDC( CACHED_HDC_1, hMaskBitmap );
	const BYTE	cRed = SALCOLOR_RED( nTransparentColor );
	const BYTE	cGreen = SALCOLOR_GREEN( nTransparentColor );
	const BYTE	cBlue = SALCOLOR_BLUE( nTransparentColor );

	if( rSalBitmap.ImplGethDDB() )
	{
		HDC 		hSrcDC = ImplGetCachedDC( CACHED_HDC_2, rSalBitmap.ImplGethDDB() );
		COLORREF	aOldCol = SetBkColor( hSrcDC, RGB( cRed, cGreen, cBlue ) );

		BitBlt( hMaskDC, 0, 0, (int) aSize.Width(), (int) aSize.Height(), hSrcDC, 0, 0, SRCCOPY );

		SetBkColor( hSrcDC, aOldCol );
		ImplReleaseCachedDC( CACHED_HDC_2 );
	}
	else
	{
		WinSalBitmap*	pTmpSalBmp = new WinSalBitmap;

		if( pTmpSalBmp->Create( rSalBitmap, this ) )
		{
			HDC 		hSrcDC = ImplGetCachedDC( CACHED_HDC_2, pTmpSalBmp->ImplGethDDB() );
			COLORREF	aOldCol = SetBkColor( hSrcDC, RGB( cRed, cGreen, cBlue ) );

			BitBlt( hMaskDC, 0, 0, (int) aSize.Width(), (int) aSize.Height(), hSrcDC, 0, 0, SRCCOPY );

			SetBkColor( hSrcDC, aOldCol );
			ImplReleaseCachedDC( CACHED_HDC_2 );
		}

		delete pTmpSalBmp;
	}

	ImplReleaseCachedDC( CACHED_HDC_1 );

	// hMaskBitmap is destroyed by new SalBitmap 'pMask' ( bDIB==FALSE, bCopy == FALSE )
	if( pMask->Create( hMaskBitmap, FALSE, FALSE ) )
		drawBitmap( rPosAry, rSalBitmap, *pMask );

	delete pMask;
}

// -----------------------------------------------------------------------

void WinSalGraphics::drawBitmap( const SalTwoRect& rPosAry,
							  const SalBitmap& rSSalBitmap,
							  const SalBitmap& rSTransparentBitmap )
{
	DBG_ASSERT( !mbPrinter, "No transparency print possible!" );
    bool bTryDirectPaint(!mbPrinter && !mbXORMode);

    if(bTryDirectPaint)
    {
        // only paint direct when no scaling and no MapMode, else the
        // more expensive conversions may be done for short-time Bitmap/BitmapEx
        // used for buffering only
        if(rPosAry.mnSrcWidth == rPosAry.mnDestWidth && rPosAry.mnSrcHeight == rPosAry.mnDestHeight)
        {
            bTryDirectPaint = false;
        }
    }

    // try to draw using GdiPlus directly
    if(bTryDirectPaint && drawAlphaBitmap(rPosAry, rSSalBitmap, rSTransparentBitmap))
    {
        return;
    }

    const WinSalBitmap& rSalBitmap = static_cast<const WinSalBitmap&>(rSSalBitmap);
    const WinSalBitmap& rTransparentBitmap = static_cast<const WinSalBitmap&>(rSTransparentBitmap);

	SalTwoRect	aPosAry = rPosAry;
	int 		nDstX = (int)aPosAry.mnDestX;
	int 		nDstY = (int)aPosAry.mnDestY;
	int 		nDstWidth = (int)aPosAry.mnDestWidth;
	int 		nDstHeight = (int)aPosAry.mnDestHeight;
	HDC 		hDC = getHDC();
	HBITMAP 	hMemBitmap = 0;
	HBITMAP 	hMaskBitmap = 0;

	if( ( nDstWidth > CACHED_HDC_DEFEXT ) || ( nDstHeight > CACHED_HDC_DEFEXT ) )
	{
		hMemBitmap = CreateCompatibleBitmap( hDC, nDstWidth, nDstHeight );
		hMaskBitmap = CreateCompatibleBitmap( hDC, nDstWidth, nDstHeight );
	}

	HDC hMemDC = ImplGetCachedDC( CACHED_HDC_1, hMemBitmap );
	HDC hMaskDC = ImplGetCachedDC( CACHED_HDC_2, hMaskBitmap );

	aPosAry.mnDestX = aPosAry.mnDestY = 0;
	BitBlt( hMemDC, 0, 0, nDstWidth, nDstHeight, hDC, nDstX, nDstY, SRCCOPY );

	// bei Paletten-Displays hat WIN/WNT offenbar ein kleines Problem,
	// die Farben der Maske richtig auf die Palette abzubilden,
	// wenn wir die DIB direkt ausgeben => DDB-Ausgabe
	if( ( GetBitCount() <= 8 ) && rTransparentBitmap.ImplGethDIB() && rTransparentBitmap.GetBitCount() == 1 )
	{
		WinSalBitmap aTmp;

		if( aTmp.Create( rTransparentBitmap, this ) )
			ImplDrawBitmap( hMaskDC, aPosAry, aTmp, FALSE, SRCCOPY );
	}
	else
		ImplDrawBitmap( hMaskDC, aPosAry, rTransparentBitmap, FALSE, SRCCOPY );

    // now MemDC contains background, MaskDC the transparency mask

    // #105055# Respect XOR mode
    if( mbXORMode )
    {
        ImplDrawBitmap( hMaskDC, aPosAry, rSalBitmap, FALSE, SRCERASE );
        // now MaskDC contains the bitmap area with black background
        BitBlt( hMemDC, 0, 0, nDstWidth, nDstHeight, hMaskDC, 0, 0, SRCINVERT );
        // now MemDC contains background XORed bitmap area ontop
    }
    else
    {
        BitBlt( hMemDC, 0, 0, nDstWidth, nDstHeight, hMaskDC, 0, 0, SRCAND );       
        // now MemDC contains background with masked-out bitmap area
        ImplDrawBitmap( hMaskDC, aPosAry, rSalBitmap, FALSE, SRCERASE );
        // now MaskDC contains the bitmap area with black background
        BitBlt( hMemDC, 0, 0, nDstWidth, nDstHeight, hMaskDC, 0, 0, SRCPAINT );
        // now MemDC contains background and bitmap merged together
    }
    // copy to output DC
    BitBlt( hDC, nDstX, nDstY, nDstWidth, nDstHeight, hMemDC, 0, 0, SRCCOPY );

	ImplReleaseCachedDC( CACHED_HDC_1 );
	ImplReleaseCachedDC( CACHED_HDC_2 );

	// hMemBitmap != 0 ==> hMaskBitmap != 0
	if( hMemBitmap )
	{
		DeleteObject( hMemBitmap );
		DeleteObject( hMaskBitmap );
	}
}

// -----------------------------------------------------------------------

bool WinSalGraphics::drawAlphaRect( long nX, long nY, long nWidth, 
                                    long nHeight, sal_uInt8 nTransparency )
{
    if( mbPen || !mbBrush || mbXORMode )
        return false; // can only perform solid fills without XOR.

	HDC hMemDC = ImplGetCachedDC( CACHED_HDC_1, 0 );
    SetPixel( hMemDC, (int)0, (int)0, mnBrushColor );

    BLENDFUNCTION aFunc = {
        AC_SRC_OVER,
        0,
        255 - 255L*nTransparency/100,
        0
    };

    // hMemDC contains a 1x1 bitmap of the right color - stretch-blit
    // that to dest hdc
    bool bRet = AlphaBlend( getHDC(), nX, nY, nWidth, nHeight, 
                            hMemDC, 0,0,1,1,
                            aFunc ) == TRUE;

	ImplReleaseCachedDC( CACHED_HDC_1 );

    return bRet;
}

// -----------------------------------------------------------------------

void WinSalGraphics::drawMask( const SalTwoRect& rPosAry,
							const SalBitmap& rSSalBitmap,
							SalColor nMaskColor )
{
	DBG_ASSERT( !mbPrinter, "No transparency print possible!" );

    const WinSalBitmap& rSalBitmap = static_cast<const WinSalBitmap&>(rSSalBitmap);

	SalTwoRect	aPosAry = rPosAry;
	const BYTE	cRed = SALCOLOR_RED( nMaskColor );
	const BYTE	cGreen = SALCOLOR_GREEN( nMaskColor );
	const BYTE	cBlue = SALCOLOR_BLUE( nMaskColor );
	HDC 		hDC = getHDC();
	HBRUSH		hMaskBrush = CreateSolidBrush( RGB( cRed, cGreen, cBlue ) );
	HBRUSH		hOldBrush = SelectBrush( hDC, hMaskBrush );

	// bei Paletten-Displays hat WIN/WNT offenbar ein kleines Problem,
	// die Farben der Maske richtig auf die Palette abzubilden,
	// wenn wir die DIB direkt ausgeben => DDB-Ausgabe
	if( ( GetBitCount() <= 8 ) && rSalBitmap.ImplGethDIB() && rSalBitmap.GetBitCount() == 1 )
	{
		WinSalBitmap aTmp;

		if( aTmp.Create( rSalBitmap, this ) )
			ImplDrawBitmap( hDC, aPosAry, aTmp, FALSE, 0x00B8074AUL );
	}
	else
		ImplDrawBitmap( hDC, aPosAry, rSalBitmap, FALSE, 0x00B8074AUL );

	SelectBrush( hDC, hOldBrush );
	DeleteBrush( hMaskBrush );
}

// -----------------------------------------------------------------------

SalBitmap* WinSalGraphics::getBitmap( long nX, long nY, long nDX, long nDY )
{
	DBG_ASSERT( !mbPrinter, "No ::GetBitmap() from printer possible!" );

	WinSalBitmap* pSalBitmap = NULL;

	nDX = labs( nDX );
	nDY = labs( nDY );

	HDC 	hDC = getHDC();
	HBITMAP hBmpBitmap = CreateCompatibleBitmap( hDC, nDX, nDY );
	HDC 	hBmpDC = ImplGetCachedDC( CACHED_HDC_1, hBmpBitmap );
	sal_Bool	bRet;
    DWORD err = 0;

	bRet = BitBlt( hBmpDC, 0, 0, (int) nDX, (int) nDY, hDC, (int) nX, (int) nY, SRCCOPY ) ? TRUE : FALSE;
	ImplReleaseCachedDC( CACHED_HDC_1 );

	if( bRet )
	{
		pSalBitmap = new WinSalBitmap;

		if( !pSalBitmap->Create( hBmpBitmap, FALSE, FALSE ) )
		{
			delete pSalBitmap;
			pSalBitmap = NULL;
		}
	}
    else
    {
        err = GetLastError();
        // #124826# avoid resource leak ! happens when running without desktop access (remote desktop, service, may be screensavers)
        DeleteBitmap( hBmpBitmap );
    }

	return pSalBitmap;
}

// -----------------------------------------------------------------------

SalColor WinSalGraphics::getPixel( long nX, long nY )
{
	COLORREF aWinCol = ::GetPixel( getHDC(), (int) nX, (int) nY );

	if ( CLR_INVALID == aWinCol )
		return MAKE_SALCOLOR( 0, 0, 0 );
	else
		return MAKE_SALCOLOR( GetRValue( aWinCol ),
							  GetGValue( aWinCol ),
							  GetBValue( aWinCol ) );
}

// -----------------------------------------------------------------------

void WinSalGraphics::invert( long nX, long nY, long nWidth, long nHeight, SalInvert nFlags )
{
	if ( nFlags & SAL_INVERT_TRACKFRAME )
	{
		HPEN	hDotPen = CreatePen( PS_DOT, 0, 0 );
		HPEN	hOldPen = SelectPen( getHDC(), hDotPen );
		HBRUSH	hOldBrush = SelectBrush( getHDC(), GetStockBrush( NULL_BRUSH ) );
		int 	nOldROP = SetROP2( getHDC(), R2_NOT );

		WIN_Rectangle( getHDC(), (int)nX, (int)nY, (int)(nX+nWidth), (int)(nY+nHeight) );

		SetROP2( getHDC(), nOldROP );
		SelectPen( getHDC(), hOldPen );
		SelectBrush( getHDC(), hOldBrush );
		DeletePen( hDotPen );
	}
	else if ( nFlags & SAL_INVERT_50 )
	{
		SalData* pSalData = GetSalData();
		if ( !pSalData->mh50Brush )
		{
			if ( !pSalData->mh50Bmp )
				pSalData->mh50Bmp = ImplLoadSalBitmap( SAL_RESID_BITMAP_50 );
			pSalData->mh50Brush = CreatePatternBrush( pSalData->mh50Bmp );
		}

		COLORREF nOldTextColor = ::SetTextColor( getHDC(), 0 );
		HBRUSH hOldBrush = SelectBrush( getHDC(), pSalData->mh50Brush );
		PatBlt( getHDC(), nX, nY, nWidth, nHeight, PATINVERT );
		::SetTextColor( getHDC(), nOldTextColor );
		SelectBrush( getHDC(), hOldBrush );
	}
	else
	{
		 RECT aRect;
		 aRect.left 	 = (int)nX;
		 aRect.top		 = (int)nY;
		 aRect.right	 = (int)nX+nWidth;
		 aRect.bottom	 = (int)nY+nHeight;
		 ::InvertRect( getHDC(), &aRect );
	}
}

// -----------------------------------------------------------------------

void WinSalGraphics::invert( sal_uInt32 nPoints, const SalPoint* pPtAry, SalInvert nSalFlags )
{
	HPEN		hPen;
	HPEN		hOldPen;
	HBRUSH		hBrush;
	HBRUSH		hOldBrush = 0;
	COLORREF	nOldTextColor RGB(0,0,0);
	int 		nOldROP = SetROP2( getHDC(), R2_NOT );

	if ( nSalFlags & SAL_INVERT_TRACKFRAME )
		hPen = CreatePen( PS_DOT, 0, 0 );
	else
	{

		if ( nSalFlags & SAL_INVERT_50 )
		{
			SalData* pSalData = GetSalData();
			if ( !pSalData->mh50Brush )
			{
				if ( !pSalData->mh50Bmp )
					pSalData->mh50Bmp = ImplLoadSalBitmap( SAL_RESID_BITMAP_50 );
				pSalData->mh50Brush = CreatePatternBrush( pSalData->mh50Bmp );
			}

			hBrush = pSalData->mh50Brush;
		}
		else
			hBrush = GetStockBrush( BLACK_BRUSH );

		hPen = GetStockPen( NULL_PEN );
		nOldTextColor = ::SetTextColor( getHDC(), 0 );
		hOldBrush = SelectBrush( getHDC(), hBrush );
	}
	hOldPen = SelectPen( getHDC(), hPen );

	POINT* pWinPtAry;
	// Unter NT koennen wir das Array direkt weiterreichen
	DBG_ASSERT( sizeof( POINT ) == sizeof( SalPoint ),
				"WinSalGraphics::DrawPolyLine(): POINT != SalPoint" );

	pWinPtAry = (POINT*)pPtAry;
	// Wegen Windows 95 und der Beschraenkung auf eine maximale Anzahl
	// von Punkten
	if ( nSalFlags & SAL_INVERT_TRACKFRAME )
	{
		if ( !Polyline( getHDC(), pWinPtAry, (int)nPoints ) && (nPoints > MAX_64KSALPOINTS) )
			Polyline( getHDC(), pWinPtAry, MAX_64KSALPOINTS );
	}
	else
	{
		if ( !WIN_Polygon( getHDC(), pWinPtAry, (int)nPoints ) && (nPoints > MAX_64KSALPOINTS) )
			WIN_Polygon( getHDC(), pWinPtAry, MAX_64KSALPOINTS );
	}

	SetROP2( getHDC(), nOldROP );
	SelectPen( getHDC(), hOldPen );

	if ( nSalFlags & SAL_INVERT_TRACKFRAME )
		DeletePen( hPen );
	else
	{
		::SetTextColor( getHDC(), nOldTextColor );
		SelectBrush( getHDC(), hOldBrush );
	}
}
