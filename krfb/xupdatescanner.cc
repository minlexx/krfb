/*
 *  Copyright (C) 2000 heXoNet Support GmbH, D-66424 Homburg.
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */
/*
 * December 15th 2001: removed coments, mouse pointer options and some
 * other stuff
 * January 10th 2002: improved hint creation (join adjacent hints)
 * February 20th 2002: shrink tiles to match actual changes
 * 
 *                   Tim Jansen <tim@tjansen.de>
 */   

#include <kdebug.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <string.h>
#include <assert.h>

#include "xupdatescanner.h"

int scanlines[32] = {  0, 16,  8, 24,
		       4, 20, 12, 28,
		       10, 26, 18,  2,
		       22,  6, 30, 14,
		       1, 17,  9, 25,
		       7, 23, 15, 31,
		       19,  3, 27, 11,
		       29, 13,  5, 21 };
#define MAX_ADJ_TOLERANCE 8

#define abs(x) (((x) >= 0) ? (x) : -(x))


XUpdateScanner::XUpdateScanner(Display *_dpy,
			       Window _window,
			       unsigned char *_fb,
			       int _width,
			       int _height,
			       int _bitsPerPixel,
			       int _bytesPerLine,
			       unsigned int _tileWidth,
			       unsigned int _tileHeight) : 
	dpy(_dpy),
	window(_window),
	fb(_fb),
	width(_width),
	height(_height),
	bitsPerPixel(_bitsPerPixel),
	bytesPerLine(_bytesPerLine),
	tileWidth(_tileWidth),
	tileHeight(_tileHeight),
	count (0),
	scanline(NULL),
	tile(NULL)
{
	tile = XShmCreateImage(dpy,
			       DefaultVisual( dpy, 0 ),
			       bitsPerPixel,
			       ZPixmap,
			       NULL,
			       &shminfo_tile,
			       tileWidth,
			       tileHeight);

	shminfo_tile.shmid = shmget(IPC_PRIVATE,
				    tile->bytes_per_line * tile->height,
				    IPC_CREAT | 0777);
	shminfo_tile.shmaddr = tile->data = (char *) 
		shmat(shminfo_tile.shmid, 0, 0);
	shminfo_tile.readOnly = False;

	XShmAttach(dpy, &shminfo_tile);

	tilesX = (width + tileWidth - 1) / tileWidth;
	tilesY = (height + tileHeight - 1) / tileHeight;
	tileMap = new bool[tilesX * tilesY];
        tileRegionMap = new struct TileChangeRegion[tilesX * tilesY];

	for (int i = 0; i < tilesX * tilesY; i++) 
		tileMap[i] = false;

	scanline = XShmCreateImage(dpy,
				   DefaultVisual(dpy, 0),
				   bitsPerPixel,
				   ZPixmap,
				   NULL,
				   &shminfo_scanline,
				   width,
				   1);

	shminfo_scanline.shmid = shmget(IPC_PRIVATE,
					scanline->bytes_per_line,
					IPC_CREAT | 0777);
	shminfo_scanline.shmaddr = scanline->data = (char *) 
		shmat( shminfo_scanline.shmid, 0, 0 );
	shminfo_scanline.readOnly = False;

	XShmAttach(dpy, &shminfo_scanline);
};


XUpdateScanner::~XUpdateScanner()
{
	XShmDetach(dpy, &shminfo_scanline);
	XDestroyImage(scanline);
	shmdt(shminfo_scanline.shmaddr);
	shmctl(shminfo_scanline.shmid, IPC_RMID, 0);
	delete tileMap;
        delete tileRegionMap;
	XShmDetach(dpy, &shminfo_tile);
	XDestroyImage(tile);
	shmdt(shminfo_tile.shmaddr);
	shmctl(shminfo_tile.shmid, IPC_RMID, 0);
}

// check whether at the given line there is a right half in the tile
bool XUpdateScanner::checkRight(unsigned char *src,
				unsigned char *dest,
				int halfWidthBytes) 
{
	return memcmp(dest + halfWidthBytes, 
		      src + halfWidthBytes, 
		      halfWidthBytes) != 0;
}

// check whether the half exists in the tile
bool XUpdateScanner::checkSide(unsigned char *src,
			       unsigned char *dest,
			       int halfWidthBytes,
			       int n) 
{
	for (int i = 0; i < n; i++) {
		if (memcmp(dest + halfWidthBytes, 
			   src + halfWidthBytes, 
			   halfWidthBytes))
			return true;
		src += tile->bytes_per_line;
		dest += bytesPerLine;
        }
	return false;
}

// finds the first changed line in the tile
int XUpdateScanner::findFirstLine(int maxHeight, 
				  unsigned char *&ssrc,
				  unsigned char *&sdest,
				  int halfWidthBytes,
				  bool &left) {
	int firstLine = maxHeight;
	left = true;
	for (int line = 0; line < maxHeight; line++) {
		if (memcmp(sdest, ssrc, halfWidthBytes)) {
			firstLine = line;
			break;
		}
		if (memcmp(sdest + halfWidthBytes, 
			   ssrc + halfWidthBytes, 
			   halfWidthBytes)) {
			firstLine = line;
			left = false;
			break;
		}
		ssrc += tile->bytes_per_line;
		sdest += bytesPerLine;
        }
	return firstLine;
}

// finds the last changed line in the tile
int XUpdateScanner::findLastLine(int maxHeight, 
				 int firstLine,
				 unsigned char *msrc,
				 unsigned char *mdest,
				 int halfWidthBytes,
				 bool &left) {
        int lastLine = firstLine;
	left = true;

        for (int line = maxHeight-1; line > firstLine; line--) {
		msrc -= tile->bytes_per_line;
		mdest -= bytesPerLine;
		if (memcmp(mdest, msrc, halfWidthBytes)) {
			lastLine = line;
			break;
		}
		if (memcmp(mdest + halfWidthBytes,
			   msrc + halfWidthBytes, 
			   halfWidthBytes)) {
			lastLine = line;
			left = false;
			break;
		}
        }
	return lastLine;
}
				   

bool XUpdateScanner::copyTile(int x, int y, int tx, int ty)
{
	int maxWidth = width - x;
	int maxHeight = height - y;
	if (maxWidth > tileWidth) 
		maxWidth = tileWidth;
	if (maxHeight > tileHeight) 
		maxHeight = tileHeight;

	if ((maxWidth == tileWidth) && (maxHeight == tileHeight)) {
		XShmGetImage(dpy, window, tile, x, y, AllPlanes);
	} else {
		XGetSubImage(dpy, window, x, y, maxWidth, maxHeight, 
			     AllPlanes, ZPixmap, tile, 0, 0);
	}

	int pixelsize = bitsPerPixel >> 3;
	int halfWidthBytes = (maxWidth*pixelsize)>>1;
	bool hitLeft = true;
	bool leftConfirmed = false;
	bool rightConfirmed = false;
	unsigned char *src = (unsigned char*) tile->data;
	unsigned char *dest = fb + y * bytesPerLine + x * pixelsize;

        unsigned char *ssrc = src;
        unsigned char *sdest = dest;
	// search first changed line
        int firstLine = findFirstLine(maxHeight, ssrc, sdest, 
				      halfWidthBytes, hitLeft);
	
	// if no changes, leave
        if (firstLine == maxHeight) {
		tileMap[tx + ty * tilesX] = false;
		return false;
        }

	// try to get more information whether left&right half both changed
	if (hitLeft) {
		leftConfirmed = true;
		rightConfirmed = checkRight(ssrc, sdest, halfWidthBytes);
	}
	else
		rightConfirmed = true;
	
        unsigned char *msrc = src + (tile->bytes_per_line * maxHeight);
        unsigned char *mdest = dest + (bytesPerLine * maxHeight);
	// find the last line in the tile
        int lastLine = findLastLine(maxHeight, firstLine, msrc, mdest,
				    halfWidthBytes, hitLeft);
	// try to get more information whether left&right half both changed
	if (hitLeft) {
		leftConfirmed = true;
		if (!rightConfirmed)
			rightConfirmed = checkRight(ssrc, sdest, halfWidthBytes);
	}
	else
		rightConfirmed = true;

	// now complete information about right&left half
	if (!leftConfirmed)
		leftConfirmed = checkSide(sdest, ssrc, halfWidthBytes, 
					  lastLine-firstLine+1);
	if (!rightConfirmed)
		rightConfirmed = checkSide(sdest+halfWidthBytes, 
					  ssrc+halfWidthBytes,
					  halfWidthBytes, 
					  lastLine-firstLine+1);

	// identify what halfes to copy
	int cw;
	if (leftConfirmed && rightConfirmed)
		cw = maxWidth * pixelsize;
	else {
		cw = halfWidthBytes;
		if (rightConfirmed) {
			sdest += halfWidthBytes;
			ssrc += halfWidthBytes;
		}
		else
			assert(leftConfirmed);
	}

	// copy the stuff into the framebuffer
	for (int line = firstLine; line <= lastLine; line++) {
                memcpy(sdest, ssrc, cw);
                ssrc += tile->bytes_per_line;
		sdest += bytesPerLine;
	}

	// set the descriptor
        struct TileChangeRegion *r = &tileRegionMap[tx + (ty * tilesX)];
        r->firstLine = firstLine;
        r->lastLine = lastLine;
	r->leftSide = leftConfirmed;
	r->rightSide = rightConfirmed;
	
        return lastLine == (maxHeight-1);
}

void XUpdateScanner::copyAllTiles()
{
	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
                        if (tileMap[x + y * tilesX])
                                if (copyTile(x*tileWidth, y*tileHeight, x, y) &&
                                    ((y+1) < tilesY))
					tileMap[x + (y+1) * tilesX] = true;
		}
	}
	
}

void XUpdateScanner::createHintFromTile(int x, int y, int th, Hint &hint, 
					struct TileChangeRegion *r)
{
	int tw = tileWidth;
	if (r->leftSide ^ r->rightSide) {
		tw = tileWidth>>1;
		if (r->rightSide)
			x += tw;
	}

	int w = width - x;
	int h = height - y;
	if (w > tw)
		w = tw;
	if (h > th) 
		h = th;
	
	hint.x = x;
	hint.y = y;
	hint.w = w;
	hint.h = h;
}

// adds a tile to the right of the given existing tile
bool XUpdateScanner::addTileToHint(int x, int y, int wInTiles, 
				   int th, Hint &hint,
				   struct TileChangeRegion *r)
{
	// tiles are always added to the right, so if this one has no left half..
	if (!r->leftSide)
		return false;
	// or the previous one no right half, leave
	struct TileChangeRegion *prev = r-1;
	if (!prev->rightSide)
		return false;

	// half width for tiles without right half
	int tw = tileWidth;
	if (!r->rightSide)
		tw = tileWidth>>1;

	int w = width - x;
	int h = height - y;
	if (w > tw) 
		w = tw;
	if (h > th) 
		h = th;

	if ((wInTiles*abs(hint.y-y)) > MAX_ADJ_TOLERANCE)
		return false;
	if ((wInTiles*abs((hint.y+hint.h)-(y+h))) > MAX_ADJ_TOLERANCE)
		return false;

	if (hint.x > x) {
		hint.w += hint.x - x;
		hint.x = x;
	}

	if (hint.y > y) {
		hint.h += hint.y - y;
		hint.y = y;
	}

	if ((hint.x+hint.w) < (x+w)) {
		hint.w = (x+w) - hint.x;
	}

	if ((hint.y+hint.h) < (y+h)) {
		hint.h = (y+h) - hint.y;
	}
	return true;
}

// examines and possibly joins tiles under the hint with the hint
void XUpdateScanner::extendHintY(int x, int y, int x0, Hint &hint) 
{
	int eh = 0;
	int lastLine = -1;
	int w = x - x0 + 1;
	bool leftHalf = !tileRegionMap[x0 + y * tilesX].leftSide;
	bool rightHalf = !tileRegionMap[x - 1 + y * tilesX].rightSide;

	for (int i = y+1; i < tilesY; i++) {
		bool lk = true;
		int ll = 0;
		if (leftHalf && tileRegionMap[x0 + i * tilesX].leftSide)
			break;
	        if (rightHalf && tileRegionMap[x - 1 + i * tilesX].rightSide)
			break;
		for (int j = x0; j < x; j++) {
			int idx = j + i * tilesX;
			if ((!tileMap[idx]) ||
			    (tileRegionMap[idx].firstLine*w > MAX_ADJ_TOLERANCE)) {
				lk = false;
				break;
			}
			if (tileRegionMap[idx].lastLine > ll)
				ll = tileRegionMap[idx].lastLine;
		}
		if (!lk)
			break;
		
		for (int j = x0; j < x; j++) 
			tileMap[j + i * tilesX] = false;

		lastLine = ll;
		eh++;
		if ((ll*w) > MAX_ADJ_TOLERANCE)
			break;
	}

	if (eh == 0)
		return;

	hint.h += (eh-1) * tileHeight;
	hint.h += lastLine + 1;
	if ((hint.y + hint.h) > height)
		hint.h = height - hint.y;
}

static void printStatistics(Hint &hint) {
	static int snum = 0;
	static float ssum = 0.0;

	int oX0 = hint.x & 0xffffffe0;
	int oY0 = hint.y & 0xffffffe0;
	int oX2 = (hint.x+hint.w) & 0x1f;
	int oY2 = (hint.y+hint.h) & 0x1f;
	int oX3 = (((hint.x+hint.w) | 0x1f) + ((oX2 == 0) ? 0 : 1)) & 0xffffffe0;
	int oY3 = (((hint.y+hint.h) | 0x1f) + ((oY2 == 0) ? 0 : 1)) & 0xffffffe0;
	float s0 = hint.w*hint.h;
	float s1 = (oX3-oX0)*(oY3-oY0);
	float p = (100*s0/s1);
	ssum += p;
	snum++;
	float avg = ssum / snum;
	kdDebug() << "avg size saved: "<< avg <<"%"<<endl;
}

// takes the hint, calls extendHintY if possible and then puts hint into list
void XUpdateScanner::flushHint(int x, int y, int &x0, 
			       Hint &hint, QPtrList<Hint> &hintList)
{
	if (x0 < 0)
		return;

	int w = x - x0 + 1;
	int th = (hint.y + hint.h) % tileHeight;
	if ((th == 0) ||
	    ((31-th)*w < MAX_ADJ_TOLERANCE))
		extendHintY(x, y, x0, hint);
	x0 = -1;

	assert (hint.w > 0);
	assert (hint.h > 0);

//printStatistics(hint);
kdDebug() << "size: "<< hint.w << "/"<<hint.h<<endl;

	hintList.append(new Hint(hint));
}

// creates a list of hints
void XUpdateScanner::createHints(QPtrList<Hint> &hintList)
{
	Hint hint;
	int x0 = -1;

	for (int y = 0; y < tilesY; y++) {
		int x;
		for (x = 0; x < tilesX; x++) {
			int idx = x + y * tilesX;
			if (!tileMap[idx]) {
				flushHint(x, y, x0, hint, hintList);
				continue;
			}
			struct TileChangeRegion *r = &tileRegionMap[idx];
			int ty = r->firstLine;
			int th = r->lastLine - ty +1;
			if (x0 < 0) {
				createHintFromTile(x * tileWidth, 
						   (y * tileHeight) + ty, 
						   th, hint, r);
				x0 = x;
				continue;
			}
			if (!addTileToHint(x * tileWidth, 
					   (y * tileHeight) + ty, 
					   x0 - x + 1,
					   th, hint, r)) {
				flushHint(x, y, x0, hint, hintList);
				createHintFromTile(x * tileWidth, 
						   (y * tileHeight) + ty, 
						   th, hint, r);
				x0 = x;
				
			}
		}
		flushHint(x, y, x0, hint, hintList);
	}
}

void XUpdateScanner::searchUpdates(QPtrList<Hint> &hintList)
{
	count++;
	count %= 32;

	int i;
	int x, y;

	for (i = 0; i < (tilesX * tilesY); i++) {
		tileMap[i] = false;
	}

	y = scanlines[count];
	while (y < height) {
		XShmGetImage(dpy, window, scanline, 0, y, AllPlanes);
		x = 0;
		while (x < width) {
			int pixelsize = bitsPerPixel >> 3;
			unsigned char *src = (unsigned char*) scanline->data + 
				x * pixelsize;
			unsigned char *dest = fb + 
				y * bytesPerLine + x * pixelsize;
			int w = (x + 32) > width ? (width-x) : 32;
			if (memcmp(dest, src, w * pixelsize)) 
				tileMap[(x / tileWidth) + 
					(y / tileHeight) * tilesX] = true;
			x += 32;
		}
		y += 32;
	}

	copyAllTiles();

	createHints(hintList);
}


