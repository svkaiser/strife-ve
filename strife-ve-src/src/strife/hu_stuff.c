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
// DESCRIPTION:  Heads-up displays
//


#include <ctype.h>

#include "doomdef.h"
#include "doomkeys.h"

#include "z_zone.h"

#include "deh_main.h"
#include "i_swap.h"
#include "i_video.h"

#include "hu_stuff.h"
#include "hu_lib.h"
#include "m_controls.h"
#include "m_menu.h"
#include "m_misc.h"
#include "w_wad.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

// [SVE]
#include "i_social.h"
#include "net_client.h"
#include "p_spec.h"

//
// Locally used constants, shortcuts.
//
#define HU_TITLE            (mapnames[gamemap-1])
#define HU_TITLEHEIGHT      1
#define HU_TITLEX           0

// haleyjd 09/01/10: [STRIFE] 167 -> 160 to move up level name
#define HU_TITLEY           (160 - SHORT(hu_font[0]->height))

#define HU_INPUTTOGGLE      't'
#define HU_INPUTX           HU_MSGX
#define HU_INPUTY           (HU_MSGY + HU_MSGHEIGHT*(SHORT(hu_font[0]->height) +1))
#define HU_INPUTWIDTH       64
#define HU_INPUTHEIGHT      1

#define HU_NOTIFICATIONY    160

char *chat_macros[10] =
{
    HUSTR_CHATMACRO0,
    HUSTR_CHATMACRO1,
    HUSTR_CHATMACRO2,
    HUSTR_CHATMACRO3,
    HUSTR_CHATMACRO4,
    HUSTR_CHATMACRO5,
    HUSTR_CHATMACRO6,
    HUSTR_CHATMACRO7,
    HUSTR_CHATMACRO8,
    HUSTR_CHATMACRO9
};

char                    chat_char; // remove later.
static player_t*        plr;
patch_t*                hu_font[HU_FONTSIZE];
patch_t*                yfont[HU_FONTSIZE];   // haleyjd 09/18/10: [STRIFE]
patch_t*                ffont[HU_FONTSIZE];   // haleyjd 20141204: [SVE]
static hu_textline_t    w_title;
boolean                 chat_on;
static hu_itext_t       w_chat;
static boolean          always_off = false;
static char             chat_dest[MAXPLAYERS];
static hu_itext_t       w_inputbuffer[MAXPLAYERS];

static boolean          message_on;
boolean                 message_dontfuckwithme;
static boolean          message_nottobefuckedwith;

static hu_stext_t       w_message;
static int              message_counter;

//extern int              showMessages; [STRIFE] no such variable

static boolean          headsupactive = false;

// [SVE] haleyjd: notification positions for notification widget
enum notificationpos_e
{
    NOTIFY_POS_STATBAR,
    NOTIFY_POS_FULLSCREEN
};

// [SVE] svillarreal
static hu_stext_t       w_notification;
static boolean          notification_on;
static int              notification_counter;
static int              notification_pos;
static int              notification_y;
static boolean          showfragschart;

// haleyjd 20130915 [STRIFE]: need nickname
extern char *nickname;

// haleyjd 20130915 [STRIFE]: true if setting nickname
static boolean hu_setting_name = false;

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

// haleyjd 08/31/10: [STRIFE] Changed for Strife level names.
// List of names for levels.

const char *const mapnames[HU_NUMMAPNAMES] =
{
    // Strife map names

    // First "episode" - Quest to destroy the Order's Castle
    HUSTR_1,
    HUSTR_2,
    HUSTR_3,
    HUSTR_4,
    HUSTR_5,
    HUSTR_6,
    HUSTR_7,
    HUSTR_8,
    HUSTR_9,

    // Second "episode" - Kill the Bishop and Make a Choice
    HUSTR_10,
    HUSTR_11,
    HUSTR_12,
    HUSTR_13,
    HUSTR_14,
    HUSTR_15,
    HUSTR_16,
    HUSTR_17,
    HUSTR_18,
    HUSTR_19,

    // Third "episode" - Shut down Factory, kill Loremaster and Entity
    HUSTR_20,
    HUSTR_21,
    HUSTR_22,
    HUSTR_23,
    HUSTR_24,
    HUSTR_25,
    HUSTR_26,
    HUSTR_27,
    HUSTR_28,
    HUSTR_29,

    // "Secret" levels - Abandoned Base and Training Facility
    HUSTR_30,
    HUSTR_31,

    // Demo version maps
    HUSTR_32,
    HUSTR_33,
    HUSTR_34,

    // [SVE]: Super secret level
    HUSTR_35,

    // [SVE]: Capture the Chalice maps
    HUSTR_36,
    HUSTR_37,
    HUSTR_38
};

//
// HU_Init
//
// haleyjd 09/18/10: [STRIFE]
// * Modified to load yfont along with hu_font.
//
void HU_Init(void)
{
    int		i;
    int		j;
    char	buffer[9];
    char        fefbuf[9];

    // load the heads-up font
    j = HU_FONTSTART;
    for (i=0;i<HU_FONTSIZE;i++)
    {
        DEH_snprintf(fefbuf, 9, "FEF%.3d", j);
        DEH_snprintf(buffer, 9, "STCFN%.3d", j++);
        hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);

        // haleyjd 09/18/10: load yfont as well; and yes, this is exactly
        // how Rogue did it :P
        buffer[2] = 'B';
        yfont[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);

        // haleyjd 20141204: [SVE]
        ffont[i] = (patch_t *) W_CacheLumpName(fefbuf, PU_STATIC);
    }

    // [SVE]
    for(i = 0; i < MAXPLAYERS; i++)
        M_snprintf(player_names[i], sizeof(player_names[i]), "%d: ", i+1);
}

//
// HU_Stop
//
// [STRIFE] Verified unmodified.
//
void HU_Stop(void)
{
    headsupactive = false;
}

//
// HU_Start
//
// haleyjd 09/18/10: [STRIFE] Added a hack for nickname at the end.
//
void HU_Start(void)
{
    int         i;
    const char *s;

    // haleyjd 20120211: [STRIFE] not called here.
    //if (headsupactive)
    //    HU_Stop();
    
    // haleyjd 20120211: [STRIFE] moved up
    // create the map title widget
    HUlib_initTextLine(&w_title,
                       HU_TITLEX, HU_TITLEY,
                       hu_font,
                       HU_FONTSTART);

    // haleyjd 08/31/10: [STRIFE] Get proper map name.
    // [SVE]: rangecheck to allow maps >= number of built-in maps
    if(gamemap - 1 < HU_NUMMAPNAMES)
        s = HU_TITLE;
    else
        s = "New Area";

    // [STRIFE] Removed Chex Quest stuff.

    // dehacked substitution to get modified level name
    s = DEH_String(s);
    
    while (*s)
        HUlib_addCharToTextLine(&w_title, *(s++));

    // haleyjd 20120211: [STRIFE] check for headsupactive
    if(!headsupactive)
    {
        plr = &players[consoleplayer];
        message_on = false;
        message_dontfuckwithme = false;
        message_nottobefuckedwith = false;
        chat_on = false;

        // create the message widget
        HUlib_initSText(&w_message,
                        HU_MSGX, HU_MSGY, HU_MSGHEIGHT,
                        hu_font,
                        HU_FONTSTART, &message_on);

        // create the chat widget
        HUlib_initIText(&w_chat,
                        HU_INPUTX, HU_INPUTY,
                        hu_font,
                        HU_FONTSTART, &chat_on);

        if(screenblocks > 10)
        {
            notification_pos = NOTIFY_POS_FULLSCREEN;
            notification_y   = HU_NOTIFICATIONY + 24;
        }
        else
        {
            notification_pos = NOTIFY_POS_STATBAR;
            notification_y   = HU_NOTIFICATIONY;
        }

        // [SVE] svillarreal - create the notification widget
        HUlib_initSText(&w_notification,
                        -1, notification_y, netgame ? 1 : HU_MSGHEIGHT,
                        ffont,
                        HU_FONTSTART, &notification_on);

        // create the inputbuffer widgets
        for (i=0 ; i<MAXPLAYERS ; i++)
            HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, &always_off);

        headsupactive = true;
    }
}

//
// HU_Drawer
//
// [STRIFE] Verified unmodified.
//
void HU_Drawer(void)
{
    static boolean lasthudchanged = false;
    boolean hudchanged;

    HUlib_drawSText(&w_message);
    HUlib_drawSText(&w_notification);
    HUlib_drawIText(&w_chat);
    if (automapactive)
        HUlib_drawTextLine(&w_title, false, false);

    if(deathmatch && (showfragschart || players[consoleplayer].health <= 0) && screenblocks >= 10)
        HUlib_drawFrags();

    hudchanged = (screenblocks > 10);

    // [SVE] svillarreal - need to change the offset for notifications
    // if the hud changed at all
    if(lasthudchanged != hudchanged)
    {
        notification_pos = hudchanged ? NOTIFY_POS_FULLSCREEN : NOTIFY_POS_STATBAR;
        notification_y   = hudchanged ? HU_NOTIFICATIONY + 24 : HU_NOTIFICATIONY;

        HUlib_initSText(&w_notification, -1, notification_y, netgame ? 1 : HU_MSGHEIGHT,
                        ffont, HU_FONTSTART, &notification_on);

        lasthudchanged = hudchanged;
    }
}

//
// HU_Erase
//
// [STRIFE] Verified unmodified.
//
void HU_Erase(void)
{
    HUlib_eraseSText(&w_message);
    HUlib_eraseSText(&w_notification);
    HUlib_eraseIText(&w_chat);
    HUlib_eraseTextLine(&w_title);
}

//
// HU_SetNotification
//
// [SVE] svillarreal - broadcast a special message
//

void HU_SetNotification(char *message)
{
    S_StartSound(NULL, sfx_yeah);
    notification_on = true;
    notification_counter = HU_MSGTIMEOUT/2;

    HUlib_addMessageToSText(&w_notification, NULL, message);
    w_notification.l[w_notification.cl].x = (SCREENWIDTH/2) - (HUlib_yellowTextWidth(message)/2);
}

//
// HU_NotifyCheating
//
// haleyjd 20141122: [SVE] Convenience function
//
void HU_NotifyCheating(player_t *pl)
{
    if(!pl || !(pl->cheats & CF_CHEATING))
    {
        // only one at a time plz
        // 20141203: also not in netgames.
        if(!netgame && notification_counter <= 0) 
        {
            char *msg = "ACHIEVEMENTS ARE DISABLED";
            S_StartSound(NULL, sfx_radio);
            notification_on = true;
            notification_counter = HU_MSGTIMEOUT/2;

            HUlib_addMessageToSText(&w_notification, NULL, msg);
            w_notification.l[w_notification.cl].x = (SCREENWIDTH/2) - (HUlib_yellowTextWidth(msg)/2);
        }

        if(pl)
            pl->cheats |= CF_CHEATING;
    }
}

// 
// HU_ShowTime
//
// [SVE] haleyjd 20141213: show timer countdown
//
void HU_ShowTime(void)
{
    char timestr[90];
    int minutes;
    int seconds;

    if(!levelTimer || levelTimeCount <= 0)
        return;

    // [SVE]: enhanced output
    minutes = ((levelTimeCount/TICRATE) / 60) % 60;
    seconds = (levelTimeCount/TICRATE) % 60;

    M_snprintf(timestr, sizeof(timestr), "%02d:%02d", minutes, seconds);

    notification_on = true;
    notification_counter = 2;

    HUlib_addMessageToSText(&w_notification, NULL, timestr);
    w_notification.l[w_notification.cl].x = (SCREENWIDTH/2) - (HUlib_yellowTextWidth(timestr)/2);
}

//
// HU_addMessage
//
// haleyjd 09/18/10: [STRIFE] New function
// See if you can tell whether or not I had trouble with this :P
// Looks to be extremely buggy, hackish, and error-prone.
//
// <Markov> This is definitely not the best that Rogue had to offer. Markov.
//
//  Fastcall Registers:   edx          ebx
//      Temp Registers:   esi          edi
void HU_addMessage(const char *prefix, const char *message)
{
    char        c;         // eax
    int         width = 0; // edx
    const char *rover1;    // ebx (in first loop)
    const char *rover2;    // ecx (in second loop)
    char       *bufptr;    // ebx (in second loop)
    char        buffer[HU_MAXLINELENGTH+2];  // esp+52h

    // Loop 1: Total up width of prefix.
    rover1 = prefix;
    if(rover1)
    {
        while((c = *rover1))
        {
            c = toupper((int)(byte)c) - HU_FONTSTART;
            ++rover1;

            if(c < 0 || c >= HU_FONTSIZE)
                width += 4;
            else
                width += SHORT(hu_font[(int) c]->width);
        }
    }

    // Loop 2: Copy as much of message into buffer as will fit on screen
    bufptr = buffer;
    rover2 = message;
    while((c = *rover2))
    {
        if((c == ' ' || c == '-') && width > 285)
            break;

        *bufptr = c;
        ++bufptr;       // BUG: No check for overflow.
        ++rover2;
        c = toupper(c);

        if(c == ' ' || c < '!' || c >= '_')
            width += 4;
        else
        {
            c -= HU_FONTSTART;
            width += SHORT(hu_font[(int) c]->width);
        }
    }

    // Too big to fit?
    // BUG: doesn't consider by how much it's over.
    if(width > 320) 
    {
        // backup a char... hell if I know why.
        --bufptr;
        --rover2;
    }

    // rover2 is not at the end?
    if((c = *rover2))
    {
        // if not ON a space...
        if(c != ' ')
        {
            // back up both pointers til one is found.
            // BUG: no check against LHS of buffer. Hurr!
            while(*bufptr != ' ')
            {
                --bufptr;
                --rover2;
            }
        }
    }

    *bufptr = '\0';

    // Add two message lines.
    HUlib_addMessageToSText(&w_message, prefix, buffer);
    HUlib_addMessageToSText(&w_message, NULL,   rover2);
}

//
// HU_Ticker
//
// haleyjd 09/18/10: [STRIFE] Changes to split up message into two lines,
// and support for player names (STRIFE-TODO: unfinished!)
//
void HU_Ticker(void)
{
    int i, rc;
    char c;

    // [SVE]: show time limit if active
    HU_ShowTime();
    
    // tick down message counter if message is up
    if(message_counter && !--message_counter)
    {
        message_on = false;
        message_nottobefuckedwith = false;
    }

    // [SVE] svillarreal - update notification widget
    if(--notification_counter > 0)
        notification_on = !automapactive;
    else
    {
        w_notification.cl = 0;

        // clear all text
        for(i = 0; i < w_notification.h; i++)
        {
            w_notification.l[i].l[0] = 0;
            w_notification.l[i].len = 0;
        }

        notification_on = false;
        notification_counter = 0;
    }

    // haleyjd 20110219: [STRIFE] this condition was removed
    //if (showMessages || message_dontfuckwithme)
    //{
        // display message if necessary
        if((plr->message && !message_nottobefuckedwith)
            || (plr->message && message_dontfuckwithme))
        {
            //HUlib_addMessageToSText(&w_message, 0, plr->message);
            HU_addMessage(NULL, plr->message); // haleyjd [STRIFE]
            plr->message = 0;
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            message_nottobefuckedwith = message_dontfuckwithme;
            message_dontfuckwithme = 0;
        }
    //} // else message_on = false;

    // check for incoming chat characters
    if(netgame)
    {
        for(i=0 ; i<MAXPLAYERS; i++)
        {
            if(!playeringame[i])
                continue;
            if(i != consoleplayer
                && (c = players[i].cmd.chatchar))
            {
                if(c <= HU_CHANGENAME) // [STRIFE]: allow for HU_CHANGENAME
                    chat_dest[i] = c;
                else
                {
                    rc = HUlib_keyInIText(&w_inputbuffer[i], c);
                    if(rc && c == KEY_ENTER)
                    {
                        if(w_inputbuffer[i].l.len
                            && (chat_dest[i] == consoleplayer+1
                             || chat_dest[i] == HU_BROADCAST))
                        {
                            HU_addMessage(player_names[i],
                                          w_inputbuffer[i].l.l);

                            message_nottobefuckedwith = true;
                            message_on = true;
                            message_counter = HU_MSGTIMEOUT;
                            S_StartSound(0, sfx_radio);
                        }
                        else if(chat_dest[i] == HU_CHANGENAME)
                        {
                            // haleyjd 20130915 [STRIFE]: set player name
                            DEH_snprintf(player_names[i], sizeof(player_names[i]),
                                         "%.27s: ", 
                                         w_inputbuffer[i].l.l);
                        }
                        HUlib_resetIText(&w_inputbuffer[i]);
                    }
                }
                players[i].cmd.chatchar = 0;
            }
        }
    }
}

#define QUEUESIZE		128

static char	chatchars[QUEUESIZE];
static int	head = 0;
static int	tail = 0;

//
// HU_queueChatChar
//
// haleyjd 09/18/10: [STRIFE]
// * No message is given if a chat queue overflow occurs.
//
void HU_queueChatChar(char c)
{
    chatchars[head] = c;
    if (((head + 1) & (QUEUESIZE-1)) != tail)
    {
        head = (head + 1) & (QUEUESIZE-1);
    }
}

//
// HU_dequeueChatChar
//
// [STRIFE] Verified unmodified.
//
char HU_dequeueChatChar(void)
{
    char c;

    if (head != tail)
    {
        c = chatchars[tail];
        tail = (tail + 1) & (QUEUESIZE-1);
    }
    else
    {
        c = 0;
    }

    return c;
}

//
// HU_Responder
//
// haleyjd 09/18/10: [STRIFE]
// * Mostly unmodified, except:
//   - The default value of key_message_refresh is changed. That is handled
//     elsewhere in Choco, however.
//   - There is support for setting the player name through the chat
//     mechanism.
//
boolean HU_Responder(event_t *ev)
{
    static char         lastmessage[HU_MAXLINELENGTH+1];
    char*               macromessage;
    static boolean      altdown = false;
    static boolean      discardinput = false;
    int                 i;
    int                 numplayers;

    static int          num_nobrainers = 0;

    numplayers = 0;
    for (i=0 ; i<MAXPLAYERS ; i++)
        numplayers += playeringame[i];

    if (ev->data1 == KEY_RSHIFT)
    {
        return false;
    }
    else if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT)
    {
        altdown = ev->type == ev_keydown;
        return false;
    }

    // [SVE]: frags chart
    if (netgame && ev->data1 == key_menu_save)
    {
        if (ev->type == ev_keydown)
            showfragschart = true;
        else if (ev->type == ev_keyup)
            showfragschart = false;
    }

    if (ev->type != ev_keydown && ev->type != ev_text)
        return false;

    if (!chat_on)
    {
        if (ev->data1 == key_message_refresh)
        {
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            return true;
        }
        else if (netgame && ev->data1 == key_multi_msg)
        {
            chat_on = true;
            HUlib_resetIText(&w_chat);
            HU_queueChatChar(HU_BROADCAST);
            if (isprint(ev->data1))
                discardinput = true;
            return true;
        }
        return false;
        // [STRIFE]: You cannot go straight to chatting with a particular
        // player from here... you must press 't' first. See below.
    }

    if (ev->type == ev_text && discardinput)
    {
        discardinput = false;
        return true;
    }

    // send a macro
    if (altdown)
    {
        if (ev->data1 > '9' || ev->data1 < '0')
            return false;
        // fprintf(stderr, "got here\n");
        macromessage = chat_macros[ev->data1 - '0'];

        // kill last message with a '\n'
        HU_queueChatChar(KEY_ENTER); // DEBUG!!!

        // send the macro message
        while (*macromessage)
            HU_queueChatChar(*macromessage++);
        HU_queueChatChar(KEY_ENTER);

        // leave chat mode and notify that it was sent
        chat_on = false;
        M_StringCopy(lastmessage, chat_macros[ev->data1 - '0'],
                     sizeof(lastmessage));
        plr->message = lastmessage;
        return true;
    }

    if (ev->data1 == KEY_ENTER)
    {
        chat_on = false;
        if (w_chat.l.len)
        {
            HU_queueChatChar(KEY_ENTER);

            // [STRIFE]: name setting
            if (hu_setting_name)
            {
                // [SVE]: display pretty player name
                char *oldName = HUlib_makePrettyPlayerName(consoleplayer);
                DEH_snprintf(lastmessage, sizeof(lastmessage),
                             "%s now %.27s", oldName, w_chat.l.l);
                Z_Free(oldName);

                // set name for local client
                M_snprintf(player_names[consoleplayer],
                           sizeof(player_names[consoleplayer]),
                           "%.27s: ", w_chat.l.l);
                hu_setting_name = false;
            }
            else
            {
                M_StringCopy(lastmessage, w_chat.l.l, sizeof(lastmessage));
            }
            plr->message = lastmessage;
        }
        return true;
    }
    else if (ev->data1 == KEY_ESCAPE)
    {
        chat_on = false;
        return true;
    }

    if (ev->type == ev_keydown && isprint(ev->data1))
    {
        return true; // eat keydown inputs that have text equivalent
    }

    if (w_chat.l.len) // [STRIFE]: past first char of chat?
    {
        const unsigned char c = (unsigned char) ev->data1;
        if (HUlib_keyInIText(&w_chat, c) == true)
        {
            if (isprint(c))
            {
                HU_queueChatChar(c);
            }
            return true;
        }
    }
    else
    {
        // [STRIFE]: check for player-specific message;
        // slightly different than vanilla, to allow keys to be customized
        for(i = 0; i < MAXPLAYERS; i++)
        {
            if (ev->data1 == key_multi_msgplayer[i])
                break;
        }
        if (i < MAXPLAYERS)
        {
            // talking to self?
            if (i == consoleplayer)
            {
                num_nobrainers++;
                if (num_nobrainers < 3)
                    plr->message = DEH_String(HUSTR_TALKTOSELF1);
                else if (num_nobrainers < 6)
                    plr->message = DEH_String(HUSTR_TALKTOSELF2);
                else if (num_nobrainers < 9)
                    plr->message = DEH_String(HUSTR_TALKTOSELF3);
                else if (num_nobrainers < 32)
                    plr->message = DEH_String(HUSTR_TALKTOSELF4);
                else
                    plr->message = DEH_String(HUSTR_TALKTOSELF5);
            }
            else
            {
                HU_queueChatChar(i+1);
                DEH_snprintf(lastmessage, sizeof(lastmessage),
                             "Talking to: %c", '1' + i);
                plr->message = lastmessage;
                return true;
            }
        }
        else if (ev->data1 == '$') // [STRIFE]: name changing
        {
            HU_queueChatChar(HU_CHANGENAME);
            M_StringCopy(lastmessage, DEH_String("Changing Name:"),
                         sizeof(lastmessage));
            plr->message = lastmessage;
            hu_setting_name = true;
            return true;
        }

        else
        {
            if (HUlib_keyInIText(&w_chat, ev->data1) == true)
            {
                HU_queueChatChar(ev->data1);
                return true;
            }
        }
    }

    return false;
}
