#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ERROR_STRING "Структура архива нарушена.\n"
//#define DEBUG

/* Центральный заголовок */
typedef struct {
        size_t filenameLength; // длина имени файла
        size_t localFileHeaderOffset; // байт смещения локального заголовка от начала ZIP
        size_t fileNameSum; // контрольная сумма имени файла для проверки
} cdHeader;

/* Маркеры окончания jpeg */
const short END_IMAGE_1 = 255;
const short END_IMAGE_2 = 217;

/* Информация для навигации по ZIP-файлу */

const short P = 80;
const short K = 75;

const short EOCDR_ENTRIES = 10; // смещение от начала EOCDR для получения количества центральных заголовков
const short CD_ENTRIES_BYTES_COUNT = 2; // кол-во байт для прочтения количества центральных заголовков

const short EOCDR_CD_OFFSET = 16; // смещение от начала EOCDR для получения смещения от начала ZIP центрального каталога
const short CD_ADDRESS_BYTES_COUNT = 4; // кол-во байт для прочтения смещения центрального каталога

const short CD_LK_HEADER_OFFSET_OFFSET = 42; // смещение от начала Центрального заголовка для получения смещения от начала ZIP локального заголовка
const short LK_ADDRESS_BYTES_COUNT = 4; // кол-во байт для прочтения смещения локального заголовка

const short CD_FILE_NAME_LENGTH_OFFSET = 28; // смещение от начала Центрального заголовка для получения длины имени файла 
const short LK_FILE_NAME_LENGTH_OFFSET = 26; // смещение от начала Локального заголовка для получения длины имени файла
const short LK_NAME_LENGTH_BYTES_COUNT = 2; // кол-во байт для прочтения длины имени файла

const short CD_FILE_NAME_OFFSET = 46; // смещение от начала Центрального заголовка для получения имени файла 
const short LK_FILE_NAME_OFFSET = 30; // смещение от начала Локального заголовка для получения имени файла


void displayBits(uint8_t value);
size_t getPtrAfterImage(uint8_t *bytePtr, int arraySize);
size_t getCDPtr(uint8_t *bytePtr);
size_t binToDec(uint8_t *numbers, size_t arraySize);
void checkLkHeader(cdHeader h, uint8_t *bytePtr);
void binToCharArray(uint8_t *numbers, size_t arraySize, char *fileName);
size_t twoPower(size_t p);

int main(int argc, char *argv[]) {

    size_t fileSize = 4096; // Предполагаемый размер файла в байтах
    uint8_t *byteArray; // сюда будем копировать содержимое файла
    size_t bytesRead = 0;

    byteArray = malloc(fileSize);
     
    FILE *zipFile = fopen(argv[1], "ab+"); // "zipjpeg.jpg"

    if (zipFile != NULL) {
    
         while (feof(zipFile) == 0) {
        
            if ((fileSize - bytesRead) == 0) { // если в предыдущей итерации удалось прочитать всё кол-во байтов из оставшихся по заданному fileSize
                fileSize *= 2;
                byteArray = realloc(byteArray, fileSize); // увеличиваем предполагаемый размер файла
            } 

            bytesRead += fread(&byteArray[bytesRead], 1, fileSize - bytesRead, zipFile);
        } 

        fclose(zipFile);
        
        size_t startAfterImage = getPtrAfterImage(byteArray, bytesRead);
        
        #ifdef DEBUG
            printf("Количество прочитанных байтов: %ld\n", bytesRead);
            printf("Начало сразу после картинки: %lu\n", startAfterImage);
        #endif
        
        if (startAfterImage != 0) { // Есть ли в файле ещё какие-то байты после jpeg ?
        
            uint8_t *zipArrayPtr = &byteArray[startAfterImage]; // указатель на массив байтов ZIP-файла
            
            size_t newSizeArray = bytesRead - startAfterImage;
            cdHeader cdHeaderArray[newSizeArray / sizeof(cdHeader)]; // возьмем максимально возможное кол-во структур (центральных заголовков) до конца файла
            size_t cdHeaderCount = 0;
            size_t eorcdrCDEntries = 0;
            size_t eorcdrCDOffset;
            
            for (int i = 0; i < newSizeArray; i++) {
            
                if (zipArrayPtr[i] == P && zipArrayPtr[i + 1] == K && zipArrayPtr[i + 2] == 1 && zipArrayPtr[i + 3] == 2) { // центральный заголовок
                        
                    cdHeaderArray[cdHeaderCount].filenameLength = binToDec(&zipArrayPtr[i + CD_FILE_NAME_LENGTH_OFFSET], LK_NAME_LENGTH_BYTES_COUNT);
                    cdHeaderArray[cdHeaderCount].localFileHeaderOffset = binToDec(&zipArrayPtr[i + CD_LK_HEADER_OFFSET_OFFSET], LK_ADDRESS_BYTES_COUNT);

                    char fileNameArray[cdHeaderArray[cdHeaderCount].filenameLength];
                    binToCharArray(&zipArrayPtr[i + CD_FILE_NAME_OFFSET], cdHeaderArray[cdHeaderCount].filenameLength, fileNameArray);
                    cdHeaderArray[cdHeaderCount].fileNameSum = binToDec(&zipArrayPtr[i + CD_FILE_NAME_OFFSET], cdHeaderArray[cdHeaderCount].filenameLength);
                    
                    checkLkHeader(cdHeaderArray[cdHeaderCount], zipArrayPtr);
                    
                    printf("Имя файла: %s\n", fileNameArray);
                    
                    cdHeaderCount++;
                } 
            
                if (zipArrayPtr[i] == P && zipArrayPtr[i + 1] == K && zipArrayPtr[i + 2] == 5 && zipArrayPtr[i + 3] == 6) { // EOCDR
                
                    if (newSizeArray >= i + 22) {
                    
                        eorcdrCDEntries = binToDec(&zipArrayPtr[i + EOCDR_ENTRIES], CD_ENTRIES_BYTES_COUNT);
                        eorcdrCDOffset = binToDec(&zipArrayPtr[i + EOCDR_CD_OFFSET], CD_ADDRESS_BYTES_COUNT);
                        
                        #ifdef DEBUG
                            printf("Порядковый номер байта для EOCDR: %d\n", i);
                            printf("Смещение указателя на центральный каталог: %lu\n", eorcdrCDOffset);
                            printf("Количество записей в центральном каталоге: %lu\n", eorcdrCDEntries);
                        #endif
                    
                    } else {
                        printf(ERROR_STRING);
                        return 0;
                    }
                    break;
                }
            }
            
            if (eorcdrCDEntries == 0 || eorcdrCDEntries != 0 && eorcdrCDEntries != cdHeaderCount) {
                printf("Архив не найден или структура архива нарушена.\n");
                return 0;
             }
        } 

    } else {
        printf("Не удалось открыть файл для прочтения\n");
    } 
    
    free(byteArray);

}

void checkLkHeader(cdHeader h, uint8_t *bytePtr) { 

    short filenameLength = binToDec(&bytePtr[h.localFileHeaderOffset + LK_FILE_NAME_LENGTH_OFFSET], LK_NAME_LENGTH_BYTES_COUNT);
            
    if (filenameLength == h.filenameLength) {
    
        size_t fileNameSum = binToDec(&bytePtr[h.localFileHeaderOffset + LK_FILE_NAME_OFFSET], filenameLength);
        
        if (h.fileNameSum != fileNameSum) {
            printf(ERROR_STRING);
            return;
        } 
    } else {
        printf(ERROR_STRING);
        return;
    }
}


size_t binToDec(uint8_t *numbers, size_t arraySize) {

    size_t result = 0;
    uint8_t mask;
    short number;
    size_t p = 0;
    
    for (int bytes = 0; bytes <= arraySize - 1; bytes++) {
        
        mask = 1;
        for (int bits = 0; bits <= 7; bits++) {

            number = numbers[bytes] & mask ? 1 : 0;
            if (number == 1) {
                result += twoPower(p);
            }    
            mask <<= 1;
            p++; // степень двойки увеличивается в обратном порядке (у первого байта будет наименьшая степень, у последнего байта - наибольшая)
        } 
    } 
    
    return result;
}

size_t twoPower(size_t p) {

    if (p == 0) {
        return 1;
    }
    size_t result = 2;
    for (size_t i = 2; i <= p; i++) {
        result *= 2;
    }
    return result;
}


void binToCharArray(uint8_t *numbers, size_t arraySize, char *fileName) { // байты-символы сканируются в обратном порядке

    for (int bytes = arraySize - 1; bytes >= 0; bytes--) {
        fileName[bytes] = numbers[bytes];
    } 
}


size_t getPtrAfterImage(uint8_t *bytePtr, int arraySize) {

    for (int i = 0; i < arraySize; i++) {
        if (bytePtr[i] == END_IMAGE_1 && bytePtr[i + 1] == END_IMAGE_2) {
        
            if (i != (arraySize - 1)) { // после картинки что-то ещё есть?
                return i + 2;
            } 
            break;
        }
    }
    return 0;
}


/* Не используется, но может пригодиться при отладке */

void displayBits(uint8_t value) {

    unsigned int displayMask = 1 << 7;
    int count;
    
    for (count = 1; count <= 8; count++) {
    
        putchar(value & displayMask ? '1' : '0');
        
        value <<= 1;
        
        if (count % 8 == 0) {
            putchar(' ' );
        }
    }
    putchar('\n');
}

