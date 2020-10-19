//Abhishek Dhital
//Memory Protection Unit Implementation
//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port


//                  The priority of the MPU regions is higher the region number,
//                  higher will be the priority of that region
//
//                  MPU Model implemented in this code showing all regions
//  *************************************************************************************
//  *                                                                                   *
//  * -1 default region covering all of addressable 4GiB of memory                      *
//  *  priv mode    rwx access                                                          *
//  *  Unpriv mode  no access                                                           *
//  *  ___________________________________________________________________________      *
//  * |  ********************                       region #0                     |     *
//  * |  * 256 KiB          *                       priv mode- r/w access         |     *
//  * |  * Flash Memory     *                       unpriv mode - r/w access      |     *
//  * |  * region #1                                                              |     *
//  * |  * priv mode- rwx   *                                                     |     *
//  * |  *unpriv mode - rwx *                                                     |     *
//  * |  ********************       _____________________________                 |     *
//  * |                             |  31 KiB SRAM (total 32KiB) |                |     *
//  * |                             |  divided into 32 subregions|                |     *
//  * |                             |         region #2-5        |                |     *
//  * |                             |  priv mode- rw access      |                |     *
//  * |                             |  unpriv mode - no access   |                |     *
//  * |                             |                            |                |     *
//  * |                             |____________________________|                |     *
//  * |                                   remaining 1 KiB SRAM                    |     *
//  * |                                   priv mode- rw access,                   |     *
//  * |                                   unpriv mode rw access                   |     *
//  * |                                                                           |     *
//  * |                                                                           |     *
//  * |                        2GiB                                               |     *
//  * |                        Peripherals/Bitband                                |     *
//  * |                                                                           |     *
//  * |                                                                           |     *
//  * |                                                                           |     *
//  * |___________________________________________________________________________|     *
//  *                                                                                   *
//  *************************************************************************************

#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdbool.h>
#include "uart0.h"


#define GREEN_LED_MASK 8
#define RED_LED_MASK 2
#define PUSH_BUTTON_MASK 16

#define Line_Break "\n\r"


//define statements for the assembly functions on MPU_s.s file
extern void unprivilegedMode();
extern void privilegedMode();
extern void setPSPaddress(uint32_t address);
extern uint32_t getPSPaddress();
extern uint32_t getMSPaddress();
extern void setASPbit();


//******************************Sub-routines***************************************************



//function that takes a 32 bit integer as argument
//and then prints out the corresponding Hex value for the 32 bit integer
void PrintIntToHex(uint32_t value)
{
    char hexStr[9] = {0};

    char HexAlphabets [] = "ABCDEF";
    int i =0;
    while (value != 0)
    {
       char temp;

       if ((value % 16) <= 15 && (value % 16) >= 10)
            temp = HexAlphabets [(value % 16) % 10];

       else
           temp = (value % 16) + 48;

       hexStr[7-i] = temp;

       value = value / 16;
       i++;
    }

    putsUart0("0x");
    i=0;
    while(i <= 7)
    {
        if (hexStr[i] != 0)
            putcUart0(hexStr[i]);
        else
            putcUart0('0');
        i++;
    }


}


//Bus Fault Handler routine
void BusFaultISR()
{
  putsUart0("Bus fault in process N");
  putsUart0(Line_Break);
}

//Usage Fault Handler routine
void UsageFaultISR()
{
 putsUart0("Usage fault in process N");
 putsUart0(Line_Break);
}



//Hard Fault Handler routine
void HardFaultISR()
{
    putsUart0("Hard fault in process N\n\r");

    putsUart0("PSP: ");
    PrintIntToHex(getPSPaddress());
    putsUart0(Line_Break);

    putsUart0("MSP: ");
    PrintIntToHex(getMSPaddress());
    putsUart0(Line_Break);

    putsUart0("Hard Fault Flag: ");
    PrintIntToHex(NVIC_HFAULT_STAT_R);
    putsUart0(Line_Break);

}

//MPU Fault Handler routine
void MPUFaultISR()
{
    NVIC_INT_CTRL_R   |= 0x10000000;  //setting the pendSV active bit at bit #28  of the NVIC_INT_CTRL register to enable the PendSV ISR call
    NVIC_SYS_HND_CTRL_R  &= ~0x2000;     //clearing the MPU fault pending bit at bit #13 of the NVIC_SYS_HNDCTRL register
    //_delay_cycles(5);

    putsUart0(Line_Break);
    putsUart0("MPU Fault in process N \n\r");

    putsUart0("PSP: ");
    PrintIntToHex(getPSPaddress());
    putsUart0(Line_Break);

    putsUart0("MSP: ");
    PrintIntToHex(getMSPaddress());
    putsUart0(Line_Break);

    putsUart0("MFault Flag: ");
    PrintIntToHex(NVIC_FAULT_STAT_R | 0xFF); //accessing the MFault flags in the NVIC_FAULT_STAT_R at bits 0:7
    putsUart0(Line_Break);


    //Process Stack Dump
    uint32_t* PSP_Address = (uint32_t*) getPSPaddress();


    putsUart0("Offending instruction address: ");
    PrintIntToHex(*(PSP_Address + 6));
    putsUart0(Line_Break);

    putsUart0("Offending Data address: ");
    PrintIntToHex(NVIC_MM_ADDR_R);
    putsUart0(Line_Break);

    putsUart0("xPSR: ");
    PrintIntToHex(*(PSP_Address + 7));
    putsUart0(Line_Break);

    putsUart0("PC:   ");
    PrintIntToHex(*(PSP_Address + 6));
    putsUart0(Line_Break);

    putsUart0("LR:   ");
    PrintIntToHex(*(PSP_Address + 5));
    putsUart0(Line_Break);

    putsUart0("R12:  ");
    PrintIntToHex(*(PSP_Address + 4));
    putsUart0(Line_Break);

    putsUart0("R3:   ");
    PrintIntToHex(*(PSP_Address + 3));
    putsUart0(Line_Break);

    putsUart0("R2:   ");
    PrintIntToHex(*(PSP_Address + 2));
    putsUart0(Line_Break);

    putsUart0("R1:   ");
    PrintIntToHex(*(PSP_Address + 1));
    putsUart0(Line_Break);

    putsUart0("R0:   ");
    PrintIntToHex(*PSP_Address);
    putsUart0(Line_Break);

  //  NVIC_SYS_HND_CTRL_R  |= 0x400;

}

//PendSV Handler ISR
void PendSVHandler()
{
    putsUart0("Pendsv in process N");
    putsUart0(Line_Break);

    if(NVIC_FAULT_STAT_R | 0x1 || NVIC_FAULT_STAT_R | 0x2)  //if any of the DERR and IERR bits are set
    {
        if(NVIC_FAULT_STAT_R | 0x1)
            NVIC_FAULT_STAT_R &= ~0x1;

        if(NVIC_FAULT_STAT_R | 0x2)
            NVIC_FAULT_STAT_R &= ~0x2;

        putsUart0("Called from MPU");
        putsUart0(Line_Break);
    }
}




void init_MPU()
{
//    //region #0 for the background region
    //RW access (no X access) for both privileged and unprivileged mode for RAM, bitband and peripherals
    NVIC_MPU_NUMBER_R |= 0x0;        //select region 0
    NVIC_MPU_BASE_R   |= 0x00000000; //addr=0x0000.0000 for complete 4GiB of memory, valid=0, region already set
    NVIC_MPU_ATTR_R   |= 0x1304003E; //S=0, C=1, B=0, size=31(11111 for 4GiB), XN=1(instruction fetch disabled), TEX=000
    NVIC_MPU_ATTR_R   |= 0x1;        //MPU  region enable
//
//
    //region #1 for the 256kiB flash memory
    //RWX access for both privileged and unprivileged mode
    NVIC_MPU_NUMBER_R |= 0x1;        //select region 1
    NVIC_MPU_BASE_R   |= 0x00000000; //addr=0x00000000 for base address of flash bits, 256KiB, valid =0, region already set in the NUMBER register
    NVIC_MPU_ATTR_R   |= 0x03020022; //S=0, C=1, B=0, size=17(10001) for 256KB, XN=0(instruction fetch enabled), TEX=000
    NVIC_MPU_ATTR_R   |= 0x1;        //MPU  region enable
//
//
//
//    //we divide the 32KiB SRAM into 4 different regions in the MPU
//    //each of those regions in the MPU will protect 8KiB of SRAM each
//    //the 8KiB within the regions are further divided into 8 subregions of 1KiB each
//
    //first region of SRAM- region 2
    NVIC_MPU_NUMBER_R   = 0x2;       //select region 2
    NVIC_MPU_BASE_R    |= 0x20000000;//addr=0x20000000,  valid =0, region already set in NUMBER register
    NVIC_MPU_ATTR_R    |= 0x11060018;//for internal SRAM S=1, C=1, B=0, size=12(1100) for 8KiB, XN=1(instruction fetch disabled), TEX=000
                                     //all 8 of the subregions enabled
    NVIC_MPU_ATTR_R    |= 0x1;

    //second region of SRAM- region 3
    NVIC_MPU_NUMBER_R   = 0x3;       //select region 3
    NVIC_MPU_BASE_R    |= 0x20002000;//addr=0x20002000,  valid =0, region already set in NUMBER register
    NVIC_MPU_ATTR_R    |= 0x11060018;//for internal SRAM S=1, C=1, B=0, size=12(1100) for 8KiB, XN=1(instruction fetch disabled), TEX=000
                                     //all 8 of the subregions enabled
    NVIC_MPU_ATTR_R    |= 0x1;

    //third region of SRAM- region 4
    NVIC_MPU_NUMBER_R   = 0x4;       //select region 4
    NVIC_MPU_BASE_R    |= 0x20004000;//addr=0x20004000,  valid =0, region already set in NUMBER register
    NVIC_MPU_ATTR_R    |= 0x11060018;//for internal SRAM S=1, C=1, B=0, size=12(1100) for 8KiB, XN=1(instruction fetch disabled), TEX=000
                                     //all 8 of the subregions enabled
    NVIC_MPU_ATTR_R    |= 0x1;

    //fourth region of SRAM- region 5
    NVIC_MPU_NUMBER_R   = 0x5;       //select region 5
    NVIC_MPU_BASE_R    |= 0x20006000;//addr=0x20006000,  valid =0, region already set in NUMBER register
    NVIC_MPU_ATTR_R    |= 0x11068018;//for internal SRAM S=1, C=1, B=0, size=12(1100) for 8KiB, XN=1(instruction fetch disabled), TEX=000
                                     //all but the last subregion enabled
    NVIC_MPU_ATTR_R    |= 0x1;

//
    NVIC_MPU_CTRL_R    |= 0x7;       //MPU enable, default region enable, MPU enabled during hard faults


}

// Blocking function that returns only when SW1 is pressed
void waitPbPress()
{
    while(GPIO_PORTF_DATA_R & PUSH_BUTTON_MASK);
}


void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Configure LED and pushbutton pins
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | RED_LED_MASK;   // bits 1 and 3 are outputs, other pins are inputs
    GPIO_PORTF_DIR_R &= ~PUSH_BUTTON_MASK;               // bit 4 is an input
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | RED_LED_MASK;  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= PUSH_BUTTON_MASK | GREEN_LED_MASK | RED_LED_MASK;
                                                         // enable LEDs and pushbuttons
    GPIO_PORTF_PUR_R |= PUSH_BUTTON_MASK;                // enable internal pull-up for push button
}

void thread()
{
//thread code used to test the MPU configuration, called from Main
    uint32_t *pa;

    putsUart0(Line_Break);
    putsUart0("Trying to access an address that falls under subregion 31 of the SRAM");
    putsUart0(Line_Break);
    pa = (uint32_t*) 0x20007FFC;
   *pa = 0;

   putsUart0("Now trying to access an address that falls under subregion 0 of the SRAM");
   putsUart0(Line_Break);
   pa = (uint32_t*)  0x20000000;
   *pa = 0;
}


int main(void)
{
    NVIC_SYS_HND_CTRL_R |= 0x70000;
    setPSPaddress(0x20008000);

    setASPbit();
    initUart0();
    putsUart0("Starting \n\r");
    init_MPU();




    //privilegedMode();
    unprivilegedMode();
    thread();
//    initHw();




	return 0;
}
