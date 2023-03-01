#include <iostream>
#include <vector>
#include <string>
#include <pthread.h>
#include <memory.h>
#include <unistd.h>
#include <czmq.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include "cJSON.h"
#include "utils.h"
//#include "../../ServiceLogger/ServiceLogger/ServiceLogger.h"


pthread_mutex_t LogMutex;
pthread_mutex_t StringMutex;

using namespace std;

#define LOG_LINE_LEN 5200
char LogLine[LOG_LINE_LEN];

#define ZMQ_NETWORK_ADDRESS_LEN 500
char ZMQNetworkAddress[ZMQ_NETWORK_ADDRESS_LEN];

#define JSON_MESSAGE_FROM_SERVER_LEN 1000
char JsonMessageFromServer[JSON_MESSAGE_FROM_SERVER_LEN];

#define JSON_CONFIG_STORAGE_SIZE 1000
char json_config_storage[JSON_CONFIG_STORAGE_SIZE];

#define JSON_MESSAGE_TO_SERVER_LEN 1000
char JsonMessageToServer[JSON_MESSAGE_TO_SERVER_LEN];

char quotation_mark[ ] ={"\""};

int NUM_RETRIES;

int RESEND_MESSAGE_TIME;

pthread_t Keyboard_Thread;
int restrict_arg;


using namespace std;
//void ServiceLog(char *logType, char* message, char *key, char *value);
void KeyboardLog(char *logType, char* message, char *key, char *value)
{
    pthread_mutex_lock(&LogMutex);
    systemLog(logType, message, key, value);
/*    memset(LogLine,0,sizeof(char)*LOG_LINE_LEN);
    FILE *log_file=fopen("log.txt","a");
    if(log_file!=NULL) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(LogLine,"%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        strcat(LogLine,new_log);
        printf("%s\n\r",LogLine);
        fwrite(LogLine,sizeof(char),strlen(LogLine),log_file);
        fclose(log_file);
    }
    */
    pthread_mutex_unlock(&LogMutex);
}

#define KEYBOARD_STRING_MAX_LEN 500
char buf[KEYBOARD_STRING_MAX_LEN];
char buf2[KEYBOARD_STRING_MAX_LEN*2];

vector<string> KeyboardStrings;

#define UK "UNKNOWN"

#define ESCAPE(key) (key == KEY_ESC)

static const char *keycodes[] =
{
    "RESERVED", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
    "-", "=", "BACKSPACE", "TAB", "q", "w", "e", "r", "t", "y", "u", "i",
    "o", "p", "[", "]", "ENTER", "L_CTRL", "a", "s", "d", "f", "g", "h",
    "j", "k", "l", ";", "'", "`", "L_SHIFT", "\\", "z", "x", "c", "v", "b",
    "n", "m", ",", ".", "/", "R_SHIFT", "*", "L_ALT", "SPACE", "CAPS_LOCK",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUM_LOCK",
    "SCROLL_LOCK", "NL_7", "NL_8", "NL_9", "-", "NL_4", "NL5",
    "NL_6", "+", "NL_1", "NL_2", "NL_3", "INS", "DEL", UK, UK, UK,
    "F11", "F12", UK, UK,	UK, UK,	UK, UK, UK, "R_ENTER", "R_CTRL", "/",
    "PRT_SCR", "R_ALT", UK, "HOME", "UP", "PAGE_UP", "LEFT", "RIGHT", "END",
    "DOWN",	"PAGE_DOWN", "INSERT", "DELETE", UK, UK, UK, UK,UK, UK, UK,
    "PAUSE"
};

static const char *shifted_keycodes[] =
{
    "RESERVED", "ESC", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")",
    "_", "+", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T", "Y", "U", "I",
    "O", "P", "{", "}", "ENTER", "L_CTRL", "A", "S", "D", "F", "G", "H",
    "J", "K", "L", ":", "\"", "~", "L_SHIFT", "|", "Z", "X", "C", "V", "B",
    "N", "M", "<", ">", "?", "R_SHIFT", "*", "L_ALT", "SPACE", "CAPS_LOCK",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUM_LOCK",
    "SCROLL_LOCK", "HOME", "UP", "PGUP", "-", "LEFT", "NL_5",
    "R_ARROW", "+", "END", "DOWN", "PGDN", "INS", "DEL", UK, UK, UK,
    "F11", "F12", UK, UK,	UK, UK,	UK, UK, UK, "R_ENTER", "R_CTRL", "/",
    "PRT_SCR", "R_ALT", UK, "HOME", "UP", "PAGE_UP", "LEFT", "RIGHT", "END",
    "DOWN",	"PAGE_DOWN", "INSERT", "DELETE", UK, UK, UK, UK,UK, UK, UK,
    "PAUSE"
};

#define SHIFT(key)  ((key == KEY_LEFTSHIFT) || (key == KEY_RIGHTSHIFT))

#define KEYBOARD_JSON_FILE_LEN 1000
char keyboard_json_file[KEYBOARD_JSON_FILE_LEN];

char keyboard[KEYBOARD_JSON_FILE_LEN];

int shift_flag=0;

bool file_open;

void LoadAndParseKeyboardJson(void)
{
    FILE *jd;
    jd=fopen("Keyboard.config","rb");
    if(jd!=NULL) {
        memset(keyboard_json_file,0,sizeof(char)*KEYBOARD_JSON_FILE_LEN);
        fread(keyboard_json_file,1,sizeof(char)*KEYBOARD_JSON_FILE_LEN,jd);
        fclose(jd);
        cJSON *root=cJSON_Parse(keyboard_json_file);
        if(root!=NULL) {
            cJSON *KeyboardAddress=cJSON_GetObjectItem(root,"KeyboardAddress");
            if(KeyboardAddress!=NULL)
                if(KeyboardAddress->valuestring!=NULL) {
                    memset(keyboard,0,sizeof(char)*KEYBOARD_JSON_FILE_LEN);
                    strcpy(keyboard,KeyboardAddress->valuestring);
                }
        }
        cJSON_Delete(root);
    }

}

void* Keyboard_ThreadLoop(void)
{

    bool ok=false;
    struct input_event event;
    ssize_t n;
    int fd;

    int c;

    LoadAndParseKeyboardJson();

    file_open=false;

    fd = open(keyboard, O_RDONLY );
    while(!file_open) {
        if (fd == -1) {
            fprintf(stderr, "Cannot open %s: %s.\n", keyboard, strerror(errno));
            sleep(3);
            fd = open(keyboard, O_RDONLY );
        }
        if(fd>=0) file_open=true;
    }

    while(true) {

    ok=false;
    memset(buf,0,sizeof(char)*KEYBOARD_STRING_MAX_LEN);
    c=0;
    while(!ok) {
        if(read(fd, &event, sizeof(event))<0)
        {
            LoadAndParseKeyboardJson();
            file_open=false;
            fd = open(keyboard, O_RDONLY );
            while(!file_open) {
            if (fd == -1) {
                fprintf(stderr, "Cannot open %s: %s.\n", keyboard, strerror(errno));
                sleep(3);
                fd = open(keyboard, O_RDONLY );
            }
            if(fd>=0) file_open=true;
            }

        }
        /* If a key from the keyboard is pressed */
        if (event.type == EV_KEY && event.value == 1) {
            if (ESCAPE(event.code))
                {
                }

            if (SHIFT(event.code))
                shift_flag = event.code;

            if (shift_flag && !SHIFT(event.code)) {
                    if(strstr(shifted_keycodes[event.code],"ENTER")) {
                    }
                    else {
                            if(strstr(shifted_keycodes[event.code],"DOWN")) {ok=true;}
                            else {
                                buf[c]=*shifted_keycodes[event.code];
                                c++;
                                }
                         }
            }
            else if (!shift_flag && !SHIFT(event.code)) {
                    if(strstr(keycodes[event.code],"ENTER")) { }
                    else {
                        if(strstr(keycodes[event.code],"DOWN")) {ok=true;}
                        else {
                        buf[c]=*keycodes[event.code];
                        c++;
                        }
                    }
                }

        }
        else {
            /* If a key from the keyboard is released */
            if (event.type == EV_KEY && event.value == 0)
                if (SHIFT(event.code))
                    shift_flag = 0;
        }

     }
        pthread_mutex_lock(&StringMutex);
        KeyboardStrings.push_back(string(buf));
        memset(&buf2,0,sizeof(char)*KEYBOARD_STRING_MAX_LEN*2);
        sprintf(buf2,"Scan:%s",buf);
        KeyboardLog("INFO",buf2,"BARCODE READER","operation");
        pthread_mutex_unlock(&StringMutex);
        usleep(50000);

    }
 close(fd);
}
//ServiceLog(char *logType, char* message, char *key, char *value);
void MainThreadLog(char *logType, char* message, char *key, char *value)
{
    pthread_mutex_lock(&LogMutex);
    systemLog(logType, message, key, value);
    /*
    memset(LogLine,0,sizeof(char)*LOG_LINE_LEN);
    FILE *log_file=fopen("log.txt","a");
    if(log_file!=NULL) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(LogLine,"%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        strcat(LogLine,new_log);
        printf("%s\n\r",LogLine);
        fwrite(LogLine,sizeof(char),strlen(LogLine),log_file);
        fclose(log_file);
    }
    */
    pthread_mutex_unlock(&LogMutex);
}


int LoadAndParseNetworkConfigJson(void)
{
    memset(json_config_storage,0,sizeof(char)*JSON_CONFIG_STORAGE_SIZE);
    FILE *config=fopen("Network.config","rb");
    if(config!=NULL) {
        fread(json_config_storage,sizeof(char),ZMQ_NETWORK_ADDRESS_LEN,config);
        cJSON *root=cJSON_Parse(json_config_storage);
        if(root!=NULL) {
            cJSON *NetworkAddress=cJSON_GetObjectItem(root,"NetworkAddress");
            if(NetworkAddress!=NULL) {
                if(NetworkAddress->valuestring!=NULL) {
                    strcpy(ZMQNetworkAddress,NetworkAddress->valuestring);
                  //  printf("%s\n",NetworkAddress->valuestring);
                }
            }
            cJSON *ResendMessageAfterMiliSeconds=cJSON_GetObjectItem(root,"ResendMessageAfterMiliSeconds");
            if(ResendMessageAfterMiliSeconds!=NULL) {
                RESEND_MESSAGE_TIME=ResendMessageAfterMiliSeconds->valueint;
            }
            cJSON *NumberOfRetries=cJSON_GetObjectItem(root,"NumberOfRetries");

            if(NumberOfRetries!=NULL) {
                NUM_RETRIES=NumberOfRetries->valueint;
                //printf("Num ret:%d\n",NUM_RETRIES);
            }
            cJSON_Delete(root);
        }
        fclose(config);
        MainThreadLog("INFO","Network config json ok.","ZMQ","config");
        return 1;
    }
    else {
        MainThreadLog("INFO","Network config json file not found!","ZMQ","config");
        return 0;
    }
}

string New_Keyboard_String;

int main()
{

  //  SetCURLString("curl -PUT -u 'elbet:gujmyh-fUspo6-goxzeb' -H 'Content-Type: application/json' 'https://3.122.113.110:9200/elbet-terminals-'%s'/_doc' -d '%s' --insecure &");

    struct tm t1;
    time_t vreme=timelocal(&t1);
    srand(vreme);
    unsigned int MessageID;

    LoadAndParseNetworkConfigJson();
    pthread_mutex_init(&LogMutex,NULL);
    pthread_mutex_init(&StringMutex,NULL);
    pthread_create(&Keyboard_Thread,NULL,&Keyboard_ThreadLoop,(void*)restrict_arg);

    bool new_keyboard_string=false;
    MainThreadLog("INFO","Scanner started.","BARCODE READER","operation");
    while(true) {
        new_keyboard_string=false;
        usleep(50000);
        pthread_mutex_lock(&StringMutex);
        if(KeyboardStrings.size()>0) {
  //          system("clear");
//            printf("%s\n",KeyboardStrings[0].c_str());
         //   printf("size:%d\n",KeyboardStrings.size());
            New_Keyboard_String=KeyboardStrings[0];
            KeyboardStrings.erase(KeyboardStrings.begin());
            new_keyboard_string=true;
        }
        pthread_mutex_unlock(&StringMutex);
        if(new_keyboard_string) {
//            printf("new keyboard string\n");
            int c=0;
            bool ok=false;
//            printf("pre rand\n");
            MessageID=rand();
  //          printf("post rand\n");
    //        printf("NUM RETRIES:%d\n",NUM_RETRIES);
            while((c<NUM_RETRIES)&&(!ok)) {
      //          printf("pre context\n");
                void *context = zmq_ctx_new ();
                void *requester = zmq_socket (context, ZMQ_REQ);
        //        printf("pre connect\n");
                zmq_connect (requester, ZMQNetworkAddress);

                memset(JsonMessageToServer,0,sizeof(char)*JSON_MESSAGE_TO_SERVER_LEN);
                strcat(JsonMessageToServer,"{ ");
                strcat(JsonMessageToServer,quotation_mark);
                strcat(JsonMessageToServer,"ScannedCode");
                strcat(JsonMessageToServer,quotation_mark);
                strcat(JsonMessageToServer,": { ");
                strcat(JsonMessageToServer,quotation_mark);
                strcat(JsonMessageToServer,"messageId");
                strcat(JsonMessageToServer,quotation_mark);
                strcat(JsonMessageToServer," : ");

                char temp_chars[100];
                memset(temp_chars,0,sizeof(char)*100);
                sprintf(temp_chars,"%d , ",MessageID);
                strcat(JsonMessageToServer,temp_chars);
                strcat(JsonMessageToServer,quotation_mark);
                strcat(JsonMessageToServer,"scannedString");
                strcat(JsonMessageToServer,quotation_mark);
                strcat(JsonMessageToServer," : ");
                strcat(JsonMessageToServer,quotation_mark);
                strcat(JsonMessageToServer,New_Keyboard_String.c_str());
                strcat(JsonMessageToServer,quotation_mark);
                strcat(JsonMessageToServer," } } ");

//                printf("%s\n",JsonMessageToServer);

                zmq_send (requester, JsonMessageToServer,strlen(JsonMessageToServer),0);

                int message_time=0;
                while(message_time<RESEND_MESSAGE_TIME) {
//                printf("RESEND TIME :%d\n",RESEND_MESSAGE_TIME);
                    if(zmq_recv (requester, JsonMessageFromServer, JSON_MESSAGE_FROM_SERVER_LEN, ZMQ_NOBLOCK)>0)
                    {
    //                    printf("%s\n",JsonMessageFromServer);
                        message_time=RESEND_MESSAGE_TIME;
                        cJSON *root=cJSON_Parse(JsonMessageFromServer);
                        if(root!=NULL) {
    //                        printf("parsed ok\n");
                            cJSON *ScannedCode=cJSON_GetObjectItem(root,"ScannedCode");
                            if(ScannedCode!=NULL) {
      //                          printf("Scanned ok\n");
                                cJSON *messageId=cJSON_GetObjectItem(ScannedCode,"messageId");
                                if(messageId!=NULL) {
                                    unsigned int mess_id=messageId->valueint;
                                    if(mess_id==MessageID) {
                                        cJSON *status=cJSON_GetObjectItem(ScannedCode,"status");
                                        if(status!=NULL) {
                                            if(status->valueint==1) {
                                                ok=true;
                                                new_keyboard_string=false;
                                                MainThreadLog("INFO","Scanned string sent.","BARCODE READER","zmq");
                                            }
                                            else {
                                                char temp_log[500];
                                                memset(temp_log,0,sizeof(char)*500);
                                                sprintf(temp_log,"Received status %d for ScannedCode request. Message ID %d ",status->valueint,mess_id);
                                                MainThreadLog("ERROR",temp_log,"BARCODE READER","zmq");
                                            }
                                        }
                                    }
                                }
                            }
                            cJSON_Delete(root);
                        }

                    }
                    usleep(100000);
                    message_time+=100;
                }
//                printf("RETRY\n");

                zmq_close (requester);
                zmq_ctx_destroy (context);
              //  printf("network down\n");
                c++;
                if(c==NUM_RETRIES)  {
                    sleep(3);
                    //exit(0);
                   // printf("END OF RETRIES\n");
                    char temp_log[500];
                    memset(temp_log,0,sizeof(char)*500);
                    sprintf(temp_log,"[CRITICAL] can not send Scanned Code %s",JsonMessageToServer);
                    MainThreadLog("ERROR","Can Not Send Scanned Code!","SCANNER","zmq");
                    LoadAndParseNetworkConfigJson();
                }
            }

        }

    }

    return 0;
}
