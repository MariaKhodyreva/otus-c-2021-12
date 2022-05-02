#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>

//#define DEBUG

#define WORD_SIZE 30 // какого размера можно ввести слово для проверки его повторений в файле

FILE* input_file;
struct stat input_file_info;

struct word_node {
        size_t word; // самма чисел байтов, составляющих слово
        size_t count; // количество повторений слова в файле
        struct word_node* next; 
        
};

struct word_node* hash_table[100] = {0}; // размер 100 был взят без какого-либо расчета

/* Прототипы функций  */

void create_hash_table(const uint8_t* const byte_array, size_t const array_size);
void create_list_nodes(struct word_node* next_node, struct word_node* const node_for_compare);
unsigned short get_index(size_t const word_number);

size_t get_word_number(char* symbol_bytes);
size_t get_count(size_t const word_number);
size_t get_count_from_list(const struct word_node* const next_node, size_t const word_number);

void displayBits(uint8_t value);


int main(int argc, char* argv[]) {

    /* Предполагается, что файл будет в кодировке UTF-8. Программа корректно может обработать текст, если буквы будут занимать не более 2-х байтов. */

    if (argc < 2) {
        printf("Укажите файл для проверки. Файл должен быть в кодировке UTF-8. Допускается только латиница и кириллица.\n"); 
        return 0;
    
    } else {

        input_file = fopen(argv[1], "rb"); 

        if (input_file == NULL) {

            printf("Не удалось открыть файл. Проверьте правильность указания наименования и права доступа файла для чтения.\n");
            return 0;
            
        } else {
            int file_descriptor = fileno(input_file);
        
            if(fstat(file_descriptor, &input_file_info) != 0) {
            
                printf("Что-то не так с входным файлом. Попробуйте запустить программу ещё раз.\n");
                return 0;

            } else if (input_file_info.st_size == 0) {
            
                printf("Входной файл пустой.\n");
                return 0;
            } 

            uint8_t* byte_array = malloc(input_file_info.st_size + 1); 
            size_t bytes_read = fread(byte_array, 1, input_file_info.st_size, input_file);
            
            if (bytes_read != input_file_info.st_size) {
                printf("Не удалось прочитать файл полностью. Попробуйте запустить программу ещё раз.\n");
                return 0;
            }
            printf("Файл был успешно прочитан.\n");
            fclose(input_file);
            byte_array[input_file_info.st_size] = '\n'; // добавляем пробельный символ в конец файла, чтобы выделить последнее слово
            
            create_hash_table(byte_array, bytes_read + 1); // заполняем хэш-таблицу словами из файла

            printf("Введите, пожалуйста, слово для проверки (допускается не более 30 символов).\n");
            
            char symbol_bytes[WORD_SIZE + 1] = {0};
            scanf("%s[^ \n]", symbol_bytes);

            size_t word_number = get_word_number(symbol_bytes); // вычисляем контрольную сумму слова
            
            printf("Слово встречается %lu раз(а).\n", get_count(word_number)); 

            free(byte_array);

        }
    }   
}

/* Хэш-функция: определяет индекс в массиве по контрольной сумме слова */
/* Наверняка, это не самая хорошая хэш-функция и будет много коллизий, */
    /* однако я сделала самую простую, т. к. написать хорошую хэш-функцию не уверена, что смогу, и не уверена, что в домашнее задание это входит */

unsigned short get_index(size_t const word_number) {
    return word_number % 100;
}


/* Заполняем хэш-таблицу на основе прочитанного файла */
/* В нашем случае хэш-таблица - это массив с указателями на структуры. В результате коллизий элементы будут добавляться в связанные списки по заданному индексу массива */

void create_hash_table(const uint8_t* const byte_array, size_t const array_size) {

    /* По каждому слову файла подсчитываем сумму его чисел, где каждый байт - это отдельное число */
    size_t word_number = 0;
    
    uint8_t utf_chars[2] = {0}; // для символов, занимающих 2 байта
    int is_next = 0;  
        
    for (size_t i = 0; i <= array_size; i++) {

        if ((byte_array[i] & 192) == 192) { // первый символ из двухбайтового символа UTF-8
            utf_chars[0] = byte_array[i];
            is_next = 1;
            continue;
        }
        
        if (is_next) { // второй символ из двухбайтового символа UTF-8
            utf_chars[1] = byte_array[i];
            word_number = word_number + utf_chars[0] + utf_chars[1];
            utf_chars[0] = 0;
            utf_chars[1] = 0;
            is_next = 0;
            continue;
        }
        
        /* В качестве разделителей для слов используются точки, запятые, скобки и пробельные символы */
        
        if (byte_array[i] != 32 
                && byte_array[i] != 10 
                && byte_array[i] != 40 
                && byte_array[i] != 41 
                && byte_array[i] != 44 
                && byte_array[i] != 46) {
            
            word_number += byte_array[i];
        }
        
        if ((byte_array[i] == 32 
                || byte_array[i] == 10 
                || byte_array[i] == 40 
                || byte_array[i] == 41 
                || byte_array[i] == 44 
                || byte_array[i] == 46) 
                    && byte_array[i - 1] != 32 
                    && byte_array[i - 1] != 10
                    && byte_array[i - 1] != 40 
                    && byte_array[i - 1] != 41 
                    && byte_array[i - 1] != 44 
                    && byte_array[i - 1] != 46) { // последний байт массива тоже имеет значение "10" (добавляли выше)

            struct word_node* current_word = (struct word_node*) malloc(sizeof(struct word_node));
            current_word->word = word_number;
            current_word->count = 1;
            current_word->next = NULL;
            short index = get_index(word_number);

            if (hash_table[index] == NULL) {
                hash_table[index] = current_word;
            
            } else if (hash_table[index]->word == word_number) {
                hash_table[index]->count += 1;
            
            } else if (hash_table[index]->next == NULL) {
                hash_table[index]->next = current_word;
                
            } else {
                create_list_nodes(hash_table[index]->next, current_word);
                
                #ifdef DEBUG
                    printf("Произошла коллизия. Индекс массива %hd\n", index);
                #endif
            } 
            
            word_number = 0; // сбрасываем счетчик для подсчета числа следующего слова
       }     
    } 
}

/* Проходимся по связанному списку (в случае коллизи) */

void create_list_nodes(struct word_node* next_node, struct word_node* const node_for_compare) {

    if (next_node->word == node_for_compare->word) {
        
        next_node->count += 1;
        return;
        
    } else if (next_node->next == NULL) {

        next_node->next = node_for_compare;
        return;

    } else {
        create_list_nodes(next_node->next, node_for_compare);
    }
}

/* Вычисляем контрольную сумму для заданного слова (введенного пользователем) */

size_t get_word_number(char* symbol_bytes) {

    size_t word_number = 0;
    unsigned int positive_number = 0;

    for (int i = 0; i < WORD_SIZE; i++) { // не учитываем для проверки завершающий символ строки '\0'
    
        positive_number = symbol_bytes[i];
        if (symbol_bytes[i] < 0) { 
            positive_number = symbol_bytes[i] + 256; // для некоторых байтов русских букв будет числиться отрицательное значение из-за превышения 127
        }
        
        if (i == 0 && (positive_number == '\0' || positive_number == '\n')) {
            printf("Вы не ввели слово для проверки. Ожидается хотя бы 1 символ.\n");
            scanf("%s[^ \n]", symbol_bytes);
            i--; // чтобы начать текущий цикл снова с нулевого индекса
            continue;
        } 
        
        word_number += positive_number;   
    }
    
    return word_number;
}


/* Получаем количество повторений для заданного слова */

size_t get_count(size_t const word_number) {


    short index = get_index(word_number);
    
    if (hash_table[index] != NULL) {
    
        if (hash_table[index]->word == word_number) {
            
            return hash_table[index]->count;
        
        } else if (hash_table[index]->next != NULL) {
        
            return get_count_from_list(hash_table[index]->next, word_number);
        }
    } 
    
    return 0;
}

/* Ищем слово в связанном списке (в результате коллизий) */

size_t get_count_from_list(const struct word_node* const next_node, size_t const word_number) {

    if (next_node->word == word_number) {

        return next_node->count;
        
    } else if (next_node->next == NULL) {

        return 0;

    } else {
        get_count_from_list(next_node->next, word_number);
    }
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
