#ifndef ISR_HANDLERS_H
#define ISR_HANDLERS_H

#include <Arduino.h>

void pbSelFallIsr(void);
void pbDecFallIsr(void);
void pbIncFallIsr(void);
void pbCnlFallIsr(void);
void pbMenFallIsr(void);

#endif // ISR_HANDLERS_H
