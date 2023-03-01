#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C"
{
#endif


struct viktor_string {
  char *ptr;
  size_t len;
};

void init_string(struct viktor_string *s);
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct viktor_string *s);
void checkInternet(char* url, long timeout ,int *status, char **response, double *response_time);
void postRequest(char* url, char* jsonObj);


typedef struct DateAndTime {
    int year;
    int month;
    int day;
    int hour;
    int minutes;
    int seconds;
    int msec;
} DateAndTime;

char* gettimestamp();
char* getIndexDate();
char* getFormattedTimeStamp();
char *createJsonLog(char* level, char* logMessage, char* key, char* value);
void systemLog(char* logType, char* message, char* key, char* value);

#ifdef __cplusplus
}
#endif


#endif // UTILS_H_INCLUDED
