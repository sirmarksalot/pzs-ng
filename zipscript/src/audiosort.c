#include <string.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef HAVE_STRLCPY
# include "strl/strl.h"
#endif

#include "../conf/zsconfig.h"
#include "zsconfig.defaults.h"

#include "objects.h"
#include "zsfunctions.h"
#include "audiosort.h"
#include "multimedia.h"
#include "helpfunctions.h"

/*
#define audio_sort_flac_separate     TRUE
#define audio_genre_path_flac        "/site/index/flac/music.by.genre/"
#define audio_artist_path_flac       "/site/index/flac/music.by.artist/"
#define audio_year_path_flac         "/site/index/flac/music.by.year/"
#define audio_group_path_flac        "/site/index/flac/music.by.group/"
#define audio_language_path_flac     "/site/index/flac/music.by.language/"
#define audio_artist_nosub           TRUE
*/



/*
reutrns 0 if no bad chars found and 1st char is not digit
returns 1 if no bad chars found and 1st char is digit
returns 2 if bad chars found and 1st char is not digit
returns 3 if bad chars found and 1st char is digit
*/
int allowedcharscheck(char *temp_p, char *link_target)
{
	const char *allowed = ",&'._-()0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const char *numeric = "0123456789";
	const char *initalchar = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (!strchr(initalchar, *temp_p)) {
		if (strchr(numeric, *link_target)) {
			return 3;
		} else {
			return 2;
		}
	}
	char *c = temp_p;
	while (*c) {
		if (!strchr(allowed, *c)) {
			if (strchr(numeric, *temp_p)) {
				return 3;
			} else {
				return 2;
			}
		}
		c++;
	}
	if (strchr(numeric, *temp_p)) {
		return 1;
	} else {
		return 0;
	}
}

void audioSortDir(char *targetDir)
{
	int cnt = 0;
	char link_source[PATH_MAX], link_target[PATH_MAX], *file_target = NULL;
	struct audio info;
	DIR *ourDir;


	if (*targetDir != '/')
	{
		d_log("audioSortDir: not an absolute path. (%s)\n", targetDir);
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

	cnt = extractDirname(link_target, targetDir);
	strlcpy(link_source, targetDir, PATH_MAX);

	if (chdir(targetDir) == -1) {
		d_log("audioSortDir: Failed to chdir() to %s: %s\n", targetDir, strerror(errno));
	}
	if ((ourDir = opendir(targetDir)) == NULL) {
		printf("Error: Failed to open dir \"%s\" : %s\n", targetDir, strerror(errno));
		return;
	}

	file_target = findfileextfromlist(ourDir, audio_types);

	get_audio_info(file_target, &info);
	audioSort(&info, link_source, link_target);

	closedir(ourDir);
}

void audioSort(struct audio *info, char *link_source, char *link_target)
{
#if (audio_genre_sort == TRUE) || (audio_artist_sort == TRUE) || (audio_year_sort == TRUE) || (audio_group_sort == TRUE) || (audio_language_sort == TRUE)
	int n = 0;
 #if (audio_artist_sort == TRUE) || (audio_group_sort == TRUE) || (audio_language_sort == TRUE)
	char *temp_p = NULL;
	char temp_pp[PATH_MAX];
	char dualpath[PATH_MAX];
	int allowed;
  #if (audio_language_sort == TRUE)
	char language[3];
  #endif
  #if (audio_group_sort == TRUE)
	char *temp_q = NULL;
	char temp_nam[16];
  #endif
 #endif
#else
	d_log("audioSort: No audio_*_sort is set to TRUE - skipping sorting of audio\n");
	(void)info; (void)link_source; (void)link_target;
#endif

	if (subcomp(link_target, NULL)) {
        	int i, pos = 0;
	        char sourceDir[PATH_MAX];
        	for (i = strlen(link_source); i > 0; i--) {
                	if (link_source[i] == '/')  {  pos = i; break; }
	        }
        	strcpy(sourceDir, link_source);
	        strlcpy(link_source, sourceDir, pos + 1);
        	extractDirname(link_target, link_source);
	}


#if ( audio_genre_sort == TRUE )
	d_log("audioSort:   Sorting audio by genre (%s)\n", info->id3_genre);
	#if ( audio_sort_flac_separate == TRUE )
		if (strncasecmp(info->codec, "FLAC", 4) == 0 ) {
			createlink(audio_genre_path_flac, info->id3_genre, link_source, link_target);
		} else {
			createlink(audio_genre_path, info->id3_genre, link_source, link_target);
		}
	#else
		createlink(audio_genre_path, info->id3_genre, link_source, link_target);
	#endif
#endif


#if ( audio_artist_sort == TRUE )
	d_log("audioSort:   Sorting audio by artist\n");
	if (*info->id3_artist) {
		d_log("audioSort:     - artist: %s\n", info->id3_artist);
		if (!strncasecmp(link_target, "VA", 2) && (link_target[2] == '-' || link_target[2] == '_')) {
 #if ( audio_artist_sort_various_only == TRUE )
			memcpy(info->id3_artist, "VA", 3);
 #endif
			#if ( audio_sort_flac_separate == TRUE )
				if (strncasecmp(info->codec, "FLAC", 4) == 0 ) {
					createlink(audio_artist_path_flac, "VA", link_source, link_target);
				} else {
					createlink(audio_artist_path, "VA", link_source, link_target);
				}
			#else
				createlink(audio_artist_path, "VA", link_source, link_target);
			#endif
		}
		if (memcmp(info->id3_artist, "VA", 3)) {

 #if ( audio_artist_nosub == FALSE )

			temp_p = ng_realloc2(temp_p, 2, 1, 0, 1);

  #if ( audio_artist_noid3 == FALSE )
			snprintf(temp_p, 2, "%c", toupper(*info->id3_artist));
  #else
			snprintf(temp_p, 2, "%c", toupper(*link_target));
  #endif

 #else
			temp_p = ng_realloc2(temp_p, strlen(info->id3_artist)+1, 1, 0, 1);
			snprintf(temp_p, strlen(info->id3_artist)+1, "%s", info->id3_artist);
			temp_p = check_nocase_linkname(audio_artist_path, temp_p);
			space_to_dot(temp_p);
 #endif
			#if ( audio_sort_flac_separate == TRUE )
				strcpy(temp_pp, temp_p);
				allowed = allowedcharscheck(temp_p, link_target);
				if (strncasecmp(info->codec, "FLAC", 4) == 0 ) {
					if (allowed == 0) {
						snprintf(dualpath, sizeof(dualpath), "%s%c", audio_artist_path_flac, toupper(temp_p[0]));
					} else if (allowed == 1) {
						snprintf(dualpath, sizeof(dualpath), "%s%s", audio_artist_path_flac, "0-9");
					} else if (allowed == 2) {
						char *c = link_target;
						const char *initalchar = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
						while (!strchr(initalchar, *c)) {
							c++;
						}
						snprintf(temp_p, 2, "%c", toupper(*c));
						snprintf(dualpath, sizeof(dualpath), "%s", audio_artist_path_flac);
					} else {
						snprintf(temp_p, 4, "%s", "0-9");
						snprintf(dualpath, sizeof(dualpath), "%s", audio_artist_path_flac);
					}
					mkdir(dualpath, 0777);
					createlink(dualpath, temp_p, link_source, link_target);
				} else {
					if (allowed == 0) {
						snprintf(dualpath, sizeof(dualpath), "%s%c", audio_artist_path, toupper(temp_p[0]));
					} else if (allowed == 1) {
						snprintf(dualpath, sizeof(dualpath), "%s%s", audio_artist_path, "0-9");
					} else if (allowed == 2) {
						char *c = link_target;
						const char *initalchar = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
						while (!strchr(initalchar, *c)) {
							c++;
						}
						snprintf(temp_p, 2, "%c", toupper(*c));
						snprintf(dualpath, sizeof(dualpath), "%s", audio_artist_path);
					} else {
						snprintf(temp_p, 4, "%s", "0-9");
						snprintf(dualpath, sizeof(dualpath), "%s", audio_artist_path);
					}
					mkdir(dualpath, 0777);
					createlink(dualpath, temp_p, link_source, link_target);
				}
			#else
				createlink(audio_artist_path, temp_p, link_source, link_target);
			#endif
			ng_free(temp_p);
		}
	}
#endif


#if ( audio_year_sort == TRUE )
	d_log("audioSort:   Sorting audio by year (%s)\n", info->id3_year);
	if (*info->id3_year != 0) {
		#if ( audio_sort_flac_separate == TRUE )
			if (strncasecmp(info->codec, "FLAC", 4) == 0 ) {
				createlink(audio_year_path_flac, info->id3_year, link_source, link_target);
			} else {
				createlink(audio_year_path, info->id3_year, link_source, link_target);
			}
		#else
			createlink(audio_year_path, info->id3_year, link_source, link_target);
		#endif
	}
#endif


#if ( audio_group_sort == TRUE )
	d_log("audioSort:   Sorting audio by group\n");
	temp_p = remove_pattern(link_target, "*-", RP_LONG_LEFT);
	temp_p = remove_pattern(temp_p, "_", RP_SHORT_LEFT);
	n = (int)strlen(temp_p);
	if (n > 0 && n < 15) {
		strncpy(temp_nam, temp_p, sizeof(temp_nam));
		if (n > 4) {
			temp_q = temp_nam + n - 4;
			if (!strncasecmp(temp_q, "_INT", 4)) {
				d_log("audioSort:   - Internal release detected\n");
				*temp_q = '\0';
			}
		}
		d_log("audioSort:   - Valid groupname found: %s (%i) - checking for exisiting sort-dir.\n", temp_nam, n);
		temp_p = check_nocase_linkname(audio_group_path, temp_p);
		d_log("audioSort:   - Valid groupname found: %s (%i) - using this.\n", temp_p, n);
		#if ( audio_sort_flac_separate == TRUE )
			if (strncasecmp(info->codec, "FLAC", 4) == 0 ) {
				createlink(audio_group_path_flac, temp_p, link_source, link_target);
			} else {
				createlink(audio_group_path, temp_p, link_source, link_target);
			}
		#else
			createlink(audio_group_path, temp_p, link_source, link_target);
		#endif
	}
#endif


#if ( audio_language_sort == TRUE )
	d_log("audioSort:   Sorting audio by language\n");
	language[0] = '\0';
	n = (int)strlen(link_target);
	while ( n > 3) {
		if (link_target[n] == '-') {
			if ((unsigned char)link_target[n-3] == '-' && (unsigned char)link_target[n-2] >= 'A' && (unsigned char)link_target[n-2] <= 'Z' && (unsigned char)link_target[n-1] >= 'A' && (unsigned char)link_target[n-1] <= 'Z') {
				language[0] = link_target[n-2];
				language[1] = link_target[n-1];
				language[2] = '\0';
				temp_p = language;
				break;
			}
		}
		n--;
	}
	if (language[0] != '\0' && !strcomp(audio_ignored_languages, language)) {
		d_log("audioSort:   - Valid language found: %s\n", language);
		#if ( audio_sort_flac_separate == TRUE )
			if (strncasecmp(info->codec, "FLAC", 4) == 0 ) {
				createlink(audio_language_path_flac, temp_p, link_source, link_target);
			} else {
				createlink(audio_language_path, temp_p, link_source, link_target);
			}
		#else
			createlink(audio_language_path, temp_p, link_source, link_target);
		#endif
	} else {
		d_log("audioSort:   - No valid language found - skipping.\n");
	}
#endif
}

