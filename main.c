#include <sym1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h> // For console I/O if available and configured
#include <6502.h>  // For 6502-specific features if needed

// Define the memory-mapped address of a specific port on the SYM-1
// This example assumes a VIA port at address $AC00 (example address)

extern void I2C_INIT(void);
extern void I2C_START(void);
extern void I2C_STOP(void);
extern void I2C_WRITE_BYTE(unsigned char reg);
extern unsigned char I2C_READ_BYTE(void);
extern void I2C_SET_ACK(void);
extern void I2C_SET_NACK(void);

#define TMP1075_ADDR_R  0x91
#define TMP1075_ADDR_W  0x90

#define TMP1075_TEMP    0x00
#define TMP1075_CFGR    0x01
#define TMP1075_LLIM    0x02
#define TMP1075_HLIM    0x03
#define TMP1075_DIEID   0x0F

#define TMP1075_DEVICE_ID  0x7500

#define FAILURE   -1
#define SUCCESS    0

union {
    unsigned char RegAsBytes [2];
    unsigned short RegAsUShort;
} u;

char buffer[10];

//---------------------------------------------------------------------------
// Assume raw temperature, "data", is 0x1490
//   Shift right 4 bits (drop last nibble), so 0x149
//   
//   DEGREES_PER_BIT = 0.0625  (From datasheet)
//   Scale for fixed-notation: 10000 (0x2710)
//   0.0625 * 10000 = 625 == 0x271
//   0x149 * 0x271 = 0x32339 ==> 205625
//   data = 032339 / FIXED_POINT
//   fixed_point.integer = data / FIXED_POINT
//   fixed_point.fraction = data % FIXED_POINT
//   printf("temp: %d.%dC\n", fixed_point.integer, fixed_point.fraction);
//   shows: "temp: 20.56C"
//---------------------------------------------------------------------------

#define DEGREES_PER_BIT  0x271
#define FIXED_POINT 0x64

struct {
    long integer;
    long fraction;    
} fixed_point;

//---------------------------------------------------------------------------
// Print a single hex character
//---------------------------------------------------------------------------
void print_hex_byte(unsigned byte) 
{
    (void) byte;

    asm("jsr $82FA");  // OUTBYT
}

//---------------------------------------------------------------------------
// Print a null-terminated (ASCIIZ) string
//---------------------------------------------------------------------------
void print_string(char * str)
{
    (void) str;

    __asm__ ("      sta $FB     ");  // save str ptr to zero-page variable
    __asm__ ("      stx $FC     ");
    __asm__ ("      ldy #$00    ");
    __asm__ ("@loop:            ");
    __asm__ ("      lda ($FB),y ");
    __asm__ ("      beq @done   ");
    __asm__ ("      jsr $8A47   ");   // OUTCHR
    __asm__ ("      iny         ");
    __asm__ ("      bne @loop   ");
    __asm__ ("@done:            ");
}

//---------------------------------------------------------------------------
// Read a device 16-bit register
//---------------------------------------------------------------------------
unsigned short Read_Reg16(unsigned char reg)
{
    I2C_START();
    I2C_WRITE_BYTE(TMP1075_ADDR_W);
    I2C_WRITE_BYTE(reg);

    I2C_START();
    I2C_WRITE_BYTE(TMP1075_ADDR_R);
    asm("clc");
    u.RegAsBytes[1] = I2C_READ_BYTE();   // note endian-ness here
    asm("sec");
    u.RegAsBytes[0] = I2C_READ_BYTE();

    I2C_STOP();

    return u.RegAsUShort;
}

//---------------------------------------------------------------------------
// Get a  device 16-bit reg and show it.
//---------------------------------------------------------------------------
unsigned Get_Reg16(unsigned char reg)
{
    unsigned value = Read_Reg16(reg);

    print_string("reg(");
    print_hex_byte(reg);
    print_string(") = 0x");
    itoa(value, buffer, 16);
    print_string(buffer);
    print_string("\n\r");

    return value;
}

//---------------------------------------------------------------------------
// Convert and show a temperature type value
//---------------------------------------------------------------------------
void show_temperature(char * label, unsigned long value)
{
    value >>= 4;
    value *= DEGREES_PER_BIT;
    value /= FIXED_POINT;

    fixed_point.integer  = value / FIXED_POINT;
    fixed_point.fraction = value % FIXED_POINT;

    print_string(label); 
    itoa(fixed_point.integer, buffer, 10); 
    print_string(buffer); 
    print_string("."); 
    itoa(fixed_point.fraction, buffer, 10);
    print_string(buffer); 
    print_string("C\n\r"); 
}

//---------------------------------------------------------------------------
// Initialize I2C driver, with check for device ID.
//---------------------------------------------------------------------------
int Initialize(void)
{
    I2C_INIT();

    return (Read_Reg16(TMP1075_DIEID) == TMP1075_DEVICE_ID)? SUCCESS:FAILURE;
}

//---------------------------------------------------------------------------
// Main application
//---------------------------------------------------------------------------
int main(void)
{
    puts("Built "__DATE__" "__TIME__);

    if (Initialize() == SUCCESS) {

        long temp = Get_Reg16(TMP1075_TEMP);
#if 1
        unsigned long cfg  = Get_Reg16(TMP1075_CFGR);
        unsigned long llim = Get_Reg16(TMP1075_LLIM);
        unsigned long hlim = Get_Reg16(TMP1075_HLIM);

        // format CFG
        print_string("cfg: 0x"); 
        itoa(cfg, buffer, 16); 
        print_string(buffer);  
        print_string("\n\r"); 

        show_temperature("llim: ", llim); 
        show_temperature("hlim: ", hlim); 
#endif
        show_temperature("temp: ", temp);
    }

    return 0;
}
