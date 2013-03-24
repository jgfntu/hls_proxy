/**
 * libhls_event.h : used for hls event callback
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *
 * 2013-01-28:	first version
 *
 */
#ifndef __LIBHLS_EVENT_H
#define __LIBHLS_EVENT_H

#include "util_live.h"

/* ----------------------------------------------------------------------- */ 
/* 					From VLC project libhls_events.h					   */
/* ----------------------------------------------------------------------- */
/**
 * \file
 * This file defines libhls_event external API
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup libhls_event
 * @{
 */

/**
 * Event types
 */
enum libhls_event_e {
    /* Append new event types at the end of a category.
     * Do not remove, insert or re-order any entry.
     * Keep this in sync with lib/event.c:libhls_event_type_name(). */
    libhls_MediaMetaChanged=0,
    libhls_MediaSubItemAdded,
    libhls_MediaDurationChanged,
    libhls_MediaParsedChanged,
    libhls_MediaFreed,
    libhls_MediaStateChanged,

    libhls_MediaPlayerMediaChanged=0x100,
    libhls_MediaPlayerNothingSpecial,
    libhls_MediaPlayerOpening,
    libhls_MediaPlayerBuffering,
    libhls_MediaPlayerPlaying,
    libhls_MediaPlayerPaused,
    libhls_MediaPlayerStopped,
    libhls_MediaPlayerForward,
    libhls_MediaPlayerBackward,
    libhls_MediaPlayerEndReached,
    libhls_MediaPlayerEncounteredError,
    libhls_MediaPlayerTimeChanged,
    libhls_MediaPlayerPositionChanged,
    libhls_MediaPlayerSeekableChanged,
    libhls_MediaPlayerPausableChanged,
    libhls_MediaPlayerTitleChanged,
    libhls_MediaPlayerSnapshotTaken,
    libhls_MediaPlayerLengthChanged,
    libhls_MediaPlayerVout,

	libhls_MediaPlayerDead,					/* 0x113 add item for "INPUT_EVENT_DEAD" for bug 20121110 */

    libhls_MediaListItemAdded=0x200,
    libhls_MediaListWillAddItem,
    libhls_MediaListItemDeleted,
    libhls_MediaListWillDeleteItem,

    libhls_MediaListViewItemAdded=0x300,
    libhls_MediaListViewWillAddItem,
    libhls_MediaListViewItemDeleted,
    libhls_MediaListViewWillDeleteItem,

    libhls_MediaListPlayerPlayed=0x400,
    libhls_MediaListPlayerNextItemSet,
    libhls_MediaListPlayerStopped,

    libhls_MediaDiscovererStarted=0x500,
    libhls_MediaDiscovererEnded,

};


/**@} */

#ifdef __cplusplus
}
#endif

#endif
