/* by Sven2, 2001 */

/* NewGfx interfaces */
#include <Standard.h>
#include <StdFacet.h>
#include <StdDDraw2.h>
#include <StdD3D.h>
#include <StdGL.h>
#include <StdNoGfx.h>
#include <StdMarkup.h>
#include <StdFont.h>
#include <StdWindow.h>

#include <stdio.h>
#include <math.h>
#include <limits.h>

// Global access pointer
CStdDDraw *lpDDraw=NULL;
CStdPalette *lpDDrawPal=NULL;
int iGfxEngine=-1;

inline void SetRect(RECT &rect, int left, int top, int right, int bottom)
  {
  rect.left=left; rect.top=top; rect.bottom=bottom; rect.right=right;
  }

inline DWORD GetTextShadowClr(DWORD dwTxtClr)
	{
	return RGBA(((dwTxtClr >>  0) % 256) / 3, ((dwTxtClr >>  8) % 256) / 3, ((dwTxtClr >> 16) % 256) / 3,	(dwTxtClr >> 24) % 256);
	}

void CBltTransform::SetRotate(int iAngle, float fOffX, float fOffY) // set by angle and rotation offset
	{
	// iAngle is in 1/100-degrees (cycling from 0 to 36000)
	// determine sine and cos of reversed angle in radians
	// fAngle = -iAngle/100 * pi/180 = iAngle * -pi/18000
	float fAngle=(float) iAngle*(-1.7453292519943295769236907684886e-4f);
	float fsin=(float)sin(fAngle); float fcos=(float)cos(fAngle);
	// set matrix values
	mat[0] = +fcos; mat[1] = +fsin; mat[2] = (1-fcos)*fOffX - fsin*fOffY;
	mat[3] = -fsin; mat[4] = +fcos; mat[5] = (1-fcos)*fOffY + fsin*fOffX;
	mat[6] = 0; mat[7] = 0; mat[8] = 1;
/*    calculation of rotation matrix:
	x2 = fcos*(x1-fOffX) + fsin*(y1-fOffY) + fOffX
		 = fcos*x1 - fcos*fOffX + fsin*y1 - fsin*fOffY + fOffX
		 = x1*fcos + y1*fsin + (1-fcos)*fOffX - fsin*fOffY

	y2 = -fsin*(x1-fOffX) + fcos*(y1-fOffY) + fOffY
		 = x1*-fsin + fsin*fOffX + y1*fcos - fcos*fOffY + fOffY
		 = x1*-fsin + y1*fcos + fsin*fOffX + (1-fcos)*fOffY */
	}

bool CBltTransform::SetAsInv(CBltTransform &r)
	{
	// calc inverse of matrix
	float det = r.mat[0]*r.mat[4]*r.mat[8] + r.mat[1]*r.mat[5]*r.mat[6]
		+ r.mat[2]*r.mat[3]*r.mat[7] - r.mat[2]*r.mat[4]*r.mat[6]
		- r.mat[0]*r.mat[5]*r.mat[7] - r.mat[1]*r.mat[3]*r.mat[8];
	if (!det) { Set(1,0,0,0,1,0,0,0,1); return false; }
	mat[0] = (r.mat[4] * r.mat[8] - r.mat[5] * r.mat[7]) / det;
	mat[1] = (r.mat[2] * r.mat[7] - r.mat[1] * r.mat[8]) / det;
	mat[2] = (r.mat[1] * r.mat[5] - r.mat[2] * r.mat[4]) / det;
	mat[3] = (r.mat[5] * r.mat[6] - r.mat[3] * r.mat[8]) / det;
	mat[4] = (r.mat[0] * r.mat[8] - r.mat[2] * r.mat[6]) / det;
	mat[5] = (r.mat[2] * r.mat[3] - r.mat[0] * r.mat[5]) / det;
	mat[6] = (r.mat[3] * r.mat[7] - r.mat[4] * r.mat[6]) / det;
	mat[7] = (r.mat[1] * r.mat[6] - r.mat[0] * r.mat[7]) / det;
	mat[8] = (r.mat[0] * r.mat[4] - r.mat[1] * r.mat[3]) / det;
	return true;
	}

void CBltTransform::TransformPoint(float &rX, float &rY)
	{
	// apply matrix
	float fW = mat[6] * rX + mat[7] * rY + mat[8];
	// store in temp, so original rX is used for calculation of rY
	float fX = (mat[0] * rX + mat[1] * rY + mat[2]) / fW;
	rY = (mat[3] * rX + mat[4] * rY + mat[5]) / fW;
	rX = fX; // apply temp
	}

CPattern& CPattern::operator=(const CPattern& nPattern)
	{
	pClrs = nPattern.pClrs;
	pAlpha = nPattern.pAlpha;
	sfcPattern8 = nPattern.sfcPattern8;
	sfcPattern32 = nPattern.sfcPattern32;
	if (sfcPattern32) sfcPattern32->Lock();
	delete [] CachedPattern;
	if (nPattern.CachedPattern)
		{
		CachedPattern = new uint32_t[sfcPattern32->Wdt * sfcPattern32->Hgt];
		memcpy(CachedPattern, nPattern.CachedPattern, sfcPattern32->Wdt * sfcPattern32->Hgt * 4);
		}
	else
		{
		CachedPattern = 0;
		}
	Wdt = nPattern.Wdt;
	Hgt = nPattern.Hgt;
	Zoom = nPattern.Zoom;
	Monochrome = nPattern.Monochrome;
	return *this;
	}

bool CPattern::Set(SURFACE sfcSource, int iZoom, bool fMonochrome)
	{
	// Safety
	if (!sfcSource) return false;
	// Clear existing pattern
	Clear();
	// new style: simply store pattern for modulation or shifting, which will be decided upon use
	sfcPattern32=sfcSource;
	sfcPattern32->Lock();
	Wdt = sfcPattern32->Wdt;
	Hgt = sfcPattern32->Hgt;
	// set zoom
	Zoom=iZoom;
	// set flags
	Monochrome=fMonochrome;
	CachedPattern = new uint32_t[Wdt * Hgt];
	if (!CachedPattern) return false;
	for (int y = 0; y < Hgt; ++y)
		for (int x = 0; x < Wdt; ++x)
			{
			CachedPattern[y * Wdt + x] = sfcPattern32->GetPixDw(x, y, false);
			}
	return true;
	}

bool CPattern::Set(CSurface8 * sfcSource, int iZoom, bool fMonochrome)
	{
	// Safety
	if (!sfcSource) return false;
	// Clear existing pattern
	Clear();
	// new style: simply store pattern for modulation or shifting, which will be decided upon use
	sfcPattern8=sfcSource;
	Wdt = sfcPattern8->Wdt;
	Hgt = sfcPattern8->Hgt;
	// set zoom
	Zoom=iZoom;
	// set flags
	Monochrome=fMonochrome;
	CachedPattern = 0;
	return true;
	}

CPattern::CPattern()
	{
	// disable
	sfcPattern32=NULL;
	sfcPattern8=NULL;
	CachedPattern = 0;
	Zoom=0;
	Monochrome=false;
	pClrs=NULL; pAlpha=NULL;
	}

void CPattern::Clear()
	{
	// pattern assigned
	if (sfcPattern32)
		{
		// unlock it
		sfcPattern32->Unlock();
		// clear field
		sfcPattern32=NULL;
		}
	sfcPattern8 = NULL;
	delete[] CachedPattern; CachedPattern = 0;
	}

bool CPattern::PatternClr(int iX, int iY, BYTE &byClr, DWORD &dwClr, CStdPalette &rPal) const
	{
	// pattern assigned?
	if (!sfcPattern32 && !sfcPattern8) return false;
	// position zoomed?
	if (Zoom) { iX/=Zoom; iY/=Zoom; }
	// modulate position
	((unsigned int &)iX) %= Wdt; ((unsigned int &)iY) %= Hgt;
	// new style: modulate clr
	if (CachedPattern)
		{
		DWORD dwPix = CachedPattern[iY * Wdt + iX];
		if (byClr)
			{
			if (Monochrome)
				ModulateClrMonoA(dwClr, BYTE(dwPix), BYTE(dwPix>>24));
			else
				ModulateClrA(dwClr, dwPix);
			LightenClr(dwClr);
			}
		else dwClr=dwPix;
		}
	// old style?
	else if (sfcPattern8)
		{
		// if color triplet is given, use it
		BYTE byShift = sfcPattern8->GetPix(iX, iY);
		if (pClrs) 
			{
			// IFT (alpha only)
			int iAShift=0; if (byClr & 0xf0) iAShift = 3;
			// compose color
			dwClr = RGB(pClrs[byShift*3+2], pClrs[byShift*3+1], pClrs[byShift*3])+(pAlpha[byShift+iAShift]<<24);
			}
		else
			{
			// shift color index and return indexed color
			byClr+=byShift;
			dwClr=rPal.GetClr(byClr);
			}
		}
	// success
	return true;
	}

CGammaControl::~CGammaControl()
{
	delete[] red;;
}

void CGammaControl::SetClrChannel(uint16_t *pBuf, BYTE c1, BYTE c2, int c3, uint16_t *ref)
	{
	// Using this minimum value, gamma ramp errors on some cards can be avoided
	int MinGamma = 0x100;
	// adjust clr3-value
	++c3;
	// calc ramp
	uint16_t *pBuf1 = pBuf;
	uint16_t *pBuf2 = pBuf + size / 2;
	for (int i = 0; i < size / 2; ++i)
		{
		int i2 = size / 2 - i;
		int size1 = size - 1;
		// interpolate ramps between the three points
		*pBuf1++ = BoundBy(((c1 * i2 + c2 * i) * size1 + (2 * c2 - c1 - c3) * 2 * i * i2)/(size1*size1/512), 0, 0xffff);
		*pBuf2++ = BoundBy(((c2 * i2 + c3 * i) * size1 + (2 * c2 - c1 - c3) * 2 * i * i2)/(size1*size1/512), 0, 0xffff);
		}
	if (ref)
		{
		for (int i = 0; i < size; ++i)
			{
			int c = 0x10000 * i / (size-1);
			*pBuf = BoundBy(*pBuf + ref[i] - c, MinGamma, 0xffff);
			++pBuf;
			}
		}
	else
		{
		for (int i = 0; i < size; ++i)
			{
			*pBuf = BoundBy<int>(*pBuf, MinGamma, 0xffff);
			++pBuf;
			}
		}
	}

void CGammaControl::Set(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, int nsize, CGammaControl * ref)
	{
	if (nsize != size)
		{
		delete[] red;
		red = new uint16_t[nsize * 3];
		green = &red[nsize];
		blue = &red[nsize * 2];
		size = nsize;
		}
	// set red, green and blue channel
	SetClrChannel(red,   GetBValue(dwClr1), GetBValue(dwClr2), GetBValue(dwClr3), ref ? ref->red : 0);
	SetClrChannel(green, GetGValue(dwClr1), GetGValue(dwClr2), GetGValue(dwClr3), ref ? ref->green : 0);
	SetClrChannel(blue,  GetRValue(dwClr1), GetRValue(dwClr2), GetRValue(dwClr3), ref ? ref->blue : 0);
	}

DWORD CGammaControl::ApplyTo(DWORD dwClr)
	{
	// apply to reg, green and blue color component
	return RGBA(red[GetBValue(dwClr)*256/size]>>8, green[GetGValue(dwClr)*256/size]>>8, blue[GetRValue(dwClr)*256/size]>>8, dwClr>>24);
	}


//--------------------------------------------------------------------

void CClrModAddMap::Reset(int iResX, int iResY, int iWdtPx, int iHgtPx, int iOffX, int iOffY, uint32_t dwModClr, uint32_t dwAddClr, int x0, int y0, uint32_t dwBackClr, class CSurface *backsfc)
	{
	// set values
	iResolutionX = iResX; iResolutionY = iResY;
	this->iOffX = -((iOffX) % iResolutionX);
	this->iOffY = -((iOffY) % iResolutionY);
	// calc w/h required for map
	iWdt = (iWdtPx - this->iOffX + iResolutionX-1) / iResolutionX + 1;
	iHgt = (iHgtPx - this->iOffY + iResolutionY-1) / iResolutionY + 1;
	this->iOffX += x0;
	this->iOffY += y0;
	size_t iNewMapSize = iWdt * iHgt;
	if (iNewMapSize > iMapSize || iNewMapSize < iMapSize/2)
		{
		delete [] pMap;
		pMap = new CClrModAdd[iMapSize = iNewMapSize];
		}
	// is a background color desired?
	if (dwBackClr && backsfc)
		{
		// then draw a background now and fade against transparent later
		lpDDraw->DrawBoxDw(backsfc, x0,y0, x0+iWdtPx-1, y0+iHgtPx-1, dwBackClr);
		dwModClr = 0xffffffff;
		fFadeTransparent = true;
		}
	else
		fFadeTransparent = false;
	// reset all of map to given values
	int i = iMapSize;
	for (CClrModAdd *pCurr = pMap; i--; ++pCurr)
		{
		pCurr->dwModClr = dwModClr;
		pCurr->dwAddClr = dwAddClr;
		}
	}

void CClrModAddMap::ReduceModulation(int cx, int cy, int iRadius1, int iRadius2)
	{
	// reveal all within iRadius1; fade off squared until iRadius2
	int i = iMapSize, x = iOffX, y = iOffY, xe = iWdt*iResolutionX+iOffX;
	int iRadius1Sq = iRadius1*iRadius1, iRadius2Sq = iRadius2*iRadius2;
	for (CClrModAdd *pCurr = pMap; i--; ++pCurr)
		{
		int d = (x-cx)*(x-cx)+(y-cy)*(y-cy);
		if (d < iRadius2Sq)
			{
			if (d < iRadius1Sq)
				pCurr->dwModClr = 0xffffff; // full visibility
			else
				{
				// partly visible
				int iVis = (iRadius2Sq - d) * 255 / (iRadius2Sq - iRadius1Sq);
				pCurr->dwModClr = fFadeTransparent ? (0xffffff + (Min<uint32_t>(pCurr->dwModClr>>24, 255-iVis)<<24))
					                                 : Max<uint32_t>(pCurr->dwModClr, RGB(iVis, iVis, iVis));
				}
			}
		// next pos
		x += iResolutionX;
		if (x >= xe) { x = iOffX; y += iResolutionY; }
		}
	}

void CClrModAddMap::AddModulation(int cx, int cy, int iRadius1, int iRadius2, uint8_t byTransparency)
	{
	// hide all within iRadius1; fade off squared until iRadius2
	int i = iMapSize, x = iOffX, y = iOffY, xe = iWdt*iResolutionX+iOffX;
	int iRadius1Sq = iRadius1*iRadius1, iRadius2Sq = iRadius2*iRadius2;
	for (CClrModAdd *pCurr = pMap; i--; ++pCurr)
		{
		int d = (x-cx)*(x-cx)+(y-cy)*(y-cy);
		if (d < iRadius2Sq)
			{
			if (d < iRadius1Sq && !byTransparency)
				pCurr->dwModClr = 0x000000; // full invisibility
			else
				{
				// partly visible
				int iVis = Min<int>(255 - Min<int>((iRadius2Sq - d) * 255 / (iRadius2Sq - iRadius1Sq), 255) + byTransparency, 255);
				pCurr->dwModClr = fFadeTransparent ? (0xffffff + (Max<uint32_t>(pCurr->dwModClr>>24, 255-iVis)<<24))
					                                 : Min<uint32_t>(pCurr->dwModClr, RGB(iVis, iVis, iVis));
				}
			}
		// next pos
		x += iResolutionX;
		if (x >= xe) { x = iOffX; y += iResolutionY; }
		}
	}

uint32_t CClrModAddMap::GetModAt(int x, int y) const
	{
#if 0
	// fast but inaccurate method
	x = BoundBy((x - iOffX + iResolutionX/2) / iResolutionX, 0, iWdt-1);
	y = BoundBy((y - iOffY + iResolutionY/2) / iResolutionY, 0, iHgt-1);
	return pMap[y * iWdt + x]->dwModClr;
#else
	// slower, more accurate method: Interpolate between 4 neighboured modulations
	x -= iOffX;
	y -= iOffY;
	int tx = BoundBy(x / iResolutionX, 0, iWdt-1);
	int ty = BoundBy(y / iResolutionY, 0, iHgt-1);
	int tx2 = Min(tx + 1, iWdt-1);
	int ty2 = Min(ty + 1, iHgt-1);
	CColorFadeMatrix clrs(tx*iResolutionX, ty*iResolutionY, iResolutionX, iResolutionY, pMap[ty*iWdt+tx].dwModClr, pMap[ty*iWdt+tx2].dwModClr, pMap[ty2*iWdt+tx].dwModClr, pMap[ty2*iWdt+tx2].dwModClr);
	return clrs.GetColorAt(x, y);
#endif
	}

// -------------------------------------------------------------------

CColorFadeMatrix::CColorFadeMatrix(int iX, int iY, int iWdt, int iHgt, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4)
	: ox(iX), oy(iY), w(iWdt), h(iHgt)
	{
	uint32_t dwMask = 0xff; 
	for (int iChan = 0; iChan < 4; (++iChan),(dwMask<<=8))
		{
		int c0 = (dwClr1 & dwMask) >> (iChan*8);
		int cx = (dwClr2 & dwMask) >> (iChan*8);
		int cy = (dwClr3 & dwMask) >> (iChan*8);
		int ce = (dwClr4 & dwMask) >> (iChan*8);
		clrs[iChan].c0 = c0;
		clrs[iChan].cx = cx - c0;
		clrs[iChan].cy = cy - c0;
		clrs[iChan].ce = ce;
		}
	}

uint32_t CColorFadeMatrix::GetColorAt(int iX, int iY)
	{
	iX -= ox; iY -= oy;
	uint32_t dwResult = 0x00;
	for (int iChan = 0; iChan < 4; ++iChan)
		{
		int clr = clrs[iChan].c0 + clrs[iChan].cx * iX / w + clrs[iChan].cy * iY / h;
		clr += iX*iY * (clrs[iChan].ce - clr) / (w*h);
		dwResult |= (BoundBy(clr, 0, 255) << (iChan*8));
		}
	return dwResult;
	}

// -------------------------------------------------------------------

bool CBltData::ClipBy(float fX, float fY, float fMax)
	{
	CBltVertex *pLastVtx = &vtVtx[byNumVertices-1], *pVtx = vtVtx;
	CBltVertex v;
	bool fLastVtxIn = (pLastVtx->ftx*fX+pLastVtx->fty*fY <= fMax);
	for (int i = 0; i < byNumVertices; ++i)
		{
		// get in-state
		bool fVtxIn = (pVtx->ftx*fX+pVtx->fty*fY <= fMax);
		// does it lay outside?
		if (!fVtxIn)
			{
			// backup vtx lying outside
			v = *pVtx;
			// previous was inside?
			if (fLastVtxIn)
				{
				// move current vtx in
				float lx = pLastVtx->ftx, ly = pLastVtx->fty;
				float dx = v.ftx - lx, dy = v.fty - ly;
				float dst = dx*fX+dy*fY;
				if (dst)
					{
					// calculate hit point
					float l = (fMax - fX*lx - fY*ly) / dst;
					pVtx->ftx = lx + dx * l;
					pVtx->fty = ly + dy * l;
					}
				else
					{
					// zero distance: looks like both points lie on the clipping border
					// so the point counts as inside and needs not to be moved
					fVtxIn = true;
					}
				}
			else
				{
				// both vertices lying outside
				// delete this vtx
				--byNumVertices;
				--i;
				if (i+1 < byNumVertices) memmove(pVtx, pVtx+1, (byNumVertices-i-1)*sizeof(*pVtx));
				--pVtx;
				}
			// set last vtx
			pLastVtx = &v;
			}
		else
			{
			// this vertex lies inside the clipping region
			// was the last one outside?
			if (!fLastVtxIn)
				{
				// then insert an intersection point
				float lx = pLastVtx->ftx, ly = pLastVtx->fty;
				float dx = pVtx->ftx - lx, dy = pVtx->fty - ly;
				float dst = dx*fX+dy*fY;
				if (dst)
					{
					// move up other vertices
					if (++i < ++byNumVertices) memmove(pVtx+1, pVtx, (byNumVertices-i)*sizeof(*pVtx));
					// calculate hit point
					float l = (fMax - fX*lx - fY*ly) / dst;
					pVtx->ftx = lx + dx * l;
					pVtx->fty = ly + dy * l;
					++pVtx;
					}
				else
					{
					// zero distance: looks like both points lie on the clipping border
					// that counts as all points inside
					// nothing to do here
					}
				}
			else
				{
				// last point in and this point in: nothing to do
				}
			// assign last vertex
			pLastVtx = pVtx;
			}
		// next vertex
		fLastVtxIn = fVtxIn; ++pVtx;
		}
	// return whether enough points remain inside
	return byNumVertices>2;
	}

// -------------------------------------------------------------------

void CStdDDraw::Default()
	{
	fFullscreen=FALSE;
	RenderTarget=NULL;
	ClipAll=false;
	Active=false;
	BlitModulated=false;
	dwBlitMode = 0;
	Gamma.Default();
	DefRamp.Default();
	lpPrimary=lpBack=NULL;
	// pClrModMap = NULL; - invalid it !fUseClrModMap anyway
	fUseClrModMap = false;
	}

void CStdDDraw::Clear()
	{
	sLastError.Clear();
	DisableGamma();
	Active=BlitModulated=fUseClrModMap=false;
	dwBlitMode = 0;
	}

BOOL CStdDDraw::WipeSurface(SURFACE sfcSurface)
	{
	if(!sfcSurface) return FALSE;
	return sfcSurface->Wipe();
	}

BOOL CStdDDraw::GetSurfaceSize(SURFACE sfcSurface, int &iWdt, int &iHgt)
	{
	return sfcSurface->GetSurfaceSize(iWdt, iHgt);
	}

BOOL CStdDDraw::SetPrimaryPalette(BYTE *pBuf, BYTE *pAlphaBuf)
	{
	// store into loaded pal
	memcpy(&Pal.Colors, pBuf, 768);
	// store alpha pal
	if (pAlphaBuf) memcpy(Pal.Alpha, pAlphaBuf, 256);
	// success
	return TRUE;
	}

BOOL CStdDDraw::SubPrimaryClipper(int iX1, int iY1, int iX2, int iY2)
	{
	// Set sub primary clipper
	SetPrimaryClipper(Max(iX1,ClipX1),Max(iY1,ClipY1),Min(iX2,ClipX2),Min(iY2,ClipY2));
	return TRUE;
	}

BOOL CStdDDraw::StorePrimaryClipper()
	{
	// Store current primary clipper
	StClipX1=ClipX1; StClipY1=ClipY1; StClipX2=ClipX2; StClipY2=ClipY2;
	return TRUE;
	}

BOOL CStdDDraw::RestorePrimaryClipper()
	{
	// Restore primary clipper
	SetPrimaryClipper(StClipX1, StClipY1, StClipX2, StClipY2);
	return TRUE;
	}

BOOL CStdDDraw::SetPrimaryClipper(int iX1, int iY1, int iX2, int iY2)
	{
	// set clipper
	ClipX1=iX1; ClipY1=iY1; ClipX2=iX2; ClipY2=iY2;
	UpdateClipper();
	// Done	
	return TRUE;
	}

BOOL CStdDDraw::ApplyPrimaryClipper(SURFACE sfcSurface)
	{
	//sfcSurface->SetClipper(lpClipper);
	return TRUE;
	}

BOOL CStdDDraw::DetachPrimaryClipper(SURFACE sfcSurface)
	{
	//sfcSurface->SetClipper(NULL);
	return TRUE;
	}

BOOL CStdDDraw::NoPrimaryClipper()
	{
	// apply maximum clipper
	SetPrimaryClipper(0,0,439832,439832);
	// Done
	return TRUE;
	}


/*
bool CStdDDraw::ClipPoly(CBltData &rBltData)
	{
	const int  CPO_Left   = 1,
	           CPO_Top    = 2,
	           CPO_Right  = 4,
	           CPO_Bottom = 8,
	           CPO_MovedSingle = 16,
	           CPO_Remove      = 32,
	           CPO_BLeft       = 64,
	           CPO_BTop        = 128,
	           CPO_BRight      = 256,
	           CPO_BBottom     = 512,
	           CPO_Line1Out    = 1024,
	           CPO_Line2Out    = 2048;


	const int CPO_Outside = 15;
	const int CPO_Border  = 960;

	// calculate floating point clips for performance reasons; note the +1 because it's the beginning of the next pixel that lays outside
	float fClipX1=ClipX1, fClipY1=ClipY1, fClipX2=ClipX2+1, fClipY2=ClipY2+1;

	// first, decide which points lay outside the screen, and where they do so
	int OutEdges[8]; ZeroMemory(&OutEdges, sizeof(OutEdges)); int i,iCheckDir,iVtx2;
	int byAnyClip=0;

	for (i=0; i<rBltData.byNumVertices; ++i)
		{
		// check sides
		if (rBltData.vtVtx[i].ftx < fClipX1) OutEdges[i] = CPO_Left;
		if (rBltData.vtVtx[i].fty < fClipY1) OutEdges[i] |= CPO_Top;
		if (rBltData.vtVtx[i].ftx >= fClipX2) OutEdges[i] |= CPO_Right;
		if (rBltData.vtVtx[i].fty >= fClipY2) OutEdges[i] |= CPO_Bottom;
		byAnyClip |= OutEdges[i];
		}
	// no clips?
	if (!byAnyClip) return true;

	// now iterate through the edges again, and decide what to do
	for (i=0; i<rBltData.byNumVertices; ++i)
		if (OutEdges[i] & CPO_Outside)
			{

			// so this edge is outside the screen - process adjacent lines
			float fOldX = rBltData.vtVtx[i].ftx, fOldY = rBltData.vtVtx[i].fty, fNewX, fNewY;
			bool fVertexMoved=false;
			for (iCheckDir=-1; iCheckDir<2; iCheckDir+=2)
				{
				// set base for new point to be calculated
				fNewX=fOldX; fNewY=fOldY;

				// get adjacent vertex
				iVtx2 = (i+iCheckDir)%rBltData.byNumVertices;
				if (iVtx2<0) iVtx2+=rBltData.byNumVertices;

				// had the adjecent vertex moved in a way that it's no longer on the line?
				if (OutEdges[iVtx2] & (CPO_MovedSingle | CPO_Remove))
					{
					if (iCheckDir==1)
						{
						OutEdges[i] |= CPO_Line2Out;
						// if the next vertex has set this flag (i.e., the first, and this is the last),
						// the vertex removal check has to be done
						// which is simply: if it didn't move (out of the void...) it has to be removed
						if (!fVertexMoved) OutEdges[i] |= (CPO_Remove | CPO_MovedSingle);
						// the loops will end now
						}
					else
						OutEdges[i] |= CPO_Line1Out;
					// never process the line!
					continue;
					}

				// get line deltas
				float fdx = rBltData.vtVtx[iVtx2].ftx-fNewX;
				float fdy = rBltData.vtVtx[iVtx2].fty-fNewY;

				// get the crossing with the clip
				// first, the xdir...if any of the clauses succeeds, fdx cannot have been zero, so fdy/fdx is safe
				if (fNewX<fClipX1 && (fNewX+fdx)>=fClipX1)      { fNewY-=(fdy/fdx)*(fNewX-fClipX1); fNewX=fClipX1; }
				else if (fNewX>=fClipX2 && (fNewX+fdx)<fClipX2) { fNewY-=(fdy/fdx)*(fNewX-fClipX2); fNewX=fClipX2; }
				// now, ydir must regard that vertices, that the crossing might have happend at xdir first
				// so check if they're still out, and don't just copmpare the iOut-bits!
				if (fNewY < fClipY1 && (fNewY+fdy)>=fClipY1)    { fNewX-=(fdx/fdy)*(fNewY-fClipY1); fNewY=fClipY1; }
				else if (fNewY>=fClipY2 && (fNewY+fdy)<fClipY2) { fNewX-=(fdx/fdy)*(fNewY-fClipY2); fNewY=fClipY2; }

				// check if any valid crossing has been found
				// (it might have been bogus crossings outside the screen range)
				if (fNewX>=fClipX1 && fNewX<=fClipX2 && fNewY>=fClipY1 && fNewY<=fClipY2)
					{
					// crossing found: move vertex!

					// set border flags
					int iBorderFlag=0;
					if (fNewX == fClipX1) iBorderFlag |= CPO_BLeft;
					if (fNewY == fClipY1) iBorderFlag |= CPO_BTop;
					if (fNewX == fClipX2) iBorderFlag |= CPO_BRight;
					if (fNewY == fClipY2) iBorderFlag |= CPO_BBottom;

					// move the vertex only if not already done so...
					if (!fVertexMoved)
						{
						rBltData.vtVtx[i].ftx = fNewX;
						rBltData.vtVtx[i].fty = fNewY;
						fVertexMoved=true;
						OutEdges[i] |= iBorderFlag;
						}
					else
						{
						// ...otherwise, insert a new vertex
						// vertex position is always the one checked, because this routine is not called in the first (iCheckDir==-1) call
						// move old vertices down
						for (int j=rBltData.byNumVertices; j>iVtx2; --j)
							{
							// move vertices
							rBltData.vtVtx[j] = rBltData.vtVtx[j-1];
							// move out-edge-data
							OutEdges[j] = OutEdges[j-1];
							}
						// now insert new vertex
						rBltData.vtVtx[iVtx2].ftx = fNewX; rBltData.vtVtx[iVtx2].fty = fNewY;
						++rBltData.byNumVertices;
						OutEdges[iVtx2]=(OutEdges[i] & ~CPO_Outside) | iBorderFlag;
						// Since this has been the last direction, a jump of i can be done safely here
						// the jump skips the otherwise unnecessarily checked vertex iVtx2 (i.e., i+1)
						++i;
						}

					}
				else
					{
					// a crossing to the adjacent vertex could not be found - so the line becomes unnecessary
					if (iCheckDir==1)
						{
						if (fVertexMoved)
							{
							// the vertex has already moved (or was removed), processing the other line
							// simply mark it as not to be processed by the next vertex-movement
							OutEdges[i] |= CPO_MovedSingle;
							}
						else
							{
							// no adjacent line intersected: the vertex can really be removed
							// however, it must stay in place for now, so the next vertex can be checked correctly
							OutEdges[i] |= (CPO_Remove | CPO_MovedSingle);
							}
						// in any case, the second line lay outside
						OutEdges[i] |= CPO_Line2Out;
						}
					else
						{
						OutEdges[i] |= CPO_Line1Out;
						// normally, this can only happen to the first vertex, because this line is also processed
						// by the previous vertex, and either CPO_MovedSingle or CPO_Remove must have been set
						// - both causing these checks to be skipped completely - however, floating point inaccuracy
						// could lead here as well
						// mark the vertex to be removed - if a crossing is found by the next line, it has to insert
						// a new vertex, so set fVertexMoved to true
						OutEdges[i] |= CPO_Remove;
						fVertexMoved=true;
						}
					}

				// next direction
				}

			// next vertex
			}


	// now all vertices are either inside the screen, or CPO_Remove
	// but we might have lost screen edges

	// searching for lost edges, first get the last valid edge-vertex to begin with
	// searching backwards, so the following loop can iterate up to this vertex
	for (i=rBltData.byNumVertices-1; i>=0; --i)
		if (~OutEdges[i] & CPO_Remove)
			if (OutEdges[i] & CPO_Border)
				break;
	int iLastValidBrdVtx=i;
	// no vtx found?
	if (iLastValidBrdVtx<0)
		{
		// this can only mean two things: either the whole screen is covered, or nothing
		// well, better assume nothing for now...
		return false;
		// whole screen
		rBltData.byNumVertices = 4;
		rBltData.vtVtx[0].ftx = fClipX1; rBltData.vtVtx[0].fty = fClipY1;
		rBltData.vtVtx[1].ftx = fClipX2; rBltData.vtVtx[1].fty = fClipY1;
		rBltData.vtVtx[2].ftx = fClipX2; rBltData.vtVtx[2].fty = fClipY2;
		rBltData.vtVtx[3].ftx = fClipX1; rBltData.vtVtx[3].fty = fClipY2;
		return true;
		}


	// finally, remove dead edges
	int iLastValidVtx=0;
	for (i=0; i<rBltData.byNumVertices; ++i)
		if (~OutEdges[i] & CPO_Remove)
			{
			if (iLastValidVtx != i)
				rBltData.vtVtx[iLastValidVtx] = rBltData.vtVtx[i];
			iLastValidVtx++;
			}
	rBltData.byNumVertices = iLastValidVtx;


	// d'oh - now overlapped edges are lost; x and y-clips should be applied seperately
	// gwe5...


	// done, finished!
	return (rBltData.byNumVertices > 2);
	}
*/

bool CStdDDraw::ClipPoly(CBltData &rBltData)
	{
	// safety
	if (!rBltData.byNumVertices) return false;
	CBltTransform inverse;
	// Transform the points before clipping, if possible
	bool transformed = rBltData.pTransform && inverse.SetAsInv(*rBltData.pTransform);
	if (transformed) for (int i = 0; i < rBltData.byNumVertices; ++i)
		rBltData.pTransform->TransformPoint(rBltData.vtVtx[i].ftx, rBltData.vtVtx[i].fty);
	// calculate floating point clips for performance reasons; note the +1 because it's the beginning of the next pixel that lays outside
	float fClipX1=float(ClipX1), fClipY1=float(ClipY1), fClipX2=float(ClipX2)+1, fClipY2=float(ClipY2)+1;
	fClipX1 += DDrawCfg.fBlitOff; fClipY1 += DDrawCfg.fBlitOff;
	fClipX2 += DDrawCfg.fBlitOff; fClipY2 += DDrawCfg.fBlitOff;
	// clip against left
	if (!rBltData.ClipBy(-1, 0, -fClipX1)) return false;
	// clip against top
	if (!rBltData.ClipBy(0, -1, -fClipY1)) return false;
	// clip against right
	if (!rBltData.ClipBy(1, 0, fClipX2)) return false;
	// clip against bottom
	if (!rBltData.ClipBy(0, 1, fClipY2)) return false;
	// transform them back, so that they can be transformed again by PerformBlt
	if (transformed) for (int i = 0; i < rBltData.byNumVertices; ++i)
		inverse.TransformPoint(rBltData.vtVtx[i].ftx, rBltData.vtVtx[i].fty);
	// clipping done
	return true;
	}

void CStdDDraw::BlitLandscape(SURFACE sfcSource, SURFACE sfcSource2, SURFACE sfcSource3, int fx, int fy, 
								SURFACE sfcTarget, int tx, int ty, int wdt, int hgt)
	{
	Blit(sfcSource, float(fx), float(fy), float(wdt), float(hgt), sfcTarget, tx, ty, wdt, hgt, FALSE);
	}

void CStdDDraw::Blit8Fast(CSurface8 * sfcSource, int fx, int fy, 
								SURFACE sfcTarget, int tx, int ty, int wdt, int hgt)
	{
	// blit 8bit-sfc
	// lock surfaces
	assert(sfcTarget->fPrimary);
	bool fRender = sfcTarget->IsRenderTarget();
	if (!fRender) if (!sfcTarget->Lock())
		{ return; }
	// blit 8 bit pix
	for (int ycnt=0; ycnt<hgt; ++ycnt)
		for (int xcnt=0; xcnt<wdt; ++xcnt)
			{
			BYTE byPix = sfcSource->GetPix(fx+xcnt, fy+ycnt);
			if (byPix) DrawPixInt(sfcTarget,(float)(tx+xcnt), (float)(ty+ycnt), sfcSource->pPal->GetClr(byPix));
			}
	// unlock
	if (!fRender) sfcTarget->Unlock();
	}

BOOL CStdDDraw::Blit(SURFACE sfcSource, float fx, float fy, float fwdt, float fhgt, 
										 SURFACE sfcTarget, int tx, int ty, int twdt, int thgt, 
										 BOOL fSrcColKey, CBltTransform *pTransform)
	{
	// safety
	if (!sfcSource || !sfcTarget || !twdt || !thgt || !fwdt || !fhgt) return FALSE;
	// emulated blit? 
	if (!sfcTarget->IsRenderTarget())
		return Blit8(sfcSource, int(fx), int(fy), int(fwdt), int(fhgt), sfcTarget, tx, ty, twdt, thgt, fSrcColKey, pTransform);
	// calc stretch
	float scaleX=(float) twdt/fwdt;
	float scaleY=(float) thgt/fhgt;
	// bound
	if (ClipAll) return TRUE;
	// check exact
	bool fExact = !pTransform && fwdt==twdt && fhgt==thgt;
	// manual clipping? (primary surface only)
	if (DDrawCfg.ClipManuallyE && !pTransform && sfcTarget->fPrimary)
		if (true) // always clip! /*(scaleX < 10.0) && (scaleY < 10.0)*/)
			{
			int iOver;
			// Left
			iOver=tx-ClipX1; 
			if (iOver<0) 
				{ 
				twdt+=iOver; 
				fwdt+=((float)iOver/scaleX);
				fx-=((float)iOver/scaleX);
				tx=ClipX1; 
				}
			// Top
			iOver=ty-ClipY1; 
			if (iOver<0) 
				{ 
				thgt+=iOver; 
				fhgt+=((float)iOver/scaleY);
				fy-=((float)iOver/scaleY);
				ty=ClipY1; 
				}
			// Right
			iOver=ClipX2+1-(tx+twdt); 
			if (iOver<0) 
				{ 
				fwdt+=((float)iOver/scaleX);
				twdt+=iOver; 
				}
			// Bottom
			iOver=ClipY2+1-(ty+thgt); 
			if (iOver<0) 
				{ 
				fhgt+=((float)iOver/scaleY); 
				thgt+=iOver; 
				}
		}
	// inside screen?
	if (twdt<=0 || thgt<=0) return FALSE;
	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return FALSE;
	// texture present?
	if (!sfcSource->ppTex)
		{
		// primary surface?
		if (sfcSource->fPrimary)
			{
			// blit emulated
			return Blit8(sfcSource, int(fx), int(fy), int(fwdt), int(fhgt), sfcTarget, tx, ty, twdt, thgt, fSrcColKey);
			}
		return FALSE;
		}
	// create blitting struct
	CBltData BltData;
	// pass down pTransform
	BltData.pTransform=pTransform;
	// store current state
	StoreStateBlock();
	// blit with basesfc?
	bool fBaseSfc=false;
	if (sfcSource->pMainSfc) if (sfcSource->pMainSfc->ppTex) fBaseSfc=true;
	// set blitting state - done by PerformBlt
	// get involved texture offsets
	int iTexSize=sfcSource->iTexSize;
	int iTexX=Max(int(fx/iTexSize), 0);
	int iTexY=Max(int(fy/iTexSize), 0);
	int iTexX2=Min((int)(fx+fwdt-1)/iTexSize +1, sfcSource->iTexX);
	int iTexY2=Min((int)(fy+fhgt-1)/iTexSize +1, sfcSource->iTexY);
	// calc stretch regarding texture size and indent
	float scaleX2 = scaleX * (iTexSize+DDrawCfg.fTexIndent*2);
	float scaleY2 = scaleY * (iTexSize+DDrawCfg.fTexIndent*2);
	// blit from all these textures
	SetTexture();
	for (int iY=iTexY; iY<iTexY2; ++iY)
		{
		for (int iX=iTexX; iX<iTexX2; ++iX)
			{
			CTexRef *pTex = *(sfcSource->ppTex + iY * sfcSource->iTexX + iX);
			// get current blitting offset in texture (beforing any last-tex-size-changes)
			int iBlitX=iTexSize*iX;
			int iBlitY=iTexSize*iY;
			// size changed? recalc dependant, relevant (!) values
			if (iTexSize != pTex->iSize)
				{
				iTexSize = pTex->iSize;
				scaleX2 = scaleX * (iTexSize+DDrawCfg.fTexIndent*2);
				scaleY2 = scaleY * (iTexSize+DDrawCfg.fTexIndent*2);
				}
			// get new texture source bounds
			FLOAT_RECT fTexBlt;
			fTexBlt.left  = Max<float>(fx - iBlitX, 0);
			fTexBlt.top   = Max<float>(fy - iBlitY, 0);
			fTexBlt.right = Min<float>(fx + fwdt - (float)iBlitX, (float)iTexSize);
			fTexBlt.bottom= Min<float>(fy + fhgt - (float)iBlitY, (float)iTexSize);
			// get new dest bounds
			FLOAT_RECT tTexBlt;
			tTexBlt.left  = (fTexBlt.left  + iBlitX - fx) * scaleX + tx;
			tTexBlt.top   = (fTexBlt.top   + iBlitY - fy) * scaleY + ty;
			tTexBlt.right = (fTexBlt.right + iBlitX - fx) * scaleX + tx;
			tTexBlt.bottom= (fTexBlt.bottom+ iBlitY - fy) * scaleY + ty;
			// prepare blit data texture matrix
			// translate back to texture 0/0 regarding indent and blit offset
			/*BltData.TexPos.SetMoveScale(-tTexBlt.left - DDrawCfg.fBlitOff, -tTexBlt.top  - DDrawCfg.fBlitOff, 1, 1);
			// apply back scaling and texture-indent - simply scale matrix down
			int i;
			for (i=0; i<3; ++i) BltData.TexPos.mat[i] /= scaleX2;
			for (i=3; i<6; ++i) BltData.TexPos.mat[i] /= scaleY2;
			// now, finally, move in texture - this must be done last, so no stupid zoom is applied...
			BltData.TexPos.MoveScale(((float) fTexBlt.left + DDrawCfg.fTexIndent) / iTexSize,
				((float) fTexBlt.top  + DDrawCfg.fTexIndent) / iTexSize, 1, 1);*/
			// Set resulting matrix directly
			BltData.TexPos.SetMoveScale(
				(fTexBlt.left + DDrawCfg.fTexIndent) / iTexSize - (tTexBlt.left + DDrawCfg.fBlitOff) / scaleX2,
				(fTexBlt.top + DDrawCfg.fTexIndent) / iTexSize - (tTexBlt.top + DDrawCfg.fBlitOff) / scaleY2,
				1 / scaleX2,
				1 / scaleY2);
			// set up blit data as rect
			BltData.byNumVertices = 4;
			BltData.vtVtx[0].ftx = tTexBlt.left  + DDrawCfg.fBlitOff; BltData.vtVtx[0].fty = tTexBlt.top    + DDrawCfg.fBlitOff;
			BltData.vtVtx[1].ftx = tTexBlt.right + DDrawCfg.fBlitOff; BltData.vtVtx[1].fty = tTexBlt.top    + DDrawCfg.fBlitOff;
			BltData.vtVtx[2].ftx = tTexBlt.right + DDrawCfg.fBlitOff; BltData.vtVtx[2].fty = tTexBlt.bottom + DDrawCfg.fBlitOff;
			BltData.vtVtx[3].ftx = tTexBlt.left  + DDrawCfg.fBlitOff; BltData.vtVtx[3].fty = tTexBlt.bottom + DDrawCfg.fBlitOff;
			
			CTexRef * pBaseTex = pTex;
			// is there a base-surface to be blitted first?
			if (fBaseSfc)
				{
				// then get this surface as same offset as from other surface
				// assuming this is only valid as long as there's no texture management,
				// organizing partially used textures together!
				pBaseTex = *(sfcSource->pMainSfc->ppTex + iY * sfcSource->iTexX + iX);
				}
			// base blit
			PerformBlt(BltData, pBaseTex, BlitModulated ? BlitModulateClr : 0xffffff, !!(dwBlitMode & C4GFXBLIT_MOD2), fExact);
			// overlay
			if (fBaseSfc)
				{
				DWORD dwModClr = sfcSource->ClrByOwnerClr;
				// apply global modulation to overlay surfaces only if desired
				if (BlitModulated && !(dwBlitMode & C4GFXBLIT_CLRSFC_OWNCLR))
					ModulateClr(dwModClr, BlitModulateClr);
				PerformBlt(BltData, pTex, dwModClr, !!(dwBlitMode & C4GFXBLIT_CLRSFC_MOD2), fExact);
				}
			}
		}
	// reset texture
	ResetTexture();
	// restore state
	RestoreStateBlock();
	// success
	return TRUE;
	}

BOOL CStdDDraw::Blit8(SURFACE sfcSource, int fx, int fy, int fwdt, int fhgt, 
										 SURFACE sfcTarget, int tx, int ty, int twdt, int thgt, 
										 BOOL fSrcColKey, CBltTransform *pTransform)
	{
	if (!pTransform) return BlitRotate(sfcSource, fx, fy, fwdt, fhgt, sfcTarget, tx, ty, twdt, thgt, 0, fSrcColKey!=FALSE);
	// safety
	if (!fwdt || !fhgt) return TRUE;
	// Lock the surfaces
	if (!sfcSource->Lock())
		return FALSE;
	if (!sfcTarget->Lock())
		{ sfcSource->Unlock(); return FALSE; }
	// transformed, emulated blit
	// Calculate transform target rect
	CBltTransform Transform;
	Transform.SetMoveScale(tx-(float)fx*twdt/fwdt, ty-(float)fy*thgt/fhgt, (float) twdt/fwdt, (float) thgt/fhgt);
	Transform *=* pTransform;
	CBltTransform TransformBack;
	TransformBack.SetAsInv(Transform);
	float ttx0=(float)tx, tty0=(float)ty, ttx1=(float)(tx+twdt), tty1=(float)(ty+thgt);
	float ttx2=(float)ttx0, tty2=(float)tty1, ttx3=(float)ttx1, tty3=(float)tty0;
	pTransform->TransformPoint(ttx0, tty0);
	pTransform->TransformPoint(ttx1, tty1);
	pTransform->TransformPoint(ttx2, tty2);
	pTransform->TransformPoint(ttx3, tty3);
	int ttxMin = Max<int>((int)floor(Min(Min(ttx0, ttx1), Min(ttx2, ttx3))), 0);
	int ttxMax = Min<int>((int)ceil(Max(Max(ttx0, ttx1), Max(ttx2, ttx3))), sfcTarget->Wdt);
	int ttyMin = Max<int>((int)floor(Min(Min(tty0, tty1), Min(tty2, tty3))), 0);
	int ttyMax = Min<int>((int)ceil(Max(Max(tty0, tty1), Max(tty2, tty3))), sfcTarget->Hgt);
	// blit within target rect
	for (int y = ttyMin; y < ttyMax; ++y)
		for (int x = ttxMin; x < ttxMax; ++x)
			{
			float ffx=(float)x, ffy=(float)y;
			TransformBack.TransformPoint(ffx, ffy);
			int ifx=static_cast<int>(ffx), ify=static_cast<int>(ffy);
			if (ifx<fx || ify<fy || ifx>=fx+fwdt || ify>=fy+fhgt) continue;
			sfcTarget->BltPix(x,y, sfcSource, ifx,ify, !!fSrcColKey);
			}
	// Unlock the surfaces
	sfcSource->Unlock();
	sfcTarget->Unlock();
	return TRUE;
	}

BOOL CStdDDraw::BlitRotate(SURFACE sfcSource, int fx, int fy, int fwdt, int fhgt,
								SURFACE sfcTarget, int tx, int ty, int twdt, int thgt,
								int iAngle, bool fTransparency)
	{
	// rendertarget?
	if (sfcTarget->IsRenderTarget())
		{
		CBltTransform rot;
		rot.SetRotate(iAngle, (float) (tx+tx+twdt)/2, (float) (ty+ty+thgt)/2);
		return Blit(sfcSource, float(fx), float(fy), float(fwdt), float(fhgt), sfcTarget, tx, ty, twdt, thgt, TRUE, &rot);
		}
	const double pi=3.1415926535;
  // Object is first stretched to dest rect, then rotated at place.
  int xcnt,ycnt,fcx,fcy,tcx,tcy,cpcx,cpcy;
  int npcx,npcy;
  double mtx[4],dang;
  if (!fwdt || !fhgt || !twdt || !thgt) return FALSE;
  // Lock the surfaces
  if (!sfcSource->Lock())
    return FALSE;
  if (!sfcTarget->Lock())
    { sfcSource->Unlock(); return FALSE; }
  // Rectangle centers
  fcx=fwdt/2; fcy=fhgt/2;
  tcx=twdt/2; tcy=thgt/2;
	// Adjust angle range
	while (iAngle<0) iAngle+=36000; while (iAngle>35999) iAngle-=36000;
	// Exact/free rotation
	switch (iAngle)
		{
		case 0:	
			for (ycnt=0; ycnt<thgt; ycnt++)
				if (Inside(cpcy=ty+tcy-thgt/2+ycnt,0,sfcTarget->Hgt-1))
					for (xcnt=0; xcnt<twdt; xcnt++)
						if (Inside(cpcx=tx+tcx-twdt/2+xcnt,0,sfcTarget->Wdt-1))
							sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt*fwdt/twdt+fx, ycnt*fhgt/thgt+fy, fTransparency);
			break;

		case 9000:
			for (ycnt=0; ycnt<thgt; ycnt++)
				if (Inside(cpcx=ty+tcy+thgt/2-ycnt,0,sfcTarget->Wdt-1))
					for (xcnt=0; xcnt<twdt; xcnt++)
						if (Inside(cpcy=tx+tcx-twdt/2+xcnt,0,sfcTarget->Hgt-1))
							sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt*fwdt/twdt+fx, ycnt*fhgt/thgt+fy, fTransparency);
			break;

		case 18000:
			for (ycnt=0; ycnt<thgt; ycnt++)
				if (Inside(cpcy=ty+tcy+thgt/2-ycnt,0,sfcTarget->Hgt-1))
					for (xcnt=0; xcnt<twdt; xcnt++)
						if (Inside(cpcx=tx+tcx+twdt/2-xcnt,0,sfcTarget->Wdt-1))
							sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt*fwdt/twdt+fx, ycnt*fhgt/thgt+fy, fTransparency);
			break;

		case 27000:
			for (ycnt=0; ycnt<thgt; ycnt++)
				if (Inside(cpcx=ty+tcy-thgt/2+ycnt,0,sfcTarget->Wdt-1))
					for (xcnt=0; xcnt<twdt; xcnt++)
						if (Inside(cpcy=tx+tcx+twdt/2-xcnt,0,sfcTarget->Hgt-1))
							sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt*fwdt/twdt+fx, ycnt*fhgt/thgt+fy, fTransparency);
			break;

		default:
			// Calculate rotation matrix
			dang=pi*iAngle/18000.0;
			mtx[0]=cos(dang); mtx[1]=-sin(dang);
			mtx[2]=sin(dang); mtx[3]= cos(dang);  
			// Blit source rect
			for (ycnt=0; ycnt<fhgt; ycnt++)
				{
				// Source line start
				for (xcnt=0; xcnt<fwdt; xcnt++)
					{
					// Current pixel coordinate as from source
					cpcx=xcnt-fcx; cpcy=ycnt-fcy;
					// Convert to coordinate as in dest
					cpcx=cpcx*twdt/fwdt; cpcy=cpcy*thgt/fhgt;
					// Rotate current pixel coordinate
					npcx= (int) ( mtx[0]*cpcx + mtx[1]*cpcy );
					npcy= (int) ( mtx[2]*cpcx + mtx[3]*cpcy );
					// Place in dest
					sfcTarget->BltPix(tx+tcx+npcx, ty+tcy+npcy, sfcSource, xcnt+fx, ycnt+fy, fTransparency);
					sfcTarget->BltPix(tx+tcx+npcx+1, ty+tcy+npcy, sfcSource, xcnt+fx, ycnt+fy, fTransparency);
					}
				}  
			break;
		}

  // Unlock the surfaces
	sfcSource->Unlock();
	sfcTarget->Unlock();
  return TRUE;
	}

#ifdef C4ENGINE

bool CStdDDraw::Error(const char *szMsg)
	{
	sLastError.Copy(szMsg);
	Log(szMsg); return false;
	}

#else

bool CStdDDraw::Error(const char *szMsg)
	{
	sLastError.Copy(szMsg);
	return false;
	}

#endif

BOOL CStdDDraw::CreatePrimaryClipper()
	{
	// simply setup primary viewport
	SetPrimaryClipper(0, 0, pApp->ScreenWidth()-1, pApp->ScreenHeight()-1);
	StClipX1=ClipX1; StClipY1=ClipY1; StClipX2=ClipX2; StClipY2=ClipY2;
	return TRUE;
	}

BOOL CStdDDraw::AttachPrimaryPalette(SURFACE sfcSurface)
	{/*
	if (sfcSurface->SetPaletteEntries(0, lpPalette)!=DD_OK) return FALSE;*/
	return TRUE;
	}

BOOL CStdDDraw::BlitSurface(SURFACE sfcSurface, SURFACE sfcTarget, int tx, int ty, bool fBlitBase)
	{
	if (fBlitBase)
		{
		Blit(sfcSurface, 0.0f, 0.0f, (float)sfcSurface->Wdt, (float)sfcSurface->Hgt, sfcTarget, tx, ty, sfcSurface->Wdt, sfcSurface->Hgt, false);
		return TRUE;
		}
	else
		{
		if (!sfcSurface) return FALSE;
		CSurface *pSfcBase = sfcSurface->pMainSfc;
		sfcSurface->pMainSfc = NULL;
		Blit(sfcSurface, 0.0f, 0.0f, (float)sfcSurface->Wdt, (float)sfcSurface->Hgt, sfcTarget, tx, ty, sfcSurface->Wdt, sfcSurface->Hgt, false);
		sfcSurface->pMainSfc = pSfcBase;
		return TRUE;
		}
  }

BOOL CStdDDraw::BlitSurfaceTile(SURFACE sfcSurface, SURFACE sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX, int iOffsetY, BOOL fSrcColKey)
  {
  int iSourceWdt,iSourceHgt,iX,iY,iBlitX,iBlitY,iBlitWdt,iBlitHgt;
	// Get source surface size
  if (!GetSurfaceSize(sfcSurface,iSourceWdt,iSourceHgt)) return FALSE;
	// reduce offset to needed size
	iOffsetX %= iSourceWdt;
	iOffsetY %= iSourceHgt;
	// Vertical blits
  for (iY=iToY+iOffsetY; iY<iToY+iToHgt; iY+=iSourceHgt)
		{
		// Vertical blit size
		iBlitY=Max(iToY-iY,0); iBlitHgt=Min(iSourceHgt,iToY+iToHgt-iY)-iBlitY;
		// Horizontal blits
		for (iX=iToX+iOffsetX; iX<iToX+iToWdt; iX+=iSourceWdt)
			{
			// Horizontal blit size
			iBlitX=Max(iToX-iX,0); iBlitWdt=Min(iSourceWdt,iToX+iToWdt-iX)-iBlitX;
			// Blit
			if (!Blit(sfcSurface,float(iBlitX),float(iBlitY),float(iBlitWdt),float(iBlitHgt),sfcTarget,iX+iBlitX,iY+iBlitY,iBlitWdt,iBlitHgt,fSrcColKey)) return FALSE;
			}
		}
  return TRUE;
  }

BOOL CStdDDraw::BlitSurfaceTile2(SURFACE sfcSurface, SURFACE sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX, int iOffsetY, BOOL fSrcColKey)
	{
	// if it's a render target, simply blit with repeating texture
	// repeating textures, however, aren't currently supported
	/*if (sfcTarget->IsRenderTarget())
		return Blit(sfcSurface, iOffsetX, iOffsetY, iToWdt, iToHgt, sfcTarget, iToX, iToY, iToWdt, iToHgt, FALSE);*/
  int tx,ty,iBlitX,iBlitY,iBlitWdt,iBlitHgt;
	// get tile size
	int iTileWdt=sfcSurface->Wdt;
	int iTileHgt=sfcSurface->Hgt;
	// adjust size of offsets
	iOffsetX%=iTileWdt;
	iOffsetY%=iTileHgt;
	if (iOffsetX<0) iOffsetX+=iTileWdt;
	if (iOffsetY<0) iOffsetY+=iTileHgt;
	// get start pos for blitting
	int iStartX=iToX-iOffsetX;
	int iStartY=iToY-iOffsetY;
	ty=0;
	// blit vertical
	for (int iY=iStartY; ty<iToHgt; iY+=iTileHgt)
		{
		// get vertical blit bounds
		iBlitY=0; iBlitHgt=iTileHgt;
		if (iY<iToY) { iBlitY=iToY-iY; iBlitHgt+=iY-iToY; }
		int iOver=ty+iBlitHgt-iToHgt; if (iOver>0) iBlitHgt-=iOver;
		// blit horizontal
		tx=0;
		for (int iX=iStartX; tx<iToWdt; iX+=iTileWdt)
			{
			// get horizontal blit bounds
			iBlitX=0; iBlitWdt=iTileWdt;
			if (iX<iToX) { iBlitX=iToX-iX; iBlitWdt+=iX-iToX; }
			iOver=tx+iBlitWdt-iToWdt; if (iOver>0) iBlitWdt-=iOver;
			// blit
			if (!Blit(sfcSurface,float(iBlitX),float(iBlitY),float(iBlitWdt),float(iBlitHgt),sfcTarget,tx+iToX,ty+iToY,iBlitWdt,iBlitHgt,fSrcColKey)) return FALSE;
			// next col
			tx+=iBlitWdt;
			}
		// next line
		ty+=iBlitHgt;
		}
	// success
	return TRUE;
	}

BOOL CStdDDraw::TextOut(const char *szText, CStdFont &rFont, float fZoom, SURFACE sfcDest, int iTx, int iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup)
	{
	CMarkup Markup(true);
	static char szLinebuf[2500+1];
	for (int cnt=0; SCopySegmentEx(szText,cnt,szLinebuf,fDoMarkup ? '|' : '\n','\n',2500); cnt++,iTy+=int(fZoom*rFont.iLineHgt))
		if (!StringOut(szLinebuf,sfcDest,iTx,iTy,dwFCol,byForm,fDoMarkup,Markup,&rFont,fZoom)) return FALSE;
	return TRUE;
	}

BOOL CStdDDraw::StringOut(const char *szText, CStdFont &rFont, float fZoom, SURFACE sfcDest, int iTx, int iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup)
	{
	// init markup
	CMarkup Markup(true);
	// output string
	return StringOut(szText, sfcDest, iTx, iTy, dwFCol, byForm, fDoMarkup, Markup, &rFont, fZoom);
	}

bool CStdDDraw::StringOut(const char *szText, SURFACE sfcDest, int iTx, int iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup, CMarkup &Markup, CStdFont *pFont, float fZoom)
	{
	// clip
	if (ClipAll) return true;
	// safety
	if (!PrepareRendering(sfcDest)) return false;
	// convert align
	int iFlags=0;
	switch (byForm)
		{
		case ACenter: iFlags |= STDFONT_CENTERED; break;
		case ARight: iFlags |= STDFONT_RIGHTALGN; break;
		}
	if (!fDoMarkup) iFlags|=STDFONT_NOMARKUP;
	// draw text
#ifdef C4ENGINE
	pFont->DrawText(sfcDest, iTx  , iTy  , dwFCol, szText, iFlags, Markup, fZoom);
#endif
	// done, success
	return true;
	}

void CStdDDraw::DrawPix(SURFACE sfcDest, float tx, float ty, DWORD dwClr)
	{
	// manual clipping?
	if (DDrawCfg.ClipManuallyE)
		{
		if (tx < ClipX1) { return; }
		if (ty < ClipY1) { return; }
		if (ClipX2 < tx) { return; }
		if (ClipY2 < ty) { return; }
		}
	// apply global modulation
	ClrByCurrentBlitMod(dwClr);
	// apply modulation map
	if (fUseClrModMap)
		ModulateClr(dwClr, pClrModMap->GetModAt((int)tx, (int)ty));
	// Draw
	DrawPixInt(sfcDest, tx, ty, dwClr);
	}

void CStdDDraw::DrawBox(SURFACE sfcDest, int iX1, int iY1, int iX2, int iY2, BYTE byCol)
	{
	// get color
	DWORD dwClr=Pal.GetClr(byCol);
	// offscreen emulation?
	if (!sfcDest->IsRenderTarget())
		{
		int iSfcWdt=sfcDest->Wdt,iSfcHgt=sfcDest->Hgt,xcnt,ycnt;
		// Lock surface
		if (!sfcDest->Lock()) return;
		// Outside of surface/clip
		if ((iX2<Max(0,ClipX1)) || (iX1>Min(iSfcWdt-1,ClipX2)) 
			|| (iY2<Max(0,ClipY1)) || (iY1>Min(iSfcHgt-1,ClipY2))) 
			{ sfcDest->Unlock(); return; }
		// Clip to surface/clip
		if (iX1<Max(0,ClipX1)) iX1=Max(0,ClipX1); if (iX2>Min(iSfcWdt-1,ClipX2)) iX2=Min(iSfcWdt-1,ClipX2); 
		if (iY1<Max(0,ClipY1)) iY1=Max(0,ClipY1); if (iY2>Min(iSfcHgt-1,ClipY2)) iY2=Min(iSfcHgt-1,ClipY2);
		// Set lines
		for (ycnt=iY2-iY1; ycnt>=0; ycnt--)
			for (xcnt=iX2-iX1; xcnt>=0; xcnt--)
				sfcDest->SetPixDw(iX1+xcnt, iY1+ycnt, dwClr);
		// Unlock surface
		sfcDest->Unlock();
		}
	// draw as primitives
	DrawBoxDw(sfcDest, iX1, iY1, iX2, iY2, dwClr);
  }

void CStdDDraw::DrawHorizontalLine(SURFACE sfcDest, int x1, int x2, int y, BYTE col)
	{
	// if this is a render target: draw it as a box
	if (sfcDest->IsRenderTarget())
		{
		DrawLine(sfcDest, x1, y, x2, y, col);
		return;
		}
	int lWdt=sfcDest->Wdt,lHgt=sfcDest->Hgt,xcnt;
	// Lock surface
	if (!sfcDest->Lock()) return;
	// Fix coordinates
	if (x1>x2) Swap(x1,x2);
	// Determine clip
	int clpx1,clpx2,clpy1,clpy2;
	clpx1=Max(0,ClipX1); clpy1=Max(0,ClipY1);
	clpx2=Min(lWdt-1,ClipX2); clpy2=Min(lHgt-1,ClipY2);
	// Outside clip check
	if ((x2<clpx1) || (x1>clpx2) || (y<clpy1) || (y>clpy2)) 
		{ sfcDest->Unlock(); return; }
	// Clip to clip
	if (x1<clpx1) x1=clpx1; if (x2>clpx2) x2=clpx2; 
	// Set line
	for (xcnt=x2-x1; xcnt>=0; xcnt--) sfcDest->SetPix(x1+xcnt, y, col);
	// Unlock surface
	sfcDest->Unlock();
	}

void CStdDDraw::DrawVerticalLine(SURFACE sfcDest, int x, int y1, int y2, BYTE col)
	{
	// if this is a render target: draw it as a box
	if (sfcDest->IsRenderTarget())
		{
		DrawLine(sfcDest, x, y1, x, y2, col);
		return;
		}
	int lWdt=sfcDest->Wdt,lHgt=sfcDest->Hgt,ycnt;
	// Lock surface
	if (!sfcDest->Lock()) return;
	// Fix coordinates
	if (y1>y2) Swap(y1,y2);
	// Determine clip
	int clpx1,clpx2,clpy1,clpy2;
	clpx1=Max(0,ClipX1); clpy1=Max(0,ClipY1);
	clpx2=Min(lWdt-1,ClipX2); clpy2=Min(lHgt-1,ClipY2);
	// Outside clip check
	if ((x<clpx1) || (x>clpx2) || (y2<clpy1) || (y1>clpy2)) 
		{ sfcDest->Unlock(); return; }
	// Clip to clip
	if (y1<clpy1) y1=clpy1; if (y2>clpy2) y2=clpy2; 
	// Set line
	for (ycnt=y1; ycnt<=y2; ycnt++) sfcDest->SetPix(x, ycnt, col);
	// Unlock surface
	sfcDest->Unlock();
	}

void CStdDDraw::DrawFrame(SURFACE sfcDest, int x1, int y1, int x2, int y2, BYTE col)
  {
  DrawHorizontalLine(sfcDest,x1,x2,y1,col);
  DrawHorizontalLine(sfcDest,x1,x2,y2,col);
  DrawVerticalLine(sfcDest,x1,y1,y2,col);
  DrawVerticalLine(sfcDest,x2,y1,y2,col);
  }

void CStdDDraw::DrawFrameDw(SURFACE sfcDest, int x1, int y1, int x2, int y2, DWORD dwClr) // make these parameters float...?
  {
	DrawLineDw(sfcDest,(float)x1,(float)y1,(float)x2,(float)y1, dwClr);
	DrawLineDw(sfcDest,(float)x2,(float)y1,(float)x2,(float)y2, dwClr);
	DrawLineDw(sfcDest,(float)x2,(float)y2,(float)x1,(float)y2, dwClr);
	DrawLineDw(sfcDest,(float)x1,(float)y2,(float)x1,(float)y1, dwClr);
  }

// Globally locked surface variables - for DrawLine callback crap

CSurface *GLSBuffer=NULL;

bool LockSurfaceGlobal(SURFACE sfcTarget)
  {
  if (GLSBuffer) return false;
  GLSBuffer=sfcTarget;
	return !!sfcTarget->Lock();
  }

bool UnLockSurfaceGlobal(SURFACE sfcTarget)
  {
  if (!GLSBuffer) return false;
	sfcTarget->Unlock();
  GLSBuffer=NULL;
  return true;
  } 

bool DLineSPix(int32_t x, int32_t y, int32_t col)
  {
  if (!GLSBuffer) return false;
  GLSBuffer->SetPix(x,y,col);
  return true;
  }

bool DLineSPixDw(int32_t x, int32_t y, int32_t dwClr)
  {
  if (!GLSBuffer) return false;
  GLSBuffer->SetPixDw(x,y,(DWORD) dwClr);
  return true;
  }

void CStdDDraw::DrawPatternedCircle(SURFACE sfcDest, int x, int y, int r, BYTE col, CPattern & Pattern1, CPattern & Pattern2, CStdPalette &rPal)
	{
	if (!sfcDest->Lock()) return;
	for (int ycnt = -r; ycnt < r; ycnt++)
		{
		int lwdt = (int) sqrt(float(r * r - ycnt * ycnt));
		// Set line
		for (int xcnt = x - lwdt; xcnt < x + lwdt; ++xcnt)
			{
			// apply both patterns 
			DWORD dwClr=rPal.GetClr(col);
			Pattern1.PatternClr(xcnt, y + ycnt, col, dwClr, rPal);
			Pattern2.PatternClr(xcnt, y + ycnt, col, dwClr, rPal);
			sfcDest->SetPixDw(xcnt, y + ycnt, dwClr);
			}
		}
	sfcDest->Unlock();
	}

void CStdDDraw::SurfaceAllowColor(SURFACE sfcSfc, DWORD *pdwColors, int iNumColors, BOOL fAllowZero)
	{
	// safety
	if (!sfcSfc) return;
	if (!pdwColors || !iNumColors) return;
	// change colors
	int xcnt,ycnt,wdt=sfcSfc->Wdt,hgt=sfcSfc->Hgt;
	// Lock surface
	if (!sfcSfc->Lock()) return;
	for (ycnt=0; ycnt<hgt; ycnt++)
		{
		for (xcnt=0; xcnt<wdt; xcnt++)
			{
			DWORD dwColor = sfcSfc->GetPixDw(xcnt,ycnt,false);
			BYTE px = 0;
			for (int i = 0; i < 256; ++i)
				if (lpDDrawPal->GetClr(i) == dwColor)
				{
				px = i; break;
				}
			if (fAllowZero)
				{
				if (!px) continue;
				--px;
				}
			sfcSfc->SetPixDw(xcnt, ycnt, pdwColors[px % iNumColors]);
			}
		}
	sfcSfc->Unlock();
	}

void CStdDDraw::Grayscale(SURFACE sfcSfc, int32_t iOffset)
	{
	// safety
	if (!sfcSfc) return;
	// change colors
	int xcnt,ycnt,wdt=sfcSfc->Wdt,hgt=sfcSfc->Hgt;
	// Lock surface
	if (!sfcSfc->Lock()) return;
	for (ycnt=0; ycnt<hgt; ycnt++)
		{
		for (xcnt=0; xcnt<wdt; xcnt++)
			{
			DWORD dwColor = sfcSfc->GetPixDw(xcnt,ycnt,false);
			uint32_t r = GetRValue(dwColor), g = GetGValue(dwColor), b = GetBValue(dwColor), a = dwColor >> 24;
			int32_t gray = BoundBy<int32_t>((r + g + b) / 3 + iOffset, 0, 255);
			sfcSfc->SetPixDw(xcnt, ycnt, RGBA(gray, gray, gray, a));
			}
		}
	sfcSfc->Unlock();
	}

BOOL CStdDDraw::GetPrimaryClipper(int &rX1, int &rY1, int &rX2, int &rY2)
	{
	// Store drawing clip values
	rX1=ClipX1; rY1=ClipY1;	rX2=ClipX2; rY2=ClipY2;
	// Done	
	return TRUE;
	}

void CStdDDraw::SetGamma(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3)
	{
	// calc ramp
	Gamma.Set(dwClr1, dwClr2, dwClr3, Gamma.size, &DefRamp);
	// set it
	ApplyGammaRamp(Gamma, false);
	}

void CStdDDraw::DisableGamma()
	{
	// set it
	ApplyGammaRamp(DefRamp, true);
	}

void CStdDDraw::EnableGamma()
	{
	// set it
	ApplyGammaRamp(Gamma, false);
	}

DWORD CStdDDraw::ApplyGammaTo(DWORD dwClr)
	{
	return Gamma.ApplyTo(dwClr);
	}

CStdDDraw *DDrawInit(CStdApp * pApp, BOOL Fullscreen, BOOL fUsePageLock, int iBitDepth, int Engine, unsigned int iMonitor)
	{
	// create engine
	switch (iGfxEngine = Engine)
		{
		default: // Use the first engine possible if none selected
#ifdef USE_DIRECTX
		case GFXENGN_DIRECTX: lpDDraw = new CStdD3D(false); break;
		case GFXENGN_DIRECTXS: lpDDraw = new CStdD3D(true); break;
#endif
#ifdef USE_GL
		case GFXENGN_OPENGL: lpDDraw = new CStdGL(); break;
#endif
		case GFXENGN_NOGFX: lpDDraw = new CStdNoGfx(); break;
		}
	if (!lpDDraw) return NULL;
	// init it
	if (!lpDDraw->Init(pApp, Fullscreen, fUsePageLock, iBitDepth, iMonitor))
		{
		delete lpDDraw;
		return NULL;
		}
	// done, success
	return lpDDraw;
	}

bool CStdDDraw::Init(CStdApp * pApp, BOOL Fullscreen, BOOL fUsePageLock, int iBitDepth, unsigned int iMonitor)
	{
	// set cfg again, as engine has been decided
	DDrawCfg.Set(DDrawCfg.Cfg, DDrawCfg.fTexIndent, DDrawCfg.fBlitOff);

	this->pApp = pApp;

	// store default gamma
	SaveDefaultGammaRamp(pApp->pWindow);
	
	DebugLog("Init DX");
	DebugLog("  Create DirectDraw...");

	if (!CreateDirectDraw())
		return Error("  CreateDirectDraw failure.");

	// set monitor info (Monitor-var and target rect)
	DebugLog("  SetOutput adapter...");
	if (!SetOutputAdapter(iMonitor))
		return Error("  Output adapter failure.");

	DebugLog("  Create Device...");

	if (!CreatePrimarySurfaces(Fullscreen, iBitDepth, iMonitor))
		return Error("  CreateDevice failure.");

	DebugLog("  Create Clipper");

	if (!CreatePrimaryClipper())
		return Error("  Clipper failure.");

	fFullscreen = Fullscreen;

	return TRUE;
	}

void CStdDDraw::DrawBoxFade(SURFACE sfcDest, int iX, int iY, int iWdt, int iHgt, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, int iBoxOffX, int iBoxOffY)
	{
	// clipping not performed - this fn should be called for clipped rects only
	// apply modulation map: Must sectionize blit
	if (fUseClrModMap)
		{
		int iModResX = pClrModMap ? pClrModMap->GetResolutionX() : CClrModAddMap::iDefResolutionX;
		int iModResY = pClrModMap ? pClrModMap->GetResolutionY() : CClrModAddMap::iDefResolutionY;
		iBoxOffX %= iModResX;
		iBoxOffY %= iModResY;
		if (iWdt+iBoxOffX > iModResX || iHgt+iBoxOffY > iModResY)
			{
			if (iWdt<=0 || iHgt<=0) return;
			CColorFadeMatrix clrs(iX, iY, iWdt, iHgt, dwClr1, dwClr2, dwClr3, dwClr4);
			int iMaxH = iModResY - iBoxOffY;
			int w,h;
			for (int y = iY, H = iHgt; H > 0; (y += h), (H -= h), (iMaxH = iModResY))
				{
				h = Min<int>(H, iMaxH);
				int iMaxW = iModResX - iBoxOffX;
				for (int x = iX, W = iWdt; W > 0; (x += w), (W -= w), (iMaxW = iModResX))
					{
					w = Min<int>(W, iMaxW);
					DrawBoxFade(sfcDest, x,y,w,h, clrs.GetColorAt(x,y), clrs.GetColorAt(x+w,y), clrs.GetColorAt(x,y+h), clrs.GetColorAt(x+w,y+h), 0,0);
					}
				}
			return;
			}
		}
	// set vertex buffer data
	// vertex order:
	// 0=upper left   dwClr1
	// 1=lower left   dwClr3
	// 2=lower right  dwClr4
	// 3=upper right  dwClr2
	int vtx[8];
	vtx[0] = iX     ; vtx[1] = iY;
	vtx[2] = iX     ; vtx[3] = iY+iHgt;
	vtx[4] = iX+iWdt; vtx[5] = iY+iHgt;
	vtx[6] = iX+iWdt; vtx[7] = iY;
	DrawQuadDw(sfcDest, vtx, dwClr1, dwClr3, dwClr4, dwClr2);
	}

void CStdDDraw::DrawBoxDw(SURFACE sfcDest, int iX1, int iY1, int iX2, int iY2, DWORD dwClr)
	{
	// manual clipping?
	if (DDrawCfg.ClipManuallyE)
		{
		int iOver;
		iOver=iX1-ClipX1; if (iOver<0) { iX1=ClipX1; }
		iOver=iY1-ClipY1; if (iOver<0) { iY1=ClipY1; }
		iOver=ClipX2-iX2; if (iOver<0) { iX2+=iOver; }
		iOver=ClipY2-iY2; if (iOver<0) { iY2+=iOver; }
		// inside screen?
		if (iX2<iX1 || iY2<iY1) return;
		}
	DrawBoxFade(sfcDest, iX1, iY1, iX2-iX1+1, iY2-iY1+1, dwClr, dwClr, dwClr, dwClr, 0,0);
	}
