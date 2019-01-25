#ifndef XBOO_H
#define XBOO_H

unsigned char rec[4];

int xboo_main();
void DelayLoop(float a, int b);
void FileCalc();
char SendData(unsigned char x);
void SendData32(unsigned char a, unsigned char b, unsigned char c, unsigned char d);
void Initialize();
void SendMainData();

#endif /* XBOO_H */
