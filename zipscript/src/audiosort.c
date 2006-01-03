#include <string.h>

#ifndef HAVE_STRLCPY
# include "strl/strl.h"
#endif

#include "../conf/zsconfig.h"
#include "zsconfig.defaults.h"

#include "objects.h"
#include "zsfunctions.h"
#include "audiosort.h"
#include "multimedia.h"

void audioSortDir(char *targetDir)
{
	int cnt, n = 0;
	char link_source[PATH_MAX], link_target[PATH_MAX], *file_target;
	struct audio info;
	DIR *ourDir; 
	

	if (*targetDir != '/')
	{
		d_log("audioSort: not an absolute path. (%s)\n", targetDir);
		return;
	}

/* Look at something like that to verify that a release is complete?
 * (And tell user to run rescan if not)
 * char *sfv_data = NULL; 
 *
 * sfv_data = ng_realloc2(sfv_data, n, 1, 1, 1);
 * sprintf(sfv_data, storage "/%s/sfvdata", targetDir);
 * 
 * readsfv(sfv_data, &g.v, 0);
 */
	
	for (cnt = strlen(targetDir); cnt; cnt--) {
		if (targetDir[cnt] == '/') {
			strlcpy(link_target, targetDir + cnt + 1, n + 1);
			link_target[n] = 0;
			break;
		} else {
			n++;
		}
	}
	strlcpy(link_source, targetDir, PATH_MAX);
	
	chdir(targetDir);
	ourDir = opendir(targetDir);
	file_target = findfileext(ourDir, ".mp3");
	closedir(ourDir);

	get_mpeg_audio_info(file_target, &info);
	
	audioSort(&info, link_source, link_target);
}

void audioSort(struct audio *info, char *link_source, char *link_target)
{
#if ( audio_genre_sort == TRUE ) || (audio_artist_sort == TRUE) || (audio_year_sort == TRUE) || (audio_group_sort == TRUE)
	char *temp_p = NULL;
	int n = 0;
#else
	(void)info; (void)link_source; (void)link_target;
#endif

#if ( audio_genre_sort == TRUE )
	d_log("audioSort:   Sorting mp3 by genre (%s)\n", info->id3_genre);
	createlink(audio_genre_path, info->id3_genre, link_source, link_target);
#endif
#if ( audio_artist_sort == TRUE )
	d_log("audioSort:   Sorting mp3 by artist\n");
	if (*info->id3_artist) {
		d_log("audioSort:     - artist: %s\n", info->id3_artist);
		if (memcmp(info->id3_artist, "VA", 3)) {
			temp_p = ng_realloc(temp_p, 2, 1, 0, NULL, 1);
			snprintf(temp_p, 2, "%c", toupper(*info->id3_artist));
			createlink(audio_artist_path, temp_p, link_source, link_target);
			ng_free(temp_p);
		} else
			createlink(audio_artist_path, "VA", link_source, link_target);
	}
#endif
#if ( audio_year_sort == TRUE )
	d_log("audioSort:   Sorting mp3 by year (%s)\n", info->id3_year);
	if (*info->id3_year != 0)
		createlink(audio_year_path, info->id3_year, link_source, link_target);
#endif
#if ( audio_group_sort == TRUE )
	d_log("audioSort:   Sorting mp3 by group\n");
	temp_p = remove_pattern(link_target, "*-", RP_LONG_LEFT);
	temp_p = remove_pattern(temp_p, "_", RP_SHORT_LEFT);
	n = (int)strlen(temp_p);
	if (n > 0 && n < 15) {
		d_log("audioSort:   - Valid groupname found: %s (%i)\n", temp_p, n);
		createlink(audio_group_path, temp_p, link_source, link_target);
	}
#endif
}
