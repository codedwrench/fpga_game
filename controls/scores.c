#include "scores.h"
char highScores[3][2][6] = { { { 0 } } };
short int saveScoresToSDFile(short int *f)
{
	char highScoreStr[12] = { 0 }; //Score line to be saved on the sd card
	alt_up_sd_card_fclose(*f); //Close the file so the file pointer is back at the beginning of the file
	OSTimeDly(100); //Wait for a moment, this is because otherwise it sometimes fucks up
	short int file = alt_up_sd_card_fopen("scores.txt", 0); //Open the file again

	alt_u8 i = 0;
	alt_u8 j;
	alt_u8 c = 0;
	for (i = 0; i < 3; i++)
	{
		sprintf(highScoreStr, "%s %s\r\n", highScores[i][0], highScores[i][1]);
		for (j = 0; j < 12; j++)
		{
			if (highScoreStr[j] != 0)
			{
				c = highScoreStr[j];
				alt_up_sd_card_write(file, c); //This function only supports writing single characters to the sdcard, so here we split the string up in characters and write that to the sdcard
			}
		}
	}
	return alt_up_sd_card_fclose(file);
}
void setInitScore(short int* file) //This function initializes the scores file to be used on our game, this is so we can safely delete the scores.txt file and have it reinitialized;
{
	alt_u8 err;
	sprintf(highScores[2][0], "07:30");
	sprintf(highScores[2][1], "CCC");
	sprintf(highScores[1][0], "05:00");
	sprintf(highScores[1][1], "BBB");
	sprintf(highScores[0][0], "03:30");
	sprintf(highScores[0][1], "AAA");
	err = saveScoresToSDFile(file);
	if(err)
		printf("Couldn't save file to SD card");
}
void setScore(alt_8 pos, alt_u8 name[], short int* f)
{
	alt_u8 err = 0;
	switch(pos)
	{
	case 0:
		//If player ends up first then move  other players down one place
		sprintf(highScores[pos+2][0], "%s", highScores[pos+1][0]);
		sprintf(highScores[pos+2][1], "%s", highScores[pos+1][1]);
		sprintf(highScores[pos+1][0], "%s", highScores[pos][0]);
		sprintf(highScores[pos+1][1], "%s", highScores[pos][1]);
		//Add player to beginning of the list
		sprintf(highScores[pos][0], "%.2d:%.2d", min, sec);
		sprintf(highScores[pos][1], "%s", name);
		break;
	case 1:
		//If player ends up second place, then only one spot has to be moved down
		sprintf(highScores[pos+1][0], "%s", highScores[pos][0]);
		sprintf(highScores[pos+1][1], "%s", highScores[pos][1]);
		sprintf(highScores[pos][0], "%.2d:%.2d", min, sec);
		sprintf(highScores[pos][1], "%s", name);
		break;
	case 2:
		//If player ends up third then we can just overwrite the old 3rd spot
		sprintf(highScores[pos][0], "%.2d:%.2d", min, sec);
		sprintf(highScores[pos][1], "%s", name);
		break;
	default:
		break;
	}
	err = saveScoresToSDFile(f);
	if(err)
		printf("Couldn't save file to SD card");
}
alt_8 checkScore()
{
	alt_u8 i, j;
	alt_8 result = -1;
	alt_u8 score[6] = { 0 };
	sprintf(score, "%.2d:%.2d", min, sec);
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 5; j++) //Check if player is eligible to placed on the highscore list, because this is a string, we walk through it 5 times to compare all the characters
		{
			if (score[j] >= '0' && score[j] <= '9')
			{
				if (score[j] < highScores[i][0][j])
				{
					result = i;
					break;
				}
				else if (score[j] > highScores[i][0][j])
				{
					result = -1;
					break;
				}
			}
			else if (score[j] == 0)
			{
				break;
			}
			OSTimeDly(1);
		}
		if (result >= 0)
			break;
	}
	return result;
}
void loadScores(short int file)
{
	char str[6] = { 0 };
	char c =  alt_up_sd_card_read(file); //read file
	alt_u8 pos = 0;
	alt_u8 i = 0;
	alt_u8 j = 0;

	while(c != EOF)
	{
		switch(c)
		{
		case ' ':
		case '\r':
			sprintf(highScores[pos][j], "%s", str);
			j ^= 1; //flips between score and name so we don't have to use an if statement
			i = 0;
			memset(str, 0, sizeof(str)); //clears string so older longer strings won't get mixed up with the shorter strings
			break;
		case '\n':
			pos++; //go to the next score
			break;
		default:
			str[i] = c; //save the character into the string
			i++;
			break;
		}
		c =  alt_up_sd_card_read(file); //read next character
	}
}
void addPenalty(alt_u8 n,OS_EVENT * timer)
{
	ALT_SEM_PEND(timer, 0);
	sec += n; //adds a penalty to the amount of seconds
	if (sec > 59)
	{
		sec %= 60;
		min = (min + 1) % 60;
	}
	ALT_SEM_POST(timer);
}
