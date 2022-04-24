#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>

//#define DEBUG
#define WINDOWS_1251 "Windows-1251"
#define KOI_8_R "KOI8-R"
#define ISO_8859_5 "ISO_8859-5"

/* Из вторых частей таблиц кодировок для проверки взята ТОЛЬКО ЧАСТЬ символов после 128, т. к. это тестовое задание и в реальности программа использоваться не будет 
   Остальные символы будут заменяться на символ подчеркивания.
*/
#define WINDOWS_1251_CIRILLIC_COUNT  73 
#define KOI_8_R_CIRILLIC_COUNT  66
#define ISO_8859_5_CIRILLIC_COUNT  65

FILE *inputFile;
FILE *outputFile;

struct stat inputFileInfo;
struct stat outputFileInfo;

typedef struct {
        uint8_t fromCode; // код исходной кодировки
        uint16_t toCode; // код кодировки UTF-8

} CodeMap;


/* Прототипы функций */

void encodeToFile_UTF_8(char *encoding);
void writeToFile(uint8_t *originArray, size_t originArraySize, CodeMap *cyrillicCodes, size_t cyrillicSize);
void bitsToChars(uint8_t value, char* charArray);

void recodeFrom_KOI_8(uint8_t *originArray, size_t originArraySize, uint8_t *targetArray, size_t targetArraySize);
void recodeFrom_ISO_8859_5(uint8_t *originArray, size_t originArraySize, uint8_t *targetArray, size_t targetArraySize);
void displayBits(uint8_t value, char* charArray);


int main(int argc, char *argv[]) {

    if (argc < 4) {
        printf("Укажите входной файл, заданную кодировку и выходной файл через пробелы.\n");
        return 0;
    
    } else {

        inputFile = fopen(argv[1], "rb"); 
        outputFile = fopen(argv[3], "wb");

        if (inputFile == NULL || outputFile == NULL) {

            printf("Не удалось открыть файлы. Проверьте правильность указания наименований и права доступа второго файла для записи.\n");
            return 0;
            
        } else {
            int fileDescriptor = fileno(inputFile);
        
            if(fstat(fileDescriptor, &inputFileInfo) != 0) {
            
                printf("Что-то не так с входным файлом. Попробуйте запустить программу ещё раз.\n");
                return 0;

            } else if (inputFileInfo.st_size == 0) {
            
                printf("Входной файл пустой.\n");
                return 0;
            } 

            encodeToFile_UTF_8(argv[2]); // Перекодирование содержимого первого файла в UTF-8 и его запись во второй файл

            
            fclose(inputFile);
            fclose(outputFile); 
        }
    }   
}     
 

void encodeToFile_UTF_8(char *encoding) {

    uint8_t *originArray = malloc(inputFileInfo.st_size);
    size_t bytesRead = fread(originArray, 1, inputFileInfo.st_size, inputFile);
                
    if (strcmp(encoding, WINDOWS_1251) == 0) {
    
        CodeMap cyrillicCodes[WINDOWS_1251_CIRILLIC_COUNT] = { 
            {132, 8222}, // „
            {147, 8220}, // “
            {148, 8221}, // ”
            {168, 1025}, // Ё
            {184, 1105}, // ё
            {185, 8470}, // №
            {187, 187}, // »
            {132, 8222} // „
            };
        
        /* Начиная с русской буквы А и до я кодовые точки совпадают в кодировках Windows-1251 и UTF-8*/

        for (size_t codeMapIndex = 8, fromCode = 192, toCode = 1040; 
            codeMapIndex < WINDOWS_1251_CIRILLIC_COUNT; 
            codeMapIndex++, fromCode++, toCode++) {
        
            cyrillicCodes[codeMapIndex].fromCode = fromCode;
            cyrillicCodes[codeMapIndex].toCode = toCode;
        }
    
        writeToFile(originArray, inputFileInfo.st_size, cyrillicCodes, WINDOWS_1251_CIRILLIC_COUNT);
        
   } else if (strcmp(encoding, ISO_8859_5) == 0) {
  
        CodeMap cyrillicCodes[ISO_8859_5_CIRILLIC_COUNT] = { 
            {161, 1025} // Ё
            };
            
        /* Начиная с русской буквы А и до я кодовые точки совпадают в кодировках ISO 8859-5 и UTF-8*/

        for (size_t codeMapIndex = 1, fromCode = 176, toCode = 1040; codeMapIndex < ISO_8859_5_CIRILLIC_COUNT; codeMapIndex++, fromCode++, toCode++) {
        
            cyrillicCodes[codeMapIndex].fromCode = fromCode;
            cyrillicCodes[codeMapIndex].toCode = toCode;
        }
        
        writeToFile(originArray, inputFileInfo.st_size, cyrillicCodes, ISO_8859_5_CIRILLIC_COUNT);
   
   } else if (strcmp(encoding, KOI_8_R) == 0) {
  
        CodeMap cyrillicCodes[KOI_8_R_CIRILLIC_COUNT] = { 
            {163, 1105}, // ё
            {179, 1025} // Ё
            };
        
        uint16_t livedCyrillicCodes[64] = {
            0x44E, 0x430, 0x431, 0x446, 0x434, 0x435, 0x444, 0x433, 0x445, 0x438, 0x439, 0x43A, 0x43B, 0x43C, 0x43D, 0x43E, 
            0x43F, 0x44F, 0x440, 0x441, 0x442, 0x443, 0x436, 0x432, 0x44C, 0x44B, 0x437, 0x448, 0x44D, 0x449, 0x447, 0x44A, 
            0x42E, 0x410, 0x411, 0x426, 0x414, 0x415, 0x424, 0x413, 0x425, 0x418, 0x419, 0x41A, 0x41B, 0x41C, 0x41D, 0x41E, 
            0x41F, 0x42F, 0x420, 0x421, 0x422, 0x423, 0x416, 0x412, 0x42C, 0x42B, 0x417, 0x428, 0x42D, 0x429, 0x427, 0x42A};


        for (size_t codeMapIndex = 2, fromCode = 192,  cirrilicIndex = 0; 
                codeMapIndex < KOI_8_R_CIRILLIC_COUNT; 
                codeMapIndex++, fromCode++, cirrilicIndex++) {
        
            cyrillicCodes[codeMapIndex].fromCode = fromCode;
            cyrillicCodes[codeMapIndex].toCode = livedCyrillicCodes[cirrilicIndex];
        }
        
        writeToFile(originArray, inputFileInfo.st_size, cyrillicCodes, KOI_8_R_CIRILLIC_COUNT);
   
   } else {
        printf("Укажите кодировку в формате 'Windows-1251', 'KOI8-R' или 'ISO_8859'. Другие кодировки эта программа не поддерживает.\n");
        
        free(originArray);
        return;
   }        
   
   free(originArray);
} 
 

void writeToFile(uint8_t *originArray, size_t originArraySize, CodeMap *cyrillicCodes, size_t cyrillicSize) {

    uint8_t targetArray[originArraySize * 2]; // берем размер с запасом на случай, если все символы будут иметь размер 2 байта

    size_t targetArrayIndex = 0;
    unsigned short found = 0;
    for (size_t originArrayIndex = 0; originArrayIndex < originArraySize; originArrayIndex++) {
    
        if (originArray[originArrayIndex] <= 127) { // проверяем, должен ли код символа соответствовать кодировке ASCII
            targetArray[targetArrayIndex] = originArray[originArrayIndex];
            targetArrayIndex++; // символ занял всего 1 байт
            continue;
        }

        found = 0;
        for (size_t codeMapIndex = 0; codeMapIndex < cyrillicSize; codeMapIndex++) {
        
            if (originArray[originArrayIndex] == cyrillicCodes[codeMapIndex].fromCode) {
                
                uint8_t firstByte = (uint8_t)(cyrillicCodes[codeMapIndex].toCode >> 6); // оставляем 5 первых значащих цифр
                firstByte = firstByte | 192; // вместо первых 3-х нулей устанавливаем "110"
                
                uint8_t secondByte = (uint8_t)cyrillicCodes[codeMapIndex].toCode << 2; // обнуляем первые 2 элемента
                secondByte = secondByte >> 2; // обнуляем первые 2 элемента
                secondByte = secondByte | 128; // вместо первых 2-х нулей устанавливаем "10"

                #ifdef DEBUG
                    printf("Число русского символа: %d\n", originArray[originArrayIndex]);
                    printf("В какое число по UTF-8 должно быть переведено: %d\n", cyrillicCodes[codeMapIndex].toCode);
                    char majorBytes[9];
                    char childBytes[9];
                    bitsToChars(firstByte, majorBytes);
                    bitsToChars(secondByte, childBytes);

                    printf("Первый байт числа после преобразования в UTF-8 - %s\n", majorBytes);
                    printf("Второй байт числа после преобразования в UTF-8 - %s\n", childBytes);
                    
                    return; // Проверяем только первый русский символ
                #endif
                
                targetArray[targetArrayIndex] = firstByte; 
                targetArray[targetArrayIndex + 1] = secondByte;
                targetArrayIndex += 2; // символ занял 2 байта
                found = 1;
                break;
            }
        }
        
        if (found == 0) {
            targetArray[targetArrayIndex] = 95; // если полученный символ не был задан в cyrillicCodes - по умолчанию устанавливаем символ подчеркивания
            targetArrayIndex++;
        } 
    }

    /* Если targetArray был заполнен не до конца, создаем новый массив размером меньше, чтобы не записывать в файл "мусор" */

    if (targetArrayIndex < originArraySize * 2) {
    
        uint8_t *newTargetArray = malloc(targetArrayIndex);
        
        for (size_t i = 0; i < targetArrayIndex; i++) {
            newTargetArray[i] = targetArray[i];
        }
        
        int writed = fwrite(newTargetArray, 1, targetArrayIndex, outputFile); 
        free(newTargetArray);
        
        if (writed != targetArrayIndex) {
            
            printf("Не удалось корректно записать данные во второй файл. Попробуйте запустить программу повторно.\n");
            return;
        } 
    }
    
    printf("Файл успешно перезаписан в кодировке UTF-8\n"); 
    printf("Размер нового файла - %lu байтов\n", targetArrayIndex); 
    
    
} 
 

/* Не используется, но может пригодиться при отладке */

void bitsToChars(uint8_t value, char* charArray) {

    unsigned int displayMask = 1 << 7;
    int count;
    
    for (int count = 1, charIndex = 0; count <= 8; count++, charIndex++) {
    
        charArray[charIndex] = value & displayMask ? '1' : '0';
        value <<= 1;
    }
    charArray[8] = 10;
}


