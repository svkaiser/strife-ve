//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2014 Night Dive Studios, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:  heads-up text and input code
//


#include <ctype.h>

#include "doomdef.h"
#include "doomkeys.h"

#include "v_video.h"
#include "i_swap.h"

#include "hu_lib.h"
#include "r_local.h"
#include "r_draw.h"
#include "hu_stuff.h" // [STRIFE]

#include "z_zone.h"
#include "m_qstring.h" // [SVE]
#include "doomstat.h"
#include "st_stuff.h"
#include "m_misc.h"
#include "w_wad.h"
#include "i_social.h"

// [SVE]: for player names
#include "net_client.h"

// boolean : whether the screen is always erased
#define noterased viewwindowx

extern boolean	automapactive;	// in AM_map.c

extern boolean D_PatchClipCallback(patch_t *patch, int x, int y); // [STRIFE]

//
// HUlib_drawYellowText
//
// haleyjd 20100918: [STRIFE] New function.
//
void HUlib_drawYellowText(int x, int y, const char *text, boolean shadowed)
{
    int start_x = x;
    const char *rover = text;
    char c;

    while((c = *rover++))
    {
        if(c == '\n')
        {
            x = start_x;
            y += 12;
            continue;
        }

        // haleyjd 20110213: found MORE code ignored/misinterpreted by Hex-Rays:
        // Underscores are replaced by spaces.
        if(c == '_')
            c = ' ';
        else if (c == ' ' && x == start_x) // skip spaces at the start of a line
            continue;

        c = toupper(c) - HU_FONTSTART;

        if(c >= 0 && c < HU_FONTSIZE)
        {
            patch_t *patch = shadowed ? ffont[(int) c] : yfont[(int) c]; // [SVE]
            int      width = SHORT(patch->width);

            if(x + width <= (SCREENWIDTH - 20))
            {
                // haleyjd: STRIFE-TODO: bit different than the exe... for now
                if(!D_PatchClipCallback(patch, x + SHORT(patch->leftoffset),
                    y + SHORT(patch->topoffset)))
                    return;
                V_DrawPatchDirect(x, y, patch);
                x = x + width;
            }
            else
            {
                x = start_x;
                --rover;
                y += 12;
            }
        }
        else
        {
            x += 4;
        }
    }
}

//
// HUlib_yellowTextWidth
//
// haleyjd 20141009: [SVE]
// NB: Only works if the string doesn't get wrapped.
//
int HUlib_yellowTextWidth(const char *text)
{
    int width = 0;
    int widestwidth = 0;
    const char *rover = text;
    char c;

    while((c = *rover++))
    {
        if(c == '\n')
        {
            if(width > widestwidth)
                widestwidth = width;
            width = 0;
            continue;
        }

        if(c == '_')
            c = ' ';

        c = toupper(c) - HU_FONTSTART;

        if(c >= 0 && c < HU_FONTSIZE)
        {
            patch_t *patch = yfont[(int) c];
            width += SHORT(patch->width);
        }
        else
        {
            width += 4;
        }
    }
    if(width > widestwidth)
        widestwidth = width;

    return widestwidth;
}

//
// HUlib_init
//
// [STRIFE] Verified unmodified.
//
void HUlib_init(void)
{
}

//
// HUlib_clearTextLine
// 
// [STRIFE] Verified unmodified.
//
void HUlib_clearTextLine(hu_textline_t* t)
{
    t->len = 0;
    t->l[0] = 0;
    t->needsupdate = true;
}

//
// HUlib_initTextLine
//
// [STRIFE] Verified unmodified
//
void
HUlib_initTextLine
( hu_textline_t*        t,
  int                   x,
  int                   y,
  patch_t**             f,
  int                   sc )
{
    t->x = x;
    t->y = y;
    t->f = f;
    t->sc = sc;
    HUlib_clearTextLine(t);
}

//
// HUlib_addCharToTextLine
//
// [STRIFE] Verified unmodified.
//
boolean
HUlib_addCharToTextLine
( hu_textline_t*        t,
  char                  ch )
{
    if (t->len == HU_MAXLINELENGTH)
        return false;
    else
    {
        t->l[t->len++] = ch;
        t->l[t->len] = 0;
        t->needsupdate = 4;
        return true;
    }
}

//
// HUlib_delCharFromTextLine
//
// [STRIFE] Verified unmodified.
//
boolean HUlib_delCharFromTextLine(hu_textline_t* t)
{
    if (!t->len) return false;
    else
    {
        t->l[--t->len] = 0;
        t->needsupdate = 4;
        return true;
    }
}

//
// HUlib_drawTextLine
//
// haleyjd 09/18/10: [STRIFE] Modified to not draw underscores in text.
// [SVE] svillarreal - longtext param added
//
void HUlib_drawTextLine(hu_textline_t* l, boolean drawcursor, boolean longtext)
{
    int                 i;
    int                 w;
    int                 x;
    unsigned char       c;
    int                 start;

    // draw the new stuff
    x = l->x;
    start = 0;

    // [SVE] svillarreal - if text is going off the screen, then pick a position
    // to offset the text by
    if(longtext)
    {
        for(i = 0; i < l->len; i++)
        {
            c = toupper(l->l[i]);
            if(c != ' ' && c >= l->sc && c < '_') // [STRIFE]: Underscores excluded
            {
                w = SHORT(l->f[c - l->sc]->width);
                if(x+w > SCREENWIDTH)
                {
                    start = l->len - i;
                    break;
                }

                x += w;
            }
            else
            {
                x += 4;

                if(x >= SCREENWIDTH)
                {
                    start = l->len - i;
                    break;
                }
            }
        }

        x = l->x;
    }

    for(i = start; i < l->len; i++)
    {
        c = toupper(l->l[i]);
        if(c != ' ' && c >= l->sc && c < '_') // [STRIFE]: Underscores excluded
        {
            w = SHORT(l->f[c - l->sc]->width);
            if(x+w > SCREENWIDTH)
                break;

            V_DrawPatchDirect(x, l->y, l->f[c - l->sc]);
            x += w;
        }
        else
        {
            x += 4;

            if(x >= SCREENWIDTH)
                break;
        }
    }

    // draw the cursor if requested
    if(drawcursor && x + SHORT(l->f['_' - l->sc]->width) <= SCREENWIDTH)
    {
        V_DrawPatchDirect(x, l->y, l->f['_' - l->sc]);
    }
}

//
// HUlib_eraseTextLine
//
// sorta called by HU_Erase and just better darn get things straight
// 
// [STRIFE] Verified unmodified.
//
void HUlib_eraseTextLine(hu_textline_t* l)
{
    int                 lh;
    int                 y;
    int                 yoffset;

    // Only erases when NOT in automap and the screen is reduced,
    // and the text must either need updating or refreshing
    // (because of a recent change back from the automap)

    if (!automapactive &&
        viewwindowx && l->needsupdate)
    {
        lh = SHORT(l->f[0]->height) + 1;
        for (y=l->y,yoffset=y*SCREENWIDTH ; y<l->y+lh ; y++,yoffset+=SCREENWIDTH)
        {
            if (y < viewwindowy || y >= viewwindowy + viewheight)
                R_VideoErase(yoffset, SCREENWIDTH); // erase entire line
            else
            {
                R_VideoErase(yoffset, viewwindowx); // erase left border
                R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
                // erase right border
            }
        }
    }

    if (l->needsupdate) l->needsupdate--;
}

//
// HUlib_initSText
//
// [STRIFE] Verified unmodified.
//
void
HUlib_initSText
( hu_stext_t*   s,
  int           x,
  int           y,
  int           h,
  patch_t**     font,
  int           startchar,
  boolean*      on )
{
    int i;

    s->h = h;
    s->on = on;
    s->laston = true;
    s->cl = 0;
    for (i=0;i<h;i++)
    {
        HUlib_initTextLine(&s->l[i],
                           x, y - i*(SHORT(font[0]->height)+1),
                           font, startchar);
    }
}

//
// HUlib_addLineToSText
//
// [STRIFE] Verified unmodified.
//
void HUlib_addLineToSText(hu_stext_t* s)
{
    int i;

    // add a clear line
    if (++s->cl == s->h)
        s->cl = 0;
    HUlib_clearTextLine(&s->l[s->cl]);

    // everything needs updating
    for (i=0 ; i<s->h ; i++)
        s->l[i].needsupdate = 4;
}

//
// HUlib_addMessageToSText
//
// [STRIFE] Verified unmodified.
//
void
HUlib_addMessageToSText
( hu_stext_t*	s,
  const char*		prefix,
  const char*		msg )
{
    HUlib_addLineToSText(s);
    if (prefix)
        while (*prefix)
            HUlib_addCharToTextLine(&s->l[s->cl], *(prefix++));

    while (*msg)
        HUlib_addCharToTextLine(&s->l[s->cl], *(msg++));
}

//
// HUlib_drawSText
//
// [STRIFE] Verified unmodified.
//
void HUlib_drawSText(hu_stext_t* s)
{
    int i, idx;
    hu_textline_t *l;

    if (!*s->on)
        return; // if not on, don't draw

    // draw everything
    for (i=0 ; i<s->h ; i++)
    {
        idx = s->cl - i;
        if (idx < 0)
            idx += s->h; // handle queue of lines

        l = &s->l[idx];

        // need a decision made here on whether to skip the draw
        HUlib_drawTextLine(l, false, false); // no cursor, please
    }
}

//
// HUlib_eraseSText
//
// [STRIFE] Verified unmodified.
//
void HUlib_eraseSText(hu_stext_t* s)
{
    int i;

    for (i=0 ; i<s->h ; i++)
    {
        if (s->laston && !*s->on)
            s->l[i].needsupdate = 4;
        HUlib_eraseTextLine(&s->l[i]);
    }
    s->laston = *s->on;
}

//
// HUlib_initIText
//
// [STRIFE] Verified unmodified.
//
void
HUlib_initIText
( hu_itext_t*   it,
  int           x,
  int           y,
  patch_t**     font,
  int           startchar,
  boolean*      on )
{
    it->lm = 0; // default left margin is start of text
    it->on = on;
    it->laston = true;
    HUlib_initTextLine(&it->l, x, y, font, startchar);
}


// The following deletion routines adhere to the left margin restriction
// [STRIFE] Verified unmodified.
void HUlib_delCharFromIText(hu_itext_t* it)
{
    if (it->l.len != it->lm)
        HUlib_delCharFromTextLine(&it->l);
}

// [STRIFE] Verified unmodified.
void HUlib_eraseLineFromIText(hu_itext_t* it)
{
    while (it->lm != it->l.len)
        HUlib_delCharFromTextLine(&it->l);
}

// Resets left margin as well
// [STRIFE] Verified unmodified.
void HUlib_resetIText(hu_itext_t* it)
{
    it->lm = 0;
    HUlib_clearTextLine(&it->l);
}

//
// HUlib_addPrefixToIText
//
// [STRIFE] Verified unmodified.
//
void
HUlib_addPrefixToIText
( hu_itext_t*   it,
  char*         str )
{
    while (*str)
        HUlib_addCharToTextLine(&it->l, *(str++));
    it->lm = it->l.len;
}

// wrapper function for handling general keyed input.
// returns true if it ate the key
// [STRIFE] Verified unmodified.
boolean
HUlib_keyInIText
( hu_itext_t*	it,
  unsigned char ch )
{
    ch = toupper(ch);

    if (ch >= ' ' && ch <= '_') 
        HUlib_addCharToTextLine(&it->l, (char) ch);
    else if (ch == KEY_BACKSPACE || ch == KEY_DEL) // [SVE] 20141213: due to Steamworks bug
        HUlib_delCharFromIText(it);
    else if (ch != KEY_ENTER) 
        return false; // did not eat key

    return true; // ate the key
}

//
// HUlib_drawIText
//
// [STRIFE] Verified unmodified.
//
void HUlib_drawIText(hu_itext_t* it)
{
    hu_textline_t *l = &it->l;

    if (!*it->on)
        return;
    HUlib_drawTextLine(l, true, true); // draw the line w/ cursor
}

//
// HUlib_eraseIText
//
// [STRIFE] Verified unmodified.
//
void HUlib_eraseIText(hu_itext_t* it)
{
    if (it->laston && !*it->on)
        it->l.needsupdate = 4;
    HUlib_eraseTextLine(&it->l);
    it->laston = *it->on;
}

//
// HUlib_makePrettyPlayerName
//
// haleyjd 20141025: [SVE] Make a pretty player name from the ugly one that has
// a ": " sequence appended to it. You must destroy the returned character
// buffer when you're done with it.
//
char *HUlib_makePrettyPlayerName(int playerNum)
{
    qstring_t qstr;
    size_t colonpos;

    QStrInitCreate(&qstr);
    QStrCopy(&qstr, player_names[playerNum]);
    if((colonpos = QStrFindFirstOfChar(&qstr, ':')) != qstring_npos)
        QStrTruncate(&qstr, colonpos);

    return QStrBuffer(&qstr);
}

static void HUlib_drawYellowNum(int x, int y, int num)
{
    int numdigits = 3;
    int w = 8; 
    int neg = num < 0;

    if(neg)
    {
        if(num < -99)
            num = -99;

        num = -num;
    }

    // in the special case of 0, you draw 0
    if(!num)
        V_DrawPatch(x - w, y, ffont['0' - HU_FONTSTART]);

    // draw the new number
    while(num && numdigits--)
    {
        x -= w;
        V_DrawPatch(x, y, ffont[(num % 10 + '0') - HU_FONTSTART]);
        num /= 10;
    }

    // draw a minus sign if necessary
    if(neg)
        V_DrawPatch(x - 8, y, ffont['-' - HU_FONTSTART]);
}

//
// HUlib_drawFrags
//
// haleyjd 20141025: [SVE] Show a frags chart when a player dies.
//
void HUlib_drawFrags(void)
{
    char *names[MAXPLAYERS+2];
    int i, x, y;
    int widest_name = 0;

    V_WriteBigText("FRAGS", (SCREENWIDTH - V_BigFontStringWidth("FRAGS"))/2, 24);

    for(i = 0; i < MAXPLAYERS; i++)
    {
        int namewidth;

        if(!playeringame[i])
            continue;

        names[i]  = HUlib_makePrettyPlayerName(i);
        if((namewidth = HUlib_yellowTextWidth(names[i])) > widest_name)
            widest_name = namewidth;
    }
    if(capturethechalice)
    {
        int w;
        names[8] = "Blue Team Score";
        names[9] = "Red Team Score";

        if((w = HUlib_yellowTextWidth(names[8])) > widest_name)
            widest_name = w;
    }

    x = (SCREENWIDTH - widest_name) / 2 - 24;
    y = 48;

    for(i = 0; i < MAXPLAYERS; i++)
    {
        char patch[9];

        if(!playeringame[i])
            continue;
     
        if(capturethechalice)
        {
            if(players[i].allegiance == CTC_TEAM_BLUE)
                M_StringCopy(patch, "STCOLOR8", sizeof(patch));
            else
                M_StringCopy(patch, "STCOLOR2", sizeof(patch));
        }
        else
            M_snprintf(patch, sizeof(patch), "STCOLOR%d", i+1);

        V_DrawPatch(x, y, W_CacheLumpName(patch, PU_CACHE));
        HUlib_drawYellowText(x+10, y+1, names[i], true);
        HUlib_drawYellowNum(x+10+widest_name+8*4, y+1, ST_calcFrags(i));

        y += 12;
        Z_Free(names[i]);
    }
    if(capturethechalice)
    {
        y += 12;
        V_DrawPatch(x, y, W_CacheLumpName("STCOLOR8", PU_CACHE));
        HUlib_drawYellowText(x+10, y+1, names[8], true);
        HUlib_drawYellowNum(x+10+widest_name+8*4, y+1, ctcbluescore);
        y += 12;
        V_DrawPatch(x, y, W_CacheLumpName("STCOLOR2", PU_CACHE));
        HUlib_drawYellowText(x+10, y+1, names[9], true);
        HUlib_drawYellowNum(x+10+widest_name+8*4, y+1, ctcredscore);
    }
}

// EOF

