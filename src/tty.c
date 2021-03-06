#include "stm32f0xx.h"
#include "stm32f0_discovery.h"

#include <stdio.h>
#include <stdarg.h>
#include "tty.h"
#include "fifo.h"

#define UNK -1
#define NON_INTR 0
#define INTR 1
int interrupt_mode = UNK;

int PUSART = 2;

int __io_putchar(int ch);
static int putchar_nonirq(int ch);
static struct fifo input_fifo;  // input buffer
static struct fifo output_fifo; // output buffer
int echo_mode = 1;              // should we echo input characters?
int line_mode = 1;              // should we wait for a newline?

//=============================================================================
// This is a version of printf() that will disable interrupts for the
// USART and write characters directly.  It is intended to handle fatal
// exceptional conditions.
// It's also an example of how to create a variadic function.
static void safe_printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    char buf[80];
    int len = vsnprintf(buf, sizeof buf, format, ap);
    int saved_txeie = USART1->CR1 & USART_CR1_TXEIE;
    USART1->CR1 &= ~USART_CR1_TXEIE;
    int x;
    for(x=0; x<len; x++) {
        putchar_nonirq(buf[x]);
    }
    USART1->CR1 |= saved_txeie;
    va_end(ap);
}

//=======================================================================
// Simply write a string one char at a time.
//=======================================================================
static void putstr(const char *s) {
    while(*s)
        __io_putchar(*s++);
}

//=======================================================================
// Insert a character and echo it.
// (or, if it's a backspace, remove a char and erase it from the line).
// If echo_mode is turned off, just insert the character and get out.
//=======================================================================
static void insert_echo_char(char ch) {
    if (ch == '\r')
        ch = '\n';
    if (!echo_mode) {
        fifo_insert(&input_fifo, ch);
        return;
    }
    if (ch == '\b' || ch == '\177') {
        if (!fifo_empty(&input_fifo)) {
            char tmp = fifo_uninsert(&input_fifo);
            if (tmp == '\n')
                fifo_insert(&input_fifo, '\n');
            else if (tmp < 32)
                putstr("\b\b  \b\b");
            else
                putstr("\b \b");
        }
        return; // Don't put a backspace into buffer.
    } else if (ch == '\n') {
        __io_putchar('\n');
    } else if (ch == 0){
        putstr("^0");
    } else if (ch == 28) {
        putstr("^\\");
    } else if (ch < 32) {
        __io_putchar('^');
        __io_putchar('A'-1+ch);
    } else {
        __io_putchar(ch);
    }
    fifo_insert(&input_fifo, ch);
}

//=======================================================================
// Enable the USART RXNE interrupt.
// Remember to enable the right bit in the NVIC registers
//=======================================================================
void enable_tty_irq(void) {
    // Student code goes here...
    USART1 -> CR1  |= USART_CR1_RXNEIE; //enable the RXNE interrupt
    NVIC_EnableIRQ(USART1_IRQn);    //enable the right bit in the NVIC registers
    interrupt_mode = INTR;  //set the interrupt_mode flag to INTR
    NVIC_SetPriority(USART1_IRQn, PUSART);
    // REMEMBER TO SET THE USART1 INTERRUPT PRIORITY HERE!
}

//=======================================================================
// This method should perform the following
// Transmit 'ch' using USART1, remember to wait for transmission register to be
// empty. Also this function must check if 'ch' is a new line character, if so
// it must transmit a carriage return before transmitting 'ch' using USART1.
// Think about this, why must we add a carriage return, what happens otherwise?
//=======================================================================
static int putchar_nonirq(int ch) {
    // Student code goes here...
    if (ch == '\n') {
        while (!(USART1 -> ISR & USART_ISR_TXE));     //Wait for the USART transmitter to be empty
        USART1 -> TDR = '\r';     //Transmit a carriage return (a ‘\r’ character)
    }
    while (!(USART1 -> ISR & USART_ISR_TXE));     //Wait for the USART transmitter to be empty
    USART1 -> TDR = ch;     //Transmit the character in the ch variable.
    return ch;
}

//-----------------------------------------------------------------------------
// Section 6.4
//-----------------------------------------------------------------------------
// See lab document for description
static int getchar_nonirq(void) {
    // Student code goes here...
    if (USART1 -> ISR & USART_ISR_ORE) {
            USART1 -> ISR |= USART_ICR_ORECF;
     }
    if (line_mode) {
        while (!fifo_newline(&input_fifo)) {
            while(!(USART1 -> ISR & USART_ISR_RXNE));
            char ch = USART1 -> RDR;
            insert_echo_char(ch);
        }
    }
    return fifo_remove(&input_fifo);
    // REMEMBER TO NOT CHECK FOR NEWLINES WHEN line_mode IS NOT SET!
}

//=======================================================================
// IRQ invoked for USART1 activity.
void USART1_IRQHandler(void) {
    // Student code goes here...
    if (USART1 -> ISR & USART_ISR_RXNE) {
            insert_echo_char(USART1 -> RDR);
        }
        if (USART1 -> ISR & USART_ISR_TXE) {
            if (fifo_empty(&output_fifo))
                USART1 -> CR1 &= ~USART_CR1_TXEIE;
            else {
                USART1 -> TDR = fifo_remove(&output_fifo);
            }
        }
    if (USART1->ISR & (USART_ISR_ORE|USART_ISR_NE|USART_ISR_FE|USART_ISR_PE)) {
            safe_printf("Problem in USART1_IRQHandler: ISR = 0x%x\n", USART1->ISR);
        }
    // REMEMBER TO REMOVE THE CKECK FOR USART_ISR_RXNE FROM THE WARNING
    // THAT INVOKES safe_printf().
}

// See lab document for description
static int getchar_irq(void) {
    // Student code goes here...
    if (line_mode) {
        while(!fifo_newline(&input_fifo))//While the input_fifo does not contain a newline
            asm("wfi");//Wait for an interrupt
    }
    return fifo_remove(&input_fifo);//Return a character removed from input_fifo
    // REMEMBER TO CHECK FOR A NEWLINE ONLY WHEN line_mode IS SET!
}

// See lab document for description
static int putchar_irq(char ch) {
    // Student code goes here...
    while (fifo_full(&output_fifo)) //While the output_fifo is full,
        asm("wfi"); //Wait for an interrupt.
    if (ch == '\n')
        fifo_insert(&output_fifo, '\r');    //Insert a ‘\r’ into the output_fifo.
    else
        fifo_insert(&output_fifo, ch);
    if (!(USART1->CR1 & USART_CR1_TXEIE)) {
        USART1 -> CR1 |= USART_CR1_TXEIE;
        USART1_IRQHandler();//start sending.
    }
    if (ch == '\n') {
        while (fifo_full(&output_fifo))
            asm("wfi");
        fifo_insert(&output_fifo, '\n');
    }
}

//=======================================================================
// Called by the Standard Peripheral library for a write()
int __io_putchar(int ch) {
    if (interrupt_mode == INTR)
        return putchar_irq(ch);
    else
        return putchar_nonirq(ch);
}

//=======================================================================
// Called by the Standard Peripheral library for a read()
int __io_getchar(void) {
    if (interrupt_mode == INTR)
        return getchar_irq();
    else
        return getchar_nonirq();
}

void tty_init(void) {
    // Disable buffers for stdio streams.  Otherwise, the first use of
    // each stream will result in a *malloc* of 2K.  Not good.
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);

    // Student code goes here...
    RCC ->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA ->MODER &= ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10);
    GPIOA ->MODER |= GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1;
    //GPIOA -> AFR[1] &= ~(GPIO_AFRH_AFRH1 | GPIO_AFRH_AFRH2);
    GPIOA -> AFR[1] |=1<<(4*1) | 1<< (4*2);
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1 -> CR1 &= ~USART_CR1_UE; //Disable the USART module
    //8 bits, 1 stop bit , no parity bit, 16x oversampling
    USART1 -> CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_OVER8);
    USART1 -> CR2 &= ~ USART_CR2_STOP;
    USART1 -> BRR =  0xD0;//(FRM table 96)supposed to be 921600 baud rate but change to 230400
    USART1 -> CR1 |= USART_CR1_RE | USART_CR1_TE;
    USART1 -> CR1 |= USART_CR1_UE;
    while (!(USART1 ->ISR & (USART_ISR_TEACK | USART_ISR_REACK)));
    interrupt_mode = NON_INTR;
    enable_tty_irq();
    // REMEMBER TO CALL enable_tty_irq() HERE!
}

void raw_mode(void)
{
    line_mode = 0;
    echo_mode = 0;
}

void cooked_mode(void)
{
    line_mode = 1;
    echo_mode = 1;
}

int available(void)
{
    if (interrupt_mode == INTR) {
        if (fifo_empty(&input_fifo))
            return 0;
        return 1;
    } else {
        if ((USART1->ISR & USART_ISR_RXNE) == 0)
            return 0;
        return 1;
    }
}

