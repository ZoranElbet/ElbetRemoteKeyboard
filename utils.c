#include "utils.h"

void init_string(struct viktor_string *s) {
  s->len = 0;
  s->ptr = (char*)malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct viktor_string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr =(char*) realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;
  return size*nmemb;
}

void checkInternet(char* url, long timeout ,int *status, char **response, double *response_time){
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
    struct viktor_string s;
    init_string(&s);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
    res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    double total;
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total) ;
    if (http_code == 0){
      total = 0;
    }
    curl_easy_cleanup(curl);
    *status=http_code;
    *response = s.ptr;
    *response_time = total;

  }

}

void postRequest(char* url, char* jsonObj){
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if(curl) {
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
 //   struct string s;
   // init_string(&s);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_USERPWD, "elbet:gujmyh-fUspo6-goxzeb");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl,CURLOPT_VERBOSE,0L);
//    curl_easy_setopt(curl, CURLOPT_USERAGENT,"libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_SSLVERSION,1);
    res = curl_easy_perform(curl);

    long http_code = 0;

    curl_easy_cleanup(curl);
    }
}

char* terminalId="viktor-terminal";

char buffer[26];
char* gettimestamp(){
    time_t timer;

    struct tm* tm_info;
    timer = time(NULL);
    tm_info = gmtime(&timer);
    strftime(buffer, 26, "%Y-%m-%dT%H:%M:%S.000", tm_info);
    return buffer;
}

char* getIndexDate(){
    time_t timer;

    struct tm* tm_info;
    timer = time(NULL);
    tm_info = gmtime(&timer);
    strftime(buffer, 26, "%Y.%m.%d", tm_info);
    return buffer;//indexDate;

}


char stringDateTime[30];

char* getFormattedTimeStamp() {
    DateAndTime date_and_time;
    struct timeval tv;
    struct tm *tm;

    gettimeofday(&tv, NULL);

    tm = gmtime(&tv.tv_sec);

    // Add 1900 to get the right year value
    // read the manual page for localtime()
    date_and_time.year = tm->tm_year + 1900;
    // Months are 0 based in struct tm
    date_and_time.month = tm->tm_mon + 1;
    date_and_time.day = tm->tm_mday;
    date_and_time.hour = tm->tm_hour;
    date_and_time.minutes = tm->tm_min;
    date_and_time.seconds = tm->tm_sec;
    date_and_time.msec = (int) (tv.tv_usec / 1000);
    sprintf(stringDateTime,"%04d-%02d-%02dT%02d:%02d:%02d.%03d",
        date_and_time.year,
        date_and_time.month,
        date_and_time.day,
        date_and_time.hour,
        date_and_time.minutes,
        date_and_time.seconds,
        date_and_time.msec
    );
    return stringDateTime;

}

char *createJsonLog(char* level, char* logMessage, char* key, char* value){
    cJSON *json= NULL;
    cJSON *log= NULL;
    int i=0;
    char keyInfo[strlen(key)];
    stpcpy(keyInfo,key);
    while (keyInfo[i]){
        keyInfo[i]=toupper(keyInfo[i]);
        i++;
    }

    json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "terminalId", terminalId);
    cJSON_AddStringToObject(json, "@timestamp", getFormattedTimeStamp());
    cJSON_AddStringToObject(json, keyInfo, value);
    cJSON *args = cJSON_CreateArray();
    cJSON *terminalArg=cJSON_CreateString(terminalId);
    cJSON *keyArg=cJSON_CreateString(keyInfo);

    cJSON_AddItemToArray(args,terminalArg);
    cJSON_AddItemToArray(args,keyArg);
    cJSON_AddItemToObject(json,"args",args);

    cJSON_AddItemToObject(json, "log", log = cJSON_CreateObject());
    cJSON_AddStringToObject(log, "level", level);
    cJSON_AddStringToObject(log, "message", logMessage);
    char  *result=cJSON_Print(json);
    cJSON_Delete(json);
    return result;
}

void systemLog(char* logType, char* message, char* key, char* value){
    char *indexDate=getIndexDate();
    char urlString[200];
    sprintf(urlString,"https://3.122.113.110:9200/elbet-terminals-%s/_doc",indexDate);

//    printf("%s\n",indexDate);
    char *json_log=createJsonLog(logType,message,key,value);
    postRequest(urlString,json_log);
    free(json_log);
}

