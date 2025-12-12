#include <sym1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h> // For console I/O if available and configured
#include <6502.h>  // For 6502-specific features if needed

// Define the memory-mapped address of a specific port on the SYM-1
// This example assumes a VIA port at address $AC00 (example address)

void print_hex_byte(unsigned char value);

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

//
// Assume raw temperature, "data", is 0x1490
//   Shift right 4 bits (drop last nibble), so 0x149
//   
//   DEGREES_PER_BIT = 0.0625  (From datasheet)
//   Scale for fixed-notation: 10000 (0x2710)
//   0.0625 * 10000 = 625 == 0x271
//   0x149 * 0x271 = 0x32339 ==> 205625
//   data = 032339 / FIXED_POINT
//   temp_integer = data / FIXED_POINT
//   temp_fraction = data % FIXED_POINT
//   printf("temp: %d.%dC\n", temp_integer, temp_fraction);
//   shows: "temp: 20.56C"
//

#define DEGREES_PER_BIT  0x271
#define FIXED_POINT 0x64
//#define FIXED_POINT 0x271

long temp_integer;
long temp_fraction;


unsigned short Read_Register(unsigned char reg)
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

unsigned short Get_Reg_Data(unsigned char reg)
{
    unsigned short value = Read_Register(reg);

#if 1
    //printf("reg(%02x) = 0x%04x\n", reg, value);
#else
    // This reduces memory size significantly - by half
    // But the "reg" value doesn't print....why?
    fputs("reg(", stdout);
    utoa(reg, buffer, 16);
    fputs(") = 0x", stdout);
    itoa(value, buffer, 16);
    fputs(buffer, stdout);
    fputs("\n", stdout); 
#endif

    return value;
}

int Initialize(void)
{
    I2C_INIT();

    return (Read_Register(TMP1075_DIEID) == TMP1075_DEVICE_ID)? SUCCESS:FAILURE;
}

int main(void)
{
    puts("Built "__DATE__" "__TIME__);

    if (Initialize() == SUCCESS) {

        long data = Get_Reg_Data(TMP1075_TEMP);
#if 0        
        Get_Reg_Data(TMP1075_CFGR);
        Get_Reg_Data(TMP1075_LLIM);
        Get_Reg_Data(TMP1075_HLIM);
#endif

        data >>= 4;
        data *= DEGREES_PER_BIT;
        data /= FIXED_POINT;

        temp_integer  = data / FIXED_POINT;
        temp_fraction = data % FIXED_POINT;

        fputs("temp: ", stdout); 
        ltoa(temp_integer, buffer, 10); 
        fputs(buffer, stdout); 
        fputs(".", stdout); 
        ltoa(temp_fraction, buffer, 10);
        fputs(buffer, stdout); 
        puts("C"); 
    }

    return 0;
}
