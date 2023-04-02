/* Emacs style mode select   -*- C -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      Refresh/render internal state variables (global).
 *
 *-----------------------------------------------------------------------------*/


#ifndef __R_STATE__
#define __R_STATE__

// Need data structure definitions.
#include "d_player.h"
#include "r_data.h"

//
// Refresh internal data structures,
//  for rendering.
//

// needed for texture pegging
extern fixed_t *textureheight;

extern int firstflat, numflats;

// for global animation
extern int *flattranslation;
extern int *texturetranslation;

// Sprite....
extern int firstspritelump;
extern int lastspritelump;
extern int numspritelumps;

//
// Lookup tables for map data.
//
extern spritedef_t      *sprites;

extern int              numvertexes;
extern vertex_t         *vertexes;

extern int              numsegs;
extern seg_t            *segs;
extern seg_data_t       *segs_data;

extern int              numsectors;
extern sector_t         *sectors;

extern int              numsubsectors;
extern subsector_t      *subsectors;

extern int              numnodes;
extern node_t           *nodes;

extern int              numlines;
extern line_t           *lines;

extern int              numsides;
extern side_t           *sides;

extern int              *sslines_indexes;
extern ssline_t         *sslines;

extern byte             *map_subsectors;

//
// POV data.
//
extern fixed_t          viewx;
extern fixed_t          viewy;
extern fixed_t          viewz;
extern angle_t          viewangle;
extern player_t         *viewplayer;
extern angle_t          clipangle;
extern int              viewangletox[FINEANGLES/2];

// e6y: resolution limitation is removed
extern angle_t          *xtoviewangle;  // killough 2/8/98

extern int              FieldOfView;

extern fixed_t          rw_distance;
extern angle_t          rw_normalangle;

// angle to line origin
extern int              rw_angle1;

extern visplane_t       *floorplane;
extern visplane_t       *ceilingplane;


inline sector_t* SEG_BACK(seg_t* seg) { return sectors + seg->back; };
inline sector_t* SEG_BACK_NULL(seg_t* seg) { return seg->back == NO_SECTOR ? 0 : sectors + seg->back; };
inline void SEG_SET_BACK(seg_t* seg, sector_t* sector) { seg->back = sector - sectors; };
inline void SEG_SET_NOBACK(seg_t* seg) { seg->back = NO_SECTOR; };
inline dboolean SEG_HAS_BACK(seg_t* seg) { return seg->back != NO_SECTOR; };

inline sector_t* SEG_FRONT(seg_t* seg) { return sectors + seg->front; };
inline void SEG_SET_FRONT(seg_t* seg, sector_t* sector) { seg->front = sector - sectors; };
inline void SEG_SET_NOFRONT(seg_t* seg) { seg->front = NO_SECTOR; };
inline dboolean SEG_HAS_FRONT(seg_t* seg) { return seg->front != NO_SECTOR; };

inline line_t* SEG_LINE(seg_t* seg) { return lines + seg->line; };
inline line_t* SEG_LINE_NULL(seg_t* seg) { return seg->line == NO_LINE ? 0 : lines + seg->line; };
inline void SEG_SET_LINE(seg_t* seg, line_t* line) { seg->line = line - lines; };
inline void SEG_SET_NOLINE(seg_t* seg) { seg->line = NO_LINE; };
inline dboolean SEG_HAS_LINE(seg_t* seg) { return seg->line != NO_LINE; };

inline side_t* SEG_SIDE(seg_t* seg) { return sides + seg->side; };
inline side_t* SEG_SIDE_NULL(seg_t* seg) { return seg->side == NO_SIDE ? 0 : sides + seg->side; };
inline void SEG_SET_SIDE(seg_t* seg, side_t* side) { seg->side = side - sides; };
inline void SEG_SET_NOSIDE(seg_t* seg) { seg->side = NO_SIDE; };
inline dboolean SEG_HAS_SIDE(seg_t* seg) { return seg->side != NO_SIDE; };

inline vertex_t* SEG_V1(seg_t* seg) { return vertexes + seg->v1i; };
inline void SEG_SET_V1(seg_t* seg, vertex_t* vertex) { seg->v1i = vertex - vertexes; };

inline vertex_t* SEG_V2(seg_t* seg) { return vertexes + seg->v2i; };
inline void SEG_SET_V2(seg_t* seg, vertex_t* vertex) { seg->v2i = vertex - vertexes; };

#endif
