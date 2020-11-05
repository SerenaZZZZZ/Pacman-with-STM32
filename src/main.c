
#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "tty.h"
#include "midi.h"
#include <math.h>
#include <stdint.h>
#include "snake.h"

#define RATE 20000
#define N 50

//game priority
//int PUSART = 2;
int P2=3;
//music priority
int P6= 1;
int P3= 2;
int PDMA= 0;

MIDI_Player *mp;
extern uint8_t medley[];
extern uint8_t death[];
extern uint8_t shrt[];

short int wavetable[N];

#define VOICES 6

#define STEP1 1.05946309436
#define STEP2 (STEP1*STEP1)
#define STEP3 (STEP2*STEP1)
#define STEP4 (STEP3*STEP1)
#define STEP5 (STEP4*STEP1)
#define STEP6 (STEP5*STEP1)
#define STEP7 (STEP6*STEP1)
#define STEP8 (STEP7*STEP1)
#define STEP9 (STEP8*STEP1)

#define A14    ((13.75   * N/RATE) * (1<<16)) /* A0 */
#define A27    ((27.5    * N/RATE) * (1<<16)) /* A1 */
#define A55    ((55.0    * N/RATE) * (1<<16)) /* A2 */
#define A110   ((110.0   * N/RATE) * (1<<16)) /* A3 */
#define A220   ((220.0   * N/RATE) * (1<<16)) /* A4 */
#define A440   ((440.0   * N/RATE) * (1<<16)) /* A5 */
#define A880   ((880.0   * N/RATE) * (1<<16)) /* A6 */
#define A1760  ((1760.0  * N/RATE) * (1<<16)) /* A7 */
#define A3520  ((3520.0  * N/RATE) * (1<<16)) /* A8 */
#define A7040  ((7040.0  * N/RATE) * (1<<16)) /* A9 */
#define A14080 ((14080.0 * N/RATE) * (1<<16)) /* A10 */

extern uint8_t midifile[];
uint16_t queue[100];

const int step[] = {
        A14 / STEP9,    // C                         C-1
        A14 / STEP8,    // C# / Db
        A14 / STEP7,    // D
        A14 / STEP6,    // D# / Eb
        A14 / STEP5,    // E
        A14 / STEP4,    // F
        A14 / STEP3,    // F# / Gb
        A14 / STEP2,    // G
        A14 / STEP1,    // G# / Ab
        A14,            // A27                       A0
        A14 * STEP1,    // A# / Bb
        A14 * STEP2,    // B
        A14 * STEP3,    // C                         C0
        A14 * STEP4,    // C# / Db
        A14 * STEP5,    // D
        A27 * STEP6,    // D# / Eb
        A27 / STEP5,    // E
        A27 / STEP4,    // F
        A27 / STEP3,    // F# / Gb
        A27 / STEP2,    // G
        A27 / STEP1,    // G# / Ab
        A27,            // A27                       A1
        A27 * STEP1,    // A# / Bb
        A27 * STEP2,    // B
        A27 * STEP3,    // C                         C1
        A27 * STEP4,    // C# / Db
        A27 * STEP5,    // D
        A27 * STEP6,    // D# / Eb
        A55 / STEP5,    // E
        A55 / STEP4,    // F
        A55 / STEP3,    // F# / Gb
        A55 / STEP2,    // G
        A55 / STEP1,    // G# / Ab
        A55,            // A55                       A2
        A55 * STEP1,    // A# / Bb
        A55 * STEP2,    // B
        A55 * STEP3,    // C                         C2
        A55 * STEP4,    // C# / Db
        A55 * STEP5,    // D
        A55 * STEP6,    // D# / Eb
        A110 / STEP5,   // E
        A110 / STEP4,   // F
        A110 / STEP3,   // F# / Gb
        A110 / STEP2,   // G
        A110 / STEP1,   // G# / Ab
        A110,           // A110                     A3
        A110 * STEP1,   // A# / Bb
        A110 * STEP2,   // B
        A110 * STEP3,   // C                        C3
        A110 * STEP4,   // C# / Db
        A110 * STEP5,   // D
        A110 * STEP6,   // D# / Eb
        A220 / STEP5,   // E
        A220 / STEP4,   // F
        A220 / STEP3,   // F# / Gb
        A220 / STEP2,   // G
        A220 / STEP1,   // G# / Ab
        A220,           // A220                     A4
        A220 * STEP1,   // A# / Bb
        A220 * STEP2,   // B
        A220 * STEP3,   // C (middle C)      C4 (element #60)
        A220 * STEP4,   // C# / Db
        A220 * STEP5,   // D
        A220 * STEP6,   // D# / Eb
        A440 / STEP5,   // E 64
        A440 / STEP4,   // F
        A440 / STEP3,   // F# / Gb
        A440 / STEP2,   // G 67
        A440 / STEP1,   // G# / Ab
        A440,           // A440                     A5
        A440 * STEP1,   // A# / Bb
        A440 * STEP2,   // B
        A440 * STEP3,   // C      72           C5
        A440 * STEP4,   // C# / Db
        A440 * STEP5,   // D
        A440 * STEP6,   // D# / Eb
        A880 / STEP5,   // E
        A880 / STEP4,   // F
        A880 / STEP3,   // F# / Gb
        A880 / STEP2,   // G
        A880 / STEP1,   // G# / Ab
        A880,           // A880                     A6
        A880 * STEP1,   // A# / Bb
        A880 * STEP2,   // B
        A880 * STEP3,   // C  84                      C6
        A880 * STEP4,   // C# / Db
        A880 * STEP5,   // D
        A880 * STEP6,   // D# / Eb
        A1760 / STEP5,  // E
        A1760 / STEP4,  // F
        A1760 / STEP3,  // F# / Gb
        A1760 / STEP2,  // G
        A1760 / STEP1,  // G# / Ab
        A1760,          // A1760                   A7
        A1760 * STEP1,  // A# / Bb
        A1760 * STEP2,  // B
        A1760 * STEP3,  // C                       C7
        A1760 * STEP4,  // C# / Db
        A1760 * STEP5,  // D
        A1760 * STEP6,  // D# / Eb
        A3520 / STEP5,  // E
        A3520 / STEP4,  // F
        A3520 / STEP3,  // F# / Gb
        A3520 / STEP2,  // G
        A3520 / STEP1,  // G# / Ab
        A3520,          // A3520                   A8
        A3520 * STEP1,  // A# / Bb
        A3520 * STEP2,  // B
        A3520 * STEP3,  // C                       C8
        A3520 * STEP4,  // C# / Db
        A3520 * STEP5,  // D
        A3520 * STEP6,  // D# / Eb
        A7040 / STEP5,  // E
        A7040 / STEP4,  // F
        A7040 / STEP3,  // F# / Gb
        A7040 / STEP2,  // G
        A7040 / STEP1,  // G# / Ab
        A7040,          // A7040                   A9
        A7040 * STEP1,  // A# / Bb
        A7040 * STEP2,  // B
        A7040 * STEP3,  // C                       C9
        A7040 * STEP4,  // C# / Db
        A7040 * STEP5,  // D
        A7040 * STEP6,  // D# / Eb
        A14080 / STEP5, // E
        A14080 / STEP4, // F
        A14080 / STEP3, // F# / Gb
        A14080 / STEP2, // G
};

struct {
    uint8_t in_use;
    uint8_t note;
    uint8_t chan;
    uint8_t volume;//new for step5
    int     step;
    int     offset;
} voice[VOICES];

void DMA1_Channel2_3_IRQHandler(void)
{
    uint16_t *arr;
    if (DMA1->ISR & DMA_ISR_HTIF3) {
        arr = &queue[0];
        DMA1->IFCR = DMA_IFCR_CHTIF3|DMA_IFCR_CGIF3;
    } else {
        arr = &queue[50];
        DMA1->IFCR = DMA_IFCR_CTCIF3|DMA_IFCR_CGIF3;
    }
    int i;
    for(i = 0; i < 50; i++) {
        // TODO: Copy the sample generation from TIM6_DAC_IRQHandler()
        int x;
        int sample = 0;
        for(x=0; x < sizeof(voice)/sizeof(voice[0]); x++) {
            if (voice[x].in_use) {
                voice[x].offset += voice[x].step;
                if (voice[x].offset >= N<<16)
                    voice[x].offset -= N<<16;
                sample += (wavetable[voice[x].offset>>16] * voice[x].volume) >> 4;//step5
            }
        }
        sample = sample / 1024 + 2048;
        if (sample > 4095) sample = 4095;
        else if (sample < 0) sample = 0;
        arr[i] = sample;
    }
}

void TIM2_IRQHandler(void)
{
    TIM2->SR &= ~TIM_SR_UIF;
    animate();
}

void TIM3_IRQHandler(void)
{
    TIM3->SR &= ~TIM_SR_UIF;

    midi_play();
}

void init_DMA() {
    RCC ->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel3->CMAR = (uint32_t)queue;
    DMA1_Channel3->CPAR = (uint32_t) &(DAC->DHR12R1);
    DMA1_Channel3->CNDTR =  sizeof queue / sizeof queue[0];
    //Set the direction of data transfer from memory to peripheral
    DMA1_Channel3->CCR |= DMA_CCR_DIR;
    //memory address should be incremented after each transfer
    DMA1_Channel3->CCR |= DMA_CCR_MINC;
    //Set the memory element size to 16-bit
    DMA1_Channel3->CCR |= DMA_CCR_MSIZE_0;
    //Set the peripheral element size to 16-bit
    DMA1_Channel3->CCR |= DMA_CCR_PSIZE_0;
    //CIRCular transfers
    DMA1_Channel3->CCR |= DMA_CCR_CIRC;
    //Enable the Transfer Complete Interrupt.
    DMA1_Channel3->CCR |= DMA_CCR_TCIE;
    //Enable the Half-Transfer Interrupt.
    DMA1_Channel3->CCR |= DMA_CCR_HTIE;
    //Enable the DMA1_Channel2_3_IRQn in the NVIC
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    DMA1_Channel3->CCR |= DMA_CCR_EN;

    //set the interrupt to be the most important (0)
    NVIC_SetPriority(DMA1_Channel2_3_IRQn,PDMA);
}


//============================================================================
// Setup prescalar and arr so that the interrupt is triggered every 125 ms.
void setup_timer2() {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2 ->PSC = 48000 - 1;
    TIM2 ->ARR = 125 - 1;
    TIM2 ->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2 ->CR1 |= TIM_CR1_CEN;//enable timer 2
    TIM2 ->CR1 |= TIM_CR1_ARPE;//change the rate of timer 2 dynamically.
    NVIC_SetPriority(TIM2_IRQn, P2);
}

void init_TIM3(int n) {
    RCC ->APB1ENR |= RCC_APB1ENR_TIM3EN;
    //rate = n*10^-6
    TIM3 ->PSC = 48 - 1;
    TIM3 ->ARR = n - 1;
    TIM3 ->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM3_IRQn);
    TIM3 ->CR1 |= TIM_CR1_CEN;//enable timer 2
    TIM3 ->CR1 |= TIM_CR1_ARPE;//change the rate of timer 2 dynamically.
    NVIC_SetPriority(TIM3_IRQn, P3);
}

void init_TIM6(void) {
    RCC ->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6 ->PSC =  48 - 1;
    TIM6 ->ARR = 1000000/RATE - 1;
//    TIM6 ->DIER |= TIM_DIER_UIE;
    TIM6 ->DIER |= TIM_DIER_UDE;
    TIM6 ->CR2 |= TIM_CR2_MMS_1;//generate a trigger on each update event step6
    NVIC_EnableIRQ(TIM6_DAC_IRQn);
    TIM6 ->CR1 |= TIM_CR1_CEN;//enable timer 6
    NVIC_SetPriority(TIM6_DAC_IRQn, P6);
}

void init_DAC(void) {
    //setup PA4 to analog mode
    RCC ->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA ->MODER |= GPIO_MODER_MODER4;
    //setup DAC
    RCC ->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC ->CR |= DAC_CR_TEN1;//enable trigger
//    DAC ->CR |= DAC_CR_TSEL1;//select software trigger
    DAC->CR &= ~DAC_CR_TSEL1;//trigger TIM6 TRGO step6
    DAC ->CR |= DAC_CR_EN1; // enable DAC channel 1
}

//===================================
// Disable timer 2.  Wait for a key press.
void freeze(void)
{
    TIM2->CR1 &= ~TIM_CR1_CEN;
//    TIM3->CR1 &= ~TIM_CR1_CEN;
    while(!available())
        ;
    getchar();
    TIM2->CR1 |= TIM_CR1_CEN;
//    TIM3->CR1 |= TIM_CR1_CEN;
}

int get_seed(void)
{
    return TIM2->CNT;
}

void init_hybrid(void) {
    int x;
    for(x=0; x<N; x++)
        wavetable[x] = 16383 * sin(2 * M_PI * x / N) + 16383.0 * (x - N/2) / (1.0*N);
}

void note_off(int time, int chan, int key, int velo)
{
    int n;
    for(n=0; n<sizeof voice / sizeof voice[0]; n++) {
        if (voice[n].in_use && voice[n].note == key) {
            voice[n].in_use = 0; // disable it first...
            voice[n].chan = 0;   // ...then clear its values
            voice[n].note = key;
            voice[n].step = step[key];
//            voice[n].volume = 0;
            return;
        }
    }
}
void note_on(int time, int chan, int key, int velo)
{
    if (velo == 0) {
        note_off(time, chan, key, velo);
        return;
    }
    int n;
    for(n=0; n<sizeof voice / sizeof voice[0]; n++) {
        if (voice[n].in_use == 0) {
            voice[n].note = key;
            voice[n].step = step[key];
            voice[n].offset = 0;
            voice[n].chan = chan;
            voice[n].in_use = 1;
            voice[n].volume = velo;
            return;
        }
    }
}
void set_tempo(int time, int value, const MIDI_Header *hdr)
{
    // This assumes that the TIM2 prescaler divides by 48.
    // It sets the timer to produce an interrupt every N
    // microseconds, where N is the new tempo (value) divided by
    // the number of divisions per beat specified in the MIDI header.
    TIM3->ARR = value/hdr->divisions - 1;
}
void pitch_wheel_change(int time, int chan, int val)
{
    float multiplier = pow(STEP1, (val - 8192.0) / 8192.0);
    int n;
     for(n=0; n<sizeof voice / sizeof voice[0]; n++) {
         if (voice[n].in_use && voice[n].chan == chan)
             voice[n].step = step[voice[n].note] * multiplier;
         }
}

int main(void)
{
    //midi
    init_hybrid();
    init_DAC();
    init_DMA();
    init_TIM6();
    init_TIM3(1000);

    //game
    tty_init();
    raw_mode();
    cursor_off();
    splash();
    init();
    setup_timer2();



    for(;;)
        asm("wfi");
    return 0;
}
