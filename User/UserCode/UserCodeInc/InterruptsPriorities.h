#ifndef INC_INTERRUPTS_PRIORITIES_H_
#define INC_INTERRUPTS_PRIORITIES_H_

#ifdef __cplusplus
extern "C"
{
#endif

//! \brief Установка приоритетов прерываний для корректной работы FreeRTOS
void InterruptsPriorities_SetPriorities(void);

#ifdef __cplusplus
}
#endif

#endif // INC_INTERRUPTS_PRIORITIES_H_
