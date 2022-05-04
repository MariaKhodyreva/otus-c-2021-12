#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "curl/curl.h"
#include <json-c/json.h>
#include <time.h>

/* 
Для работы были скачаны библиотеки (для Ubuntu):
    libjson-c4
    libjson-c-dev

    libcurl4
    libcurl4-openssl-dev
*/

//#define DEBUG

#define LOCATION_INFO_REQUEST "https://www.metaweather.com/api/location/search/?query=" // запрос для получения ID локации
#define WEATHER_INFO_REQUEST "https://www.metaweather.com/api/location/" // запрос для получения информации о погоде по заданной локации

#define LOCATION_ID_KEY "woeid" // ID локации
#define WEATHER_STATE_KEY "weather_state_name"
#define WIND_DIRECTION_KEY "wind_direction_compass"
#define WIND_SPEED_KEY "wind_speed"
#define MIN_TEMP_KEY "min_temp"
#define MAX_TEMP_KEY "max_temp"

struct memory {
    char* response;
    size_t size;
};

struct memory location_info = {0};
struct memory weather_info = {0};


/* Прототипы функций  */

size_t write_location_callback(void *data, size_t size, size_t nmemb, void *stream);
static size_t write_weather_callback(void *data, size_t size, size_t nmemb, void *stream);

char* concat(char *s1, char *s2);
char* get_id(char* info);
char* get_current_day();
char* get_weather_description(char* info);


int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Укажите локацию. Например, Moscow или London.\n"); 
        return 0;
    
    } else {

        CURL* curl = curl_easy_init();
        if(curl == NULL) {
          printf("Не удалось инициализировать CURL.\n");
          return 0;
        }

        /* Отправляем GET-запрос для получения ID, соответствующего заданной локации */

        char* request_string = concat(LOCATION_INFO_REQUEST, argv[1]);
        
        curl_easy_setopt(curl, CURLOPT_URL, request_string);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_location_callback);
        CURLcode result_code = curl_easy_perform(curl);

        #ifdef DEBUG
            printf("Запрос был отправлен по адресу: %s\n", request_string);
        #endif
        
        free(request_string);
        request_string = NULL;
        
        if (result_code != CURLE_OK) {
            printf("Не удалось получить ответ от веб-сервера. Код ошибки: %d. Описание можно посмотреть в заголовочном файле <curl.h>\n", result_code);
            return 0;
        }
        

        char* location_id = get_id(location_info.response);
        
        free(location_info.response);
        location_info.response = NULL;
        
        if (location_id == NULL) {
            printf("Вероятно, Вы ввели некорректное наименование локации.\n"
                    "Допустимые наименования можете посмотреть на сайте https://www.metaweather.com\n");
            return 0;
        }
        
        
        /* Отправляем GET-запрос для получения информации о погоде по заданному ID локации */
        
        request_string = concat(WEATHER_INFO_REQUEST, location_id); 
        
        free(location_id);
        location_id = NULL;
        
        char* current_day = get_current_day();
        request_string = concat(request_string, current_day);
        
        free(current_day);
        current_day = NULL;
        
        curl_easy_setopt(curl, CURLOPT_URL, request_string);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_weather_callback);
        result_code = curl_easy_perform(curl); 
        
        #ifdef DEBUG
            printf("Запрос был отправлен по адресу: %s\n", request_string);
        #endif
        
        free(request_string); 
        request_string = NULL;
        
        if (result_code != CURLE_OK) {
            printf("Не удалось получить ответ от веб-сервера. Код ошибки: %d. Описание можно посмотреть в заголовочном файле <curl.h>\n", result_code);
            return 0;
        }        
        
        char* weather_description = get_weather_description(weather_info.response);
        
        free(weather_info.response);
        weather_info.response = NULL;
        
        if (weather_description == NULL) {
            printf("Для заданной локации отсутствует информация о погоде на сегодня.");
            return 0;
        }
        
        printf("По локации %s погода на сегодня: \n\n%s\n\n", argv[1], weather_description);

        free(weather_description);
        weather_description = NULL;

    }
}

char* concat(char *s1, char *s2) {

    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);                      

    char *result = malloc(len1 + len2 + 1);
    result[len1 + len2] = 0;

    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2);   

    return result;
}

char* get_current_day() {
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    char* output = malloc(11);
    
    sprintf(output, "/%d/%d/%d/", timeinfo->tm_year + 1900,
            timeinfo->tm_mon + 1,
            timeinfo->tm_mday);
            
    return output;
}


char* get_id(char* info) {

    char* location_id = NULL;
    
    struct json_object* object = json_tokener_parse(info);

    if (object) {

        struct json_object* first_element = json_object_array_get_idx(object, 0);
        
        struct json_object* object_id;
        int exists = json_object_object_get_ex(first_element, "woeid", &object_id);
        
        if (exists) {
            size_t l = strlen(json_object_get_string(object_id));
            location_id = malloc(l + 1);
            memcpy(location_id, json_object_get_string(object_id), l);
            location_id[l] = 0;
        } 
    } 
    return location_id;
}


size_t write_weather_callback(void* ptr, size_t size, size_t nmemb, void* stream) {

   size_t this_size = size * nmemb;
   size_t data_length = weather_info.size;
   weather_info.size = weather_info.size + this_size;
   weather_info.response = realloc(weather_info.response, weather_info.size);
   
   memcpy((char*)weather_info.response + data_length, ptr, this_size);

   return this_size;
}


size_t write_location_callback(void* ptr, size_t size, size_t nmemb, void *stream) {

   size_t this_size = size * nmemb;
   size_t data_length = location_info.size;
   location_info.size = location_info.size + this_size;
   location_info.response = realloc(location_info.response, location_info.size);
   
   memcpy((char*)location_info.response + data_length, ptr, this_size);
    
   return this_size;
} 


char* get_weather_description(char* info) {

    char* description = NULL;
    
    struct json_object* object = json_tokener_parse(info);
    
    if (object) {

        struct json_object* first_element = json_object_array_get_idx(object, 0);
        
        struct json_object* state_name;
        json_object_object_get_ex(first_element, "weather_state_name", &state_name);
        
        struct json_object* direction_compass;
        json_object_object_get_ex(first_element, "wind_direction_compass", &direction_compass);
        
        struct json_object* wind_speed;
        json_object_object_get_ex(first_element, "wind_speed", &wind_speed);
        
        struct json_object* min_temp;
        json_object_object_get_ex(first_element, "min_temp", &min_temp);
        
        struct json_object* max_temp;
        json_object_object_get_ex(first_element, "max_temp", &max_temp);
        
        size_t l = strlen(json_object_get_string(first_element));
        description = calloc(l + 1, 1); // берем размер с запасом
        
        sprintf(description, "Как в целом: %s\nНаправление ветра: %s\nСкорость ветра: %s\nДиапазон температуры: от %s до %s\n", 
                json_object_get_string(state_name),
                json_object_get_string(direction_compass),
                json_object_get_string(wind_speed),
                json_object_get_string(min_temp),
                json_object_get_string(max_temp)
            );
    }
    return description;
}
