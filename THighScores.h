/*	THighScores.h
	Copyright (c) 2024, J T Frey
*/

/*!
	@header High score load/save
	Not much to say, the notoriety of getting a high score is great.
*/

#ifndef __THIGHSCORES_H__
#define __THIGHSCORES_H__

#include "tetrominotris_config.h"

/*
 * @typedef THighScoresRef
 *
 * Opaque reference to a high scores list.
 */
typedef struct THighScores * THighScoresRef;

/*
 * @function THighScoresLoad
 *
 * Initialize and return a high scores list from the given file
 * path.  Returns NULL if the file could not be read, etc.
 */
THighScoresRef THighScoresLoad(const char *filepath);

/*
 * @function THighScoresCreate
 *
 * Create a new high scores list.  The list will be _almost_
 * empty...
 */
THighScoresRef THighScoresCreate(void);

/*
 * @function THighScoresDestroy
 *
 * Deallocate the given highScores list.
 */
void THighScoresDestroy(THighScoresRef highScores);

/*
 * @function THighScoresGetCount
 *
 * Returns the number of high score records maintained by the
 * highScores list.
 */
unsigned int THighScoresGetCount(THighScoresRef highScores);

/*
 * @function THighScoresGetRecord
 *
 * Retrieve the score and initials for the record at index idx in the
 * highScores list.  Returns true if a record exists and *score and
 * initials were set, false otherwise.
 */
bool THighScoresGetRecord(THighScoresRef highScores, unsigned int idx, unsigned int *score, char initials[3]);

/*
 * @function THighScoresDoesQualify
 *
 * Returns true if the given score qualifies for a slot on the
 * highScores list, false otherwise.  If true, then *rank will
 * be set to the slot the score will occupy.
 */
bool THighScoresDoesQualify(THighScoresRef highScores, unsigned int score, unsigned int *rank);

/*
 * @function THighScoresRegister
 *
 * Register the given score for a slot in the highScores list.  The
 * provided three-character initials will be logged to the list.
 */
bool
THighScoresRegister(THighScoresRef highScores, unsigned int score, char initials[3]);

/*
 * @function THighScoresSave
 *
 * Attempt to write the highScores list to a file at the given
 * filepath.  Returns true if successful, false otherwise.
 */
bool THighScoresSave(THighScoresRef highScores, const char *filepath);

#endif /* __THIGHSCORES_H__ */
