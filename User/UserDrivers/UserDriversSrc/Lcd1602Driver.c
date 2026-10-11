#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "Lcd1602Driver.h"

//! Адреса PCF8574 (7-битные), перебираемые при инициализации
#define LCD1602_I2C_ADDRESS_1     0x27u
#define LCD1602_I2C_ADDRESS_2     0x3Fu
#define LCD1602_I2C_ADDRESS_3     0x20u
#define LCD1602_I2C_ADDRESS_4     0x38u
#define LCD1602_I2C_ADDRESS_5     0x3Eu

//! Количество адресов для перебора
#define LCD1602_I2C_ADDRESS_COUNT 5u

//! Бит RS в байте PCF8574
#define LCD1602_BIT_RS            0x01u

//! Бит RW в байте PCF8574 (всегда 0 - только запись)
#define LCD1602_BIT_RW            0x02u

//! Бит E в байте PCF8574
#define LCD1602_BIT_E             0x04u

//! Бит подсветки в байте PCF8574
#define LCD1602_BIT_BACKLIGHT     0x08u

//! Маска для старших 4 бит данных (D4-D7) в байте PCF8574
#define LCD1602_DATA_MASK         0xF0u

//! Сдвиг данных в младшую тетраду для отправки в PCF8574
#define LCD1602_DATA_SHIFT        4u

//! Задержка после инициализационных команд, мс
#define LCD1602_INIT_DELAY_MS     50u

//! Задержка после команды Clear, мс
#define LCD1602_CLEAR_DELAY_MS    2u

//! Задержка после команды Home, мс
#define LCD1602_HOME_DELAY_MS     2u

//! Команда HD44780: Function Set (4-бит, 2 строки, 5x8)
#define LCD1602_CMD_FUNCTION_SET  0x28u

//! Команда HD44780: Display On/Off Control (Display On, Cursor Off, Blink Off)
#define LCD1602_CMD_DISPLAY_ON    0x0Cu

//! Команда HD44780: Display On/Off Control (Display Off)
#define LCD1602_CMD_DISPLAY_OFF   0x08u

//! Команда HD44780: Entry Mode Set (курсор вправо, без сдвига экрана)
#define LCD1602_CMD_ENTRY_MODE    0x06u

//! Команда HD44780: Clear Display
#define LCD1602_CMD_CLEAR         0x01u

//! Команда HD44780: Return Home
#define LCD1602_CMD_HOME          0x02u

//! Команда HD44780: Set CGRAM Address (базовый код)
#define LCD1602_CMD_SET_CGRAM     0x40u

//! Команда HD44780: Set DDRAM Address (базовый код)
#define LCD1602_CMD_SET_DDRAM     0x80u

//! Смещение адреса DDRAM для второй строки
#define LCD1602_DDRAM_ROW1_OFFSET 0x40u

//! Признак инициализации драйвера
static uint8_t lcd1602IsInitialized = 0u;

//! Адрес PCF8574 на шине I2C (7-битный, сдвинутый влево на 1 для HAL)
static uint16_t lcd1602I2cAddress = 0u;

//! Текущее состояние байта PCF8574 (для сохранения подсветки)
static uint8_t lcd1602BacklightState = LCD1602_BIT_BACKLIGHT;

//! Буфер для преобразования числа в строку
static char lcd1602NumberBuffer[12] = { 0 };

//! Таблица битмапов кириллических букв для CGRAM (5x8, 8 байт)
static const uint8_t lcd1602CyrillicBitmaps[LCD1602_CYRILLIC_CHARS_COUNT][8] =
{
    // И (слот 0)
    {
        0x11,   // 10001
        0x13,   // 10011
        0x15,   // 10101
        0x19,   // 11001
        0x11,   // 10001
        0x11,   // 10001
        0x11,   // 10001
        0x00    // 00000
    },
    // Л (слот 1)
    {
        0x0F,   // 01111
        0x09,   // 01001
        0x09,   // 01001
        0x09,   // 01001
        0x09,   // 01001
        0x19,   // 11001
        0x19,   // 11001
        0x00    // 00000
    },
    // Д (слот 2)
    {
        0x06,   // 00110
        0x0A,   // 01010
        0x0A,   // 01010
        0x0A,   // 01010
        0x0A,   // 01010
        0x1F,   // 11111
        0x11,   // 10001
        0x00    // 00000
    },
    // П (слот 3)
    {
        0x1F,   // 11111
        0x11,   // 10001
        0x11,   // 10001
        0x11,   // 10001
        0x11,   // 10001
        0x11,   // 10001
        0x11,   // 10001
        0x00    // 00000
    },
    // У (слот 4)
    {
        0x11,   // 10001
        0x11,   // 10001
        0x11,   // 10001
        0x0A,   // 01010
        0x04,   // 00100
        0x04,   // 00100
        0x08,   // 01000
        0x00    // 00000
    },
    // Я (слот 5)
    {
        0x0F,   // 01111
        0x11,   // 10001
        0x11,   // 10001
        0x0F,   // 01111
        0x05,   // 00101
        0x09,   // 01001
        0x11,   // 10001
        0x00    // 00000
    },
    // Ы (слот 6)
    {
        0x11,   // 10001
        0x11,   // 10001
        0x11,   // 10001
        0x1D,   // 11101
        0x15,   // 10101
        0x15,   // 10101
        0x1D,   // 11101
        0x00    // 00000
    },
    // Ь (слот 7)
    {
        0x10,   // 10000
        0x10,   // 10000
        0x10,   // 10000
        0x1E,   // 11110
        0x11,   // 10001
        0x11,   // 10001
        0x1E,   // 11110
        0x00    // 00000
    }
};

//! Элемент таблицы соответствия UTF-8 кода кириллической буквы и кода в LCD
typedef struct
{
    uint16_t utf8Code;      //!< UTF-8 код буквы
    uint8_t  replacement;   //!< 'A'..'Z' (латиница) или 0x00..0x07 (CGRAM)
} CyrillicMap;

//! Таблица соответствия кириллических букв (UTF-8) и кодов LCD
static const CyrillicMap lcd1602CyrillicMap[] =
{
    { 0x0410u, 'A' },       // А -> A
    { 0x0412u, 'B' },       // В -> B
    { 0x0414u, 0x02u },     // Д -> CGRAM слот 2
    { 0x0415u, 'E' },       // Е -> E
    { 0x0418u, 0x00u },     // И -> CGRAM слот 0
    { 0x041Au, 'K' },       // К -> K
    { 0x041Bu, 0x01u },     // Л -> CGRAM слот 1
    { 0x041Cu, 'M' },       // М -> M
    { 0x041Du, 'H' },       // Н -> H
    { 0x041Eu, 'O' },       // О -> O
    { 0x041Fu, 0x03u },     // П -> CGRAM слот 3
    { 0x0420u, 'P' },       // Р -> P
    { 0x0421u, 'C' },       // С -> C
    { 0x0422u, 'T' },       // Т -> T
    { 0x0423u, 0x04u },     // У -> CGRAM слот 4
    { 0x0425u, 'X' },       // Х -> X
    { 0x042Bu, 0x06u },     // Ы -> CGRAM слот 6
    { 0x042Cu, 0x07u },     // Ь -> CGRAM слот 7
    { 0x042Fu, 0x05u }      // Я -> CGRAM слот 5
};

//! Количество элементов в таблице кириллицы
#define LCD1602_CYRILLIC_MAP_SIZE (sizeof(lcd1602CyrillicMap) / sizeof(lcd1602CyrillicMap[0]))

//! \brief  Формирование байта PCF8574 для отправки
//! \param[in] data - данные (младшие 4 бита)
//! \param[in] rs   - состояние линии RS (0 - команда, 1 - данные)
//! \param[in] e    - состояние линии E (0 или 1)
//! \return Байт для записи в PCF8574
static uint8_t Lcd1602_FormByte(uint8_t data, uint8_t rs, uint8_t e)
{
    uint8_t result = 0u;

    result |= (data << LCD1602_DATA_SHIFT) & LCD1602_DATA_MASK;

    if (rs != 0u)
    {
        result |= LCD1602_BIT_RS;
    }

    if (e != 0u)
    {
        result |= LCD1602_BIT_E;
    }

    result |= lcd1602BacklightState;

    return result;
}

//! \brief  Запись одного байта в PCF8574
//! \param[in] value - байт для записи
//! \return Статус операции
//! \note   lcd1602I2cAddress хранится уже сдвинутым влево на 1
//!         (в формате, ожидаемом HAL_I2C_Master_Transmit).
static Lcd1602Status Lcd1602_WriteByte(uint8_t value)
{
    HAL_StatusTypeDef halStatus = HAL_OK;

    halStatus = HAL_I2C_Master_Transmit(&LCD1602_I2C, lcd1602I2cAddress,
                                        &value, 1u, LCD1602_I2C_TIMEOUT_MS);

    if (halStatus == HAL_OK)
    {
        return LCD1602_OK;
    }

    if (halStatus == HAL_TIMEOUT)
    {
        return LCD1602_ERROR_TIMEOUT;
    }

    return LCD1602_ERROR_I2C;
}

//! \brief  Выдача строба E для передачи полубайта
//! \param[in] data - данные (младшие 4 бита)
//! \param[in] rs   - состояние линии RS
//! \return Статус операции
static Lcd1602Status Lcd1602_PulseEnable(uint8_t data, uint8_t rs)
{
    Lcd1602Status status = LCD1602_OK;

    // E = 1
    status = Lcd1602_WriteByte(Lcd1602_FormByte(data, rs, 1u));

    if (status != LCD1602_OK)
    {
        return status;
    }

    // E = 0
    status = Lcd1602_WriteByte(Lcd1602_FormByte(data, rs, 0u));

    return status;
}

//! \brief  Отправка полубайта в LCD
//! \param[in] data - данные (младшие 4 бита)
//! \param[in] rs   - состояние линии RS
//! \return Статус операции
static Lcd1602Status Lcd1602_WriteNibble(uint8_t data, uint8_t rs)
{
    return Lcd1602_PulseEnable(data & 0x0Fu, rs);
}

//! \brief  Отправка байта команды в LCD (два полубайта)
//! \param[in] command - команда
//! \return Статус операции
static Lcd1602Status Lcd1602_WriteCommand(uint8_t command)
{
    Lcd1602Status status = LCD1602_OK;

    // Старший полубайт
    status = Lcd1602_WriteNibble((command >> 4) & 0x0Fu, 0u);

    if (status != LCD1602_OK)
    {
        return status;
    }

    // Младший полубайт
    status = Lcd1602_WriteNibble(command & 0x0Fu, 0u);

    return status;
}

//! \brief  Отправка байта данных в LCD (два полубайта)
//! \param[in] data - данные
//! \return Статус операции
static Lcd1602Status Lcd1602_WriteData(uint8_t data)
{
    Lcd1602Status status = LCD1602_OK;

    // Старший полубайт
    status = Lcd1602_WriteNibble((data >> 4) & 0x0Fu, 1u);

    if (status != LCD1602_OK)
    {
        return status;
    }

    // Младший полубайт
    status = Lcd1602_WriteNibble(data & 0x0Fu, 1u);

    return status;
}

//! \brief  Поиск адреса PCF8574 на шине I2C
//! \return Сдвинутый адрес (для HAL) или 0, если не найден
//! \note   Возвращаемый адрес уже сдвинут влево на 1 и может
//!         напрямую передаваться в HAL_I2C_* функции.
static uint16_t Lcd1602_FindI2cAddress(void)
{
    static const uint8_t addresses[LCD1602_I2C_ADDRESS_COUNT] =
    {
        LCD1602_I2C_ADDRESS_1,
        LCD1602_I2C_ADDRESS_2,
        LCD1602_I2C_ADDRESS_3,
        LCD1602_I2C_ADDRESS_4,
        LCD1602_I2C_ADDRESS_5
    };

    for (uint8_t index = 0u; index < LCD1602_I2C_ADDRESS_COUNT; index++)
    {
        uint16_t shiftedAddress = (uint16_t)(addresses[index] << 1u);

        HAL_StatusTypeDef halStatus = HAL_I2C_IsDeviceReady(&LCD1602_I2C,
                                                             shiftedAddress,
                                                             3u,
                                                             LCD1602_I2C_TIMEOUT_MS);

        if (halStatus == HAL_OK)
        {
            return shiftedAddress;
        }
    }

    return 0u;
}

//! \brief  Инициализация HD44780 в 4-битном режиме
//! \return Статус операции
static Lcd1602Status Lcd1602_InitHd44780(void)
{
    Lcd1602Status status = LCD1602_OK;

    HAL_Delay(LCD1602_INIT_DELAY_MS);

    // Перевод в 4-битный режим (три попытки)
    status = Lcd1602_WriteNibble(0x03u, 0u);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(5u);

    status = Lcd1602_WriteNibble(0x03u, 0u);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(1u);

    status = Lcd1602_WriteNibble(0x03u, 0u);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(1u);

    status = Lcd1602_WriteNibble(0x02u, 0u);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(1u);

    // Function Set: 4-бит, 2 строки, 5x8
    status = Lcd1602_WriteCommand(LCD1602_CMD_FUNCTION_SET);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(1u);

    // Display Off
    status = Lcd1602_WriteCommand(LCD1602_CMD_DISPLAY_OFF);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(1u);

    // Clear Display
    status = Lcd1602_WriteCommand(LCD1602_CMD_CLEAR);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(LCD1602_CLEAR_DELAY_MS);

    // Entry Mode Set
    status = Lcd1602_WriteCommand(LCD1602_CMD_ENTRY_MODE);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(1u);

    // Display On
    status = Lcd1602_WriteCommand(LCD1602_CMD_DISPLAY_ON);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(1u);

    return LCD1602_OK;
}

//! \brief  Загрузка кириллических букв в CGRAM
//! \return Статус операции
static Lcd1602Status Lcd1602_LoadCyrillicToCgram(void)
{
    Lcd1602Status status = LCD1602_OK;

    for (uint8_t slot = 0u; slot < LCD1602_CYRILLIC_CHARS_COUNT; slot++)
    {
        // Установка адреса CGRAM: слот * 8
        status = Lcd1602_WriteCommand(LCD1602_CMD_SET_CGRAM | (uint8_t)(slot << 3u));

        if (status != LCD1602_OK)
        {
            return status;
        }

        // Запись 8 байт битмапа
        for (uint8_t row = 0u; row < 8u; row++)
        {
            status = Lcd1602_WriteData(lcd1602CyrillicBitmaps[slot][row]);

            if (status != LCD1602_OK)
            {
                return status;
            }
        }
    }

    return LCD1602_OK;
}

//! \brief  Преобразование числа в строку
//! \param[in] number - число
//! \return Указатель на статический буфер с результатом
static const char * Lcd1602_NumberToString(int32_t number)
{
    uint8_t index = 0u;
    uint8_t isNegative = 0u;
    uint32_t absValue = 0u;
    char tempBuffer[12] = { 0 };

    if (number < 0)
    {
        isNegative = 1u;
        absValue = (uint32_t)(-number);
    }
    else
    {
        absValue = (uint32_t)number;
    }

    // Обратное преобразование цифр
    if (absValue == 0u)
    {
        tempBuffer[index] = '0';
        index++;
    }
    else
    {
        while (absValue > 0u)
        {
            tempBuffer[index] = (char)('0' + (absValue % 10u));
            index++;
            absValue /= 10u;
        }
    }

    // Формирование результата в прямом порядке
    uint8_t outIndex = 0u;

    if (isNegative != 0u)
    {
        lcd1602NumberBuffer[outIndex] = '-';
        outIndex++;
    }

    while (index > 0u)
    {
        index--;
        lcd1602NumberBuffer[outIndex] = tempBuffer[index];
        outIndex++;
    }

    lcd1602NumberBuffer[outIndex] = '\0';

    return lcd1602NumberBuffer;
}

//! \brief  Вывод одного байта с проверкой границы строки
//! \param[in] symbol - код символа
//! \param[in] pCol   - указатель на текущий столбец (обновляется)
//! \return 1, если символ выведен; 0, если достигнут конец строки
static uint8_t Lcd1602_PrintCharBounded(char symbol, uint8_t *pCol)
{
    if (*pCol >= LCD1602_COLS)
    {
        return 0u;
    }

    Lcd1602_WriteData((uint8_t)symbol);

    (*pCol)++;

    return 1u;
}

//! \brief  Декодирование одного UTF-8 символа
//! \param[in]  pText  - указатель на строку
//! \param[out] pCode  - код символа
//! \return Количество байт, занимаемых символом (1..3), или 0 при ошибке
static uint8_t Lcd1602_DecodeUtf8(const char *pText, uint16_t *pCode)
{
    const uint8_t *pBytes = (const uint8_t *)pText;

    if (pBytes[0] < 0x80u)
    {
        *pCode = (uint16_t)pBytes[0];
        return 1u;
    }

    if ((pBytes[0] & 0xE0u) == 0xC0u)
    {
        if (pBytes[1] == 0u)
        {
            return 0u;
        }

        if ((pBytes[1] & 0xC0u) != 0x80u)
        {
            return 0u;
        }

        *pCode = (uint16_t)(((uint16_t)(pBytes[0] & 0x1Fu) << 6u) |
                            (uint16_t)(pBytes[1] & 0x3Fu));
        return 2u;
    }

    if ((pBytes[0] & 0xF0u) == 0xE0u)
    {
        if ((pBytes[1] == 0u) || (pBytes[2] == 0u))
        {
            return 0u;
        }

        if (((pBytes[1] & 0xC0u) != 0x80u) || ((pBytes[2] & 0xC0u) != 0x80u))
        {
            return 0u;
        }

        *pCode = (uint16_t)(((uint16_t)(pBytes[0] & 0x0Fu) << 12u) |
                            ((uint16_t)(pBytes[1] & 0x3Fu) << 6u) |
                            (uint16_t)(pBytes[2] & 0x3Fu));
        return 3u;
    }

    return 0u;
}

//! \brief  Поиск кириллической буквы в таблице
//! \param[in]  utf8Code - UTF-8 код буквы
//! \param[out] pResult  - код для LCD
//! \return 1, если найдено; 0, если не найдено
static uint8_t Lcd1602_FindCyrillic(uint16_t utf8Code, uint8_t *pResult)
{
    for (uint8_t index = 0u; index < LCD1602_CYRILLIC_MAP_SIZE; index++)
    {
        if (lcd1602CyrillicMap[index].utf8Code == utf8Code)
        {
            *pResult = lcd1602CyrillicMap[index].replacement;
            return 1u;
        }
    }

    return 0u;
}

//! \brief  Внутренняя реализация вывода UTF-8 строки с поддержкой кириллицы
//! \param[in]     pText       - указатель на строку UTF-8
//! \param[in,out] pCurrentCol - указатель на текущий столбец (обновляется)
//! \param[in]     maxCol      - максимально допустимый столбец (граница строки)
//! \note   Жёсткий лимит - LCD1602_DDRAM_TOTAL_CELLS (32) символа.
//!         Если maxCol == 0, действует только жёсткий лимит.
//!         Если maxCol > 0, действует минимум из maxCol и жёсткого лимита.
static void Lcd1602_PrintCyrillicInternal(const char *pText,
                                          uint8_t *pCurrentCol,
                                          uint8_t maxCol)
{
    uint16_t code = 0u;
    uint8_t bytesCount = 0u;
    uint8_t lcdCode = 0u;

    while (*pText != '\0')
    {
        // Жёсткий лимит DDRAM LCD1602
        if (*pCurrentCol >= LCD1602_DDRAM_TOTAL_CELLS)
        {
            break;
        }

        // Дополнительный лимит по границе строки
        if ((maxCol > 0u) && (*pCurrentCol >= maxCol))
        {
            break;
        }

        bytesCount = Lcd1602_DecodeUtf8(pText, &code);

        if (bytesCount == 0u)
        {
            // Некорректная последовательность - пропускаем байт
            pText++;
            continue;
        }

        // Игнорирование управляющих символов
        if ((code == (uint16_t)'\n') || (code == (uint16_t)'\r'))
        {
            pText += bytesCount;
            continue;
        }

        if (code < 0x80u)
        {
            // ASCII - выводим как есть
            Lcd1602_WriteData((uint8_t)code);
            (*pCurrentCol)++;
        }
        else
        {
            // Кириллица - поиск в таблице
            if (Lcd1602_FindCyrillic(code, &lcdCode) != 0u)
            {
                Lcd1602_WriteData(lcdCode);
                (*pCurrentCol)++;
            }
            else
            {
                // Неподдерживаемая буква - '?'
                Lcd1602_WriteData((uint8_t)'?');
                (*pCurrentCol)++;
            }
        }

        pText += bytesCount;
    }
}

//! \brief  Инициализация драйвера LCD1602 через I2C (PCF8574)
Lcd1602Status Lcd1602Driver_Init(void)
{
    Lcd1602Status status = LCD1602_OK;

    if (lcd1602IsInitialized != 0u)
    {
        return LCD1602_OK;
    }

    lcd1602BacklightState = LCD1602_BIT_BACKLIGHT;

    lcd1602I2cAddress = Lcd1602_FindI2cAddress();

    if (lcd1602I2cAddress == 0u)
    {
        return LCD1602_ERROR_NOT_FOUND;
    }

    status = Lcd1602_InitHd44780();

    if (status != LCD1602_OK)
    {
        return status;
    }

#if (LCD1602_LANGUAGE_CYRILLIC == 1)
    status = Lcd1602_LoadCyrillicToCgram();

    if (status != LCD1602_OK)
    {
        return status;
    }
#endif

    status = Lcd1602_WriteCommand(LCD1602_CMD_CLEAR);

    if (status != LCD1602_OK)
    {
        return status;
    }

    HAL_Delay(LCD1602_CLEAR_DELAY_MS);

    lcd1602IsInitialized = 1u;

    return LCD1602_OK;
}

//! \brief  Вывод строки с текущей позиции курсора (только ASCII)
void Lcd1602Driver_Print(const char *pText)
{
    uint8_t printedCount = 0u;

    if ((pText == NULL) || (lcd1602IsInitialized == 0u))
    {
        return;
    }

    while (*pText != '\0')
    {
        // Жёсткий лимит DDRAM LCD1602
        if (printedCount >= LCD1602_DDRAM_TOTAL_CELLS)
        {
            break;
        }

        Lcd1602_WriteData((uint8_t)(*pText));
        pText++;
        printedCount++;
    }
}

//! \brief  Вывод строки в заданную позицию (только ASCII)
void Lcd1602Driver_PrintAt(uint8_t row, uint8_t col, const char *pText)
{
    uint8_t currentCol = 0u;

    if ((pText == NULL) || (lcd1602IsInitialized == 0u))
    {
        return;
    }

    if ((row >= LCD1602_ROWS) || (col >= LCD1602_COLS))
    {
        return;
    }

    Lcd1602Driver_SetCursor(row, col);

    currentCol = col;

    while (*pText != '\0')
    {
        if (Lcd1602_PrintCharBounded(*pText, &currentCol) == 0u)
        {
            break;
        }

        pText++;
    }
}

//! \brief  Вывод одного символа с текущей позиции курсора
void Lcd1602Driver_PutChar(char symbol)
{
    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    Lcd1602_WriteData((uint8_t)symbol);
}

//! \brief  Вывод целого числа со знаком
void Lcd1602Driver_PrintNumber(int32_t number)
{
    const char *pString = NULL;

    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    pString = Lcd1602_NumberToString(number);

    Lcd1602Driver_Print(pString);
}

//! \brief  Вывод целого числа со знаком с ведущими нулями
void Lcd1602Driver_PrintNumberPadded(int32_t number, uint8_t width)
{
    const char *pString = NULL;
    uint8_t length = 0u;
    uint8_t zeroCount = 0u;
    uint8_t index = 0u;

    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    pString = Lcd1602_NumberToString(number);

    length = (uint8_t)strlen(pString);

    if (length >= width)
    {
        Lcd1602Driver_Print(pString);
        return;
    }

    zeroCount = width - length;

    // Если число отрицательное, знак выводится первым
    if (pString[0] == '-')
    {
        Lcd1602Driver_PutChar('-');
        index = 1u;
    }

    for (uint8_t zeroIndex = 0u; zeroIndex < zeroCount; zeroIndex++)
    {
        Lcd1602Driver_PutChar('0');
    }

    while (pString[index] != '\0')
    {
        Lcd1602Driver_PutChar(pString[index]);
        index++;
    }
}

//! \brief  Вывод строки в кодировке UTF-8 с поддержкой кириллицы
//!         с текущей позиции курсора
void Lcd1602Driver_PrintCyrillic(const char *pText)
{
    uint8_t currentCol = 0u;

    if ((pText == NULL) || (lcd1602IsInitialized == 0u))
    {
        return;
    }

    // maxCol = 0: действует только жёсткий лимит DDRAM
    Lcd1602_PrintCyrillicInternal(pText, &currentCol, 0u);
}

//! \brief  Вывод строки в кодировке UTF-8 с поддержкой кириллицы
//!         в заданную позицию
void Lcd1602Driver_PrintCyrillicAt(uint8_t row, uint8_t col, const char *pText)
{
    uint8_t currentCol = 0u;

    if ((pText == NULL) || (lcd1602IsInitialized == 0u))
    {
        return;
    }

    if ((row >= LCD1602_ROWS) || (col >= LCD1602_COLS))
    {
        return;
    }

    Lcd1602Driver_SetCursor(row, col);

    currentCol = col;

    Lcd1602_PrintCyrillicInternal(pText, &currentCol, LCD1602_COLS);
}

//! \brief  Очистка экрана и установка курсора в (0, 0)
void Lcd1602Driver_Clear(void)
{
    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    Lcd1602_WriteCommand(LCD1602_CMD_CLEAR);

    HAL_Delay(LCD1602_CLEAR_DELAY_MS);
}

//! \brief  Установка курсора в заданную позицию
void Lcd1602Driver_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address = 0u;

    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    if ((row >= LCD1602_ROWS) || (col >= LCD1602_COLS))
    {
        return;
    }

    if (row == 0u)
    {
        address = col;
    }
    else
    {
        address = LCD1602_DDRAM_ROW1_OFFSET + col;
    }

    Lcd1602_WriteCommand(LCD1602_CMD_SET_DDRAM | address);
}

//! \brief  Установка курсора в (0, 0)
void Lcd1602Driver_Home(void)
{
    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    Lcd1602_WriteCommand(LCD1602_CMD_HOME);

    HAL_Delay(LCD1602_HOME_DELAY_MS);
}

//! \brief  Включение дисплея
void Lcd1602Driver_DisplayOn(void)
{
    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    Lcd1602_WriteCommand(LCD1602_CMD_DISPLAY_ON);
}

//! \brief  Выключение дисплея
void Lcd1602Driver_DisplayOff(void)
{
    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    Lcd1602_WriteCommand(LCD1602_CMD_DISPLAY_OFF);
}

//! \brief  Включение подсветки
void Lcd1602Driver_BacklightOn(void)
{
    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    lcd1602BacklightState = LCD1602_BIT_BACKLIGHT;

    Lcd1602_WriteByte(Lcd1602_FormByte(0u, 0u, 0u));
}

//! \brief  Выключение подсветки
void Lcd1602Driver_BacklightOff(void)
{
    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    lcd1602BacklightState = 0u;

    Lcd1602_WriteByte(Lcd1602_FormByte(0u, 0u, 0u));
}

//! \brief  Загрузка кастомного символа в слот CGRAM
void Lcd1602Driver_LoadCustomChar(uint8_t slot, const uint8_t *pBitmap)
{
    Lcd1602Status status = LCD1602_OK;

    if ((pBitmap == NULL) || (lcd1602IsInitialized == 0u))
    {
        return;
    }

    if (slot >= LCD1602_CGRAM_SLOTS_COUNT)
    {
        return;
    }

    status = Lcd1602_WriteCommand(LCD1602_CMD_SET_CGRAM | (uint8_t)(slot << 3u));

    if (status != LCD1602_OK)
    {
        return;
    }

    for (uint8_t row = 0u; row < 8u; row++)
    {
        status = Lcd1602_WriteData(pBitmap[row]);

        if (status != LCD1602_OK)
        {
            return;
        }
    }
}

//! \brief  Вывод кастомного символа из слота CGRAM
void Lcd1602Driver_PutCustomChar(uint8_t slot)
{
    if (lcd1602IsInitialized == 0u)
    {
        return;
    }

    if (slot >= LCD1602_CGRAM_SLOTS_COUNT)
    {
        return;
    }

    Lcd1602_WriteData(slot);
}

//! \brief  Переход в режим энергосбережения
void Lcd1602Driver_Sleep(void)
{
    Lcd1602Driver_DisplayOff();
    Lcd1602Driver_BacklightOff();
}

//! \brief  Выход из режима энергосбережения
void Lcd1602Driver_Wake(void)
{
    Lcd1602Driver_DisplayOn();
    Lcd1602Driver_BacklightOn();
}
