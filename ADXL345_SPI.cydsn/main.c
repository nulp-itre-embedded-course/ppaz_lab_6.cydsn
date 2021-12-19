/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include <stdio.h>

// ADXL345 Register Definition
#define     RA_BW_RATE          0x2C    // R/W   0x0A   Data rate and power mode control
#define 	RA_DATA_FORMAT 	    0x31 	// R/W 	 0   	Data format control
#define 	RA_POWER_CTL 	    0x2D 	// R/W 	 0   	Power-saving features control
#define 	RA_DATAX0 	        0x32 	// R 	 0 	    X-Axis Data 0
#define 	RA_DATAX1 	        0x33 	// R 	 0 	    X-Axis Data 1
#define 	RA_DATAY0 	        0x34 	// R 	 0 	    Y-Axis Data 0
#define 	RA_DATAY1 	        0x35 	// R 	 0 	    Y-Axis Data 1
#define 	RA_DATAZ0 	        0x36 	// R 	 0 	    Z-Axis Data 0
#define 	RA_DATAZ1 	        0x37 	// R 	 0 	    Z-Axis Data 1
#define     RA_OFSX             0x1E    // R/W   0      X-axis offset
#define     RA_OFSY             0x1F    // R/W   0      Y-axis offset
#define     RA_OFSZ             0x20    // R/W   0      Z-axis offset

#define     SPI_READ            0x80
#define     SPI_WRITE           0x00
#define     SPI_MULTIPLE_BIT    0x40
#define     SPI_SINGLE_BIT      0x00

#define SCALE_FACTOR    3.9f 

float cal_X = 0, cal_Y = 0, cal_Z = 0; 
int16 x, y, z;
float xg, yg, zg;
char buffer[50]; 
char raw[6];    // raw data from regs: DATAX0, DATAX1, DATAY0, DATAY1, DATAZ0, DATAZ1

void Write_SPI(unsigned char reg, unsigned char data)       // write single byte via SPI
{
    unsigned char addr = reg | SPI_WRITE;

    SPIM_WriteTxData(addr);
    SPIM_WriteTxData(data);
    while((SPIM_ReadTxStatus() & SPIM_STS_SPI_DONE) != SPIM_STS_SPI_DONE);
}

unsigned char Read_SPI(unsigned char reg)                   // read single byte from SPI
{
    unsigned char data;
    unsigned char addr = reg | SPI_READ;

    SPIM_ClearRxBuffer();
    SPIM_WriteTxData(addr);
    SPIM_WriteTxData(0xFF);
    while((SPIM_ReadTxStatus() & SPIM_STS_SPI_DONE) != SPIM_STS_SPI_DONE);
    while(SPIM_GetRxBufferSize() < 2);
    SPIM_ReadRxData();
    data = SPIM_ReadRxData();
    SPIM_ClearRxBuffer();

    return data;
}

void ADXL345_Init(void)
{
    CyDelay(2u);
    Write_SPI(RA_POWER_CTL, 0x08);     //  START MEASUREMENT
    Write_SPI(RA_BW_RATE, 0x0A);       //  100 Hz
    Write_SPI(RA_DATA_FORMAT, 0x09);   //  +/-4g , 13-BIT MODE

    CyDelay(2u);
}

void Get_Accel_Values(void)
{
    x = ((Read_SPI(RA_DATAX1) << 8) | Read_SPI(RA_DATAX0)); 
    y = ((Read_SPI(RA_DATAY1) << 8) | Read_SPI(RA_DATAY0)); 
    z = ((Read_SPI(RA_DATAZ1) << 8) | Read_SPI(RA_DATAZ0)); 
    
    xg = SCALE_FACTOR * (float)x / 1000.0;
    yg = SCALE_FACTOR * (float)y / 1000.0;
    zg = SCALE_FACTOR * (float)z / 1000.0;   
}

void Read_XYZ()     // read acceleration values in one session
{
    unsigned char addr = RA_DATAX0 | SPI_READ | SPI_MULTIPLE_BIT;
    
    SPIM_ClearRxBuffer();
    SPIM_WriteTxData(addr);
    for (int i = 0; i < 6; i++)
        SPIM_WriteTxData(0x00);

    while((SPIM_ReadTxStatus() & SPIM_STS_SPI_DONE) != SPIM_STS_SPI_DONE);
    while(SPIM_GetRxBufferSize() < 7);
    SPIM_ReadRxData();
    
    for (int i = 0; i < 6; i++)
        raw[i] = SPIM_ReadRxData();
    
    x = (raw[1] << 8) | raw[0]; 
    y = (raw[3] << 8) | raw[2]; 
    z = (raw[5] << 8) | raw[4]; 
    
    xg = SCALE_FACTOR * (float)x / 1000.0;
    yg = SCALE_FACTOR * (float)y / 1000.0;
    zg = SCALE_FACTOR * (float)z / 1000.0; 
}

void Offset_Calibration(void)
{
    cal_X = -1 * ((Read_SPI(RA_DATAX1) << 8) | Read_SPI(RA_DATAX0));
    cal_Y = -1 * ((Read_SPI(RA_DATAY1) << 8) | Read_SPI(RA_DATAY0));
    cal_Z = -1 * ((Read_SPI(RA_DATAZ1) << 8) | Read_SPI(RA_DATAZ0));
    
    Write_SPI(RA_OFSX, cal_X);
    Write_SPI(RA_OFSY, cal_Y);
    Write_SPI(RA_OFSZ, cal_Z);
}

int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    SPIM_Start();
    Clock_Start();
    UART_Start();
    
    ADXL345_Init();
    //Offset_Calibration();
    
    UART_PutString("***ADXL345***");
    UART_PutString("\r\n");
    
    for(;;)
    {        
        Get_Accel_Values();     // reads data byte by byte in multiple sessions
        //Read_XYZ();           // read data in one session

        sprintf(buffer,"X: %3d   ", x);
        UART_PutString(buffer);

        sprintf(buffer,"Y: %3d   ", y);
        UART_PutString(buffer);

        sprintf(buffer,"Z: %3d   ", z);
        UART_PutString(buffer);
        
        sprintf(buffer,"Xg: %.2f   ", xg);
        UART_PutString(buffer);

        sprintf(buffer,"Yg: %.2f   ", yg);
        UART_PutString(buffer);

        sprintf(buffer,"Zg: %.2f   ", zg);
        UART_PutString(buffer);
        
        UART_PutString("\r\n");
        
        CyDelay(500);
    }
}

/* [] END OF FILE */
