#ifndef INC_LCD1602_DRIVER_H_
#define INC_LCD1602_DRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "Hardware.h"

//! Выбор языка: 1 = кириллица (по умолчанию), 0 = латиница
#define LCD1602_LANGUAGE_CYRILLIC     1u

//! Количество строк LCD1602
#define LCD1602_ROWS                  2u

//! Количество столбцов LCD1602
#define LCD1602_COLS                  16u

//! Общее количество знакомест DDRAM LCD1602 (2 строки x 16 столбцов)
#define LCD1602_DDRAM_TOTAL_CELLS     (LCD1602_ROWS * LCD1602_COLS)

//! Количество кириллических букв, загружаемых в CGRAM
#define LCD1602_CYRILLIC_CHARS_COUNT  8u

//! Количество слотов CGRAM
#define LCD1602_CGRAM_SLOTS_COUNT     8u

//! Максимальная длина одной строки LCD (16 символов + '\0')
#define LCD1602_LINE_BUFFER_SIZE      (LCD1602_COLS + 1u)

//! Статус выполнения операций драйвера
typedef enum
{
    LCD1602_OK = 0,
    LCD1602_ERROR_NOT_FOUND,
    LCD1602_ERROR_I2C,
    LCD1602_ERROR_TIMEOUT
} Lcd1602Status;

//! \brief  Инициализация драйвера LCD1602 через I2C (PCF8574)
//! \retval Результат инициализации:
//!         LCD1602_OK                - инициализация выполнена успешно,
//!         LCD1602_ERROR_NOT_FOUND   - PCF8574 не найден по известным адресам,
//!         LCD1602_ERROR_I2C         - ошибка обмена по I2C,
//!         LCD1602_ERROR_TIMEOUT     - таймаут операции I2C
//! \note   Init использует HAL_Delay, поэтому должен вызываться
//!         до vTaskStartScheduler() или из критической секции.
//!         Init идемпотентна: повторный вызов без предварительного
//!         Reset не выполняет повторную инициализацию.
Lcd1602Status Lcd1602Driver_Init(void);

//! \brief  Вывод строки с текущей позиции курсора (только ASCII)
//! \param[in] pText - указатель на строку (ASCII, без кириллицы)
//! \note   Вывод ограничен 32 символами (LCD1602_DDRAM_TOTAL_CELLS).
//!         Ответственность за содержимое строки - на приложении.
void Lcd1602Driver_Print(const char *pText);

//! \brief  Вывод строки в заданную позицию (только ASCII)
//! \param[in] row   - номер строки (0..1)
//! \param[in] col   - номер столбца (0..15)
//! \param[in] pText - указатель на строку (ASCII, без кириллицы)
//! \note   Если row/col вне допустимого диапазона - функция ничего не делает.
//!         Строка обрезается по границе строки LCD.
//! \warning Для строк в UTF-8 с кириллицей используйте Lcd1602Driver_PrintCyrillicAt.
void Lcd1602Driver_PrintAt(uint8_t row, uint8_t col, const char *pText);

//! \brief  Вывод одного символа с текущей позиции курсора
//! \param[in] symbol - код символа
void Lcd1602Driver_PutChar(char symbol);

//! \brief  Вывод целого числа со знаком
//! \param[in] number - число для вывода
void Lcd1602Driver_PrintNumber(int32_t number);

//! \brief  Вывод целого числа со знаком с ведущими нулями
//! \param[in] number - число для вывода
//! \param[in] width  - минимальная ширина поля (знак учитывается в ширине)
void Lcd1602Driver_PrintNumberPadded(int32_t number, uint8_t width);

//! \brief  Вывод строки в кодировке UTF-8 с поддержкой кириллицы
//!         с текущей позиции курсора
//! \param[in] pText - указатель на строку UTF-8
//! \note   Латиница, цифры, пробелы и знаки препинания выводятся как есть.
//!         Неподдерживаемые буквы заменяются на '?'.
//!         Символы '\n' и '\r' игнорируются.
//!         Вывод ограничен 32 символами (LCD1602_DDRAM_TOTAL_CELLS).
void Lcd1602Driver_PrintCyrillic(const char *pText);

//! \brief  Вывод строки в кодировке UTF-8 с поддержкой кириллицы
//!         в заданную позицию
//! \param[in] row   - номер строки (0..1)
//! \param[in] col   - номер столбца (0..15)
//! \param[in] pText - указатель на строку UTF-8
//! \note   Если row/col вне допустимого диапазона - функция ничего не делает.
//!         Строка обрезается по границе строки LCD.
void Lcd1602Driver_PrintCyrillicAt(uint8_t row, uint8_t col, const char *pText);

//! \brief  Очистка экрана и установка курсора в (0, 0)
void Lcd1602Driver_Clear(void);

//! \brief  Установка курсора в заданную позицию
//! \param[in] row - номер строки (0..1)
//! \param[in] col - номер столбца (0..15)
//! \note   Если row/col вне допустимого диапазона - функция ничего не делает.
void Lcd1602Driver_SetCursor(uint8_t row, uint8_t col);

//! \brief  Установка курсора в (0, 0)
void Lcd1602Driver_Home(void);

//! \brief  Включение дисплея
void Lcd1602Driver_DisplayOn(void);

//! \brief  Выключение дисплея
void Lcd1602Driver_DisplayOff(void);

//! \brief  Включение подсветки
void Lcd1602Driver_BacklightOn(void);

//! \brief  Выключение подсветки
void Lcd1602Driver_BacklightOff(void);

//! \brief  Загрузка кастомного символа в слот CGRAM
//! \param[in] slot    - номер слота (0..7)
//! \param[in] pBitmap - указатель на битмап 5x8 (8 байт, младшие 5 бит значимы)
//! \warning Если LCD1602_LANGUAGE_CYRILLIC == 1, слоты 0..7 заняты
//!          кириллицей (И, Л, Д, П, У, Я, Ы, Ь). Перезапись через
//!          LoadCustomChar сломает вывод кириллицы.
//! \note   Если slot > 7 - функция ничего не делает.
void Lcd1602Driver_LoadCustomChar(uint8_t slot, const uint8_t *pBitmap);

//! \brief  Вывод кастомного символа из слота CGRAM
//! \param[in] slot - номер слота (0..7)
//! \note   Если slot > 7 - функция ничего не делает.
void Lcd1602Driver_PutCustomChar(uint8_t slot);

//! \brief  Переход в режим энергосбережения (выключение дисплея и подсветки)
void Lcd1602Driver_Sleep(void);

//! \brief  Выход из режима энергосбережения (включение дисплея и подсветки)
void Lcd1602Driver_Wake(void);

#ifdef __cplusplus
}
#endif

#endif // INC_LCD1602_DRIVER_H_
