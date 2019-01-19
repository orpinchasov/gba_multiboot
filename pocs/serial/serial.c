#include <windows.h>
#include <stdio.h>

#define ARDUINO_PORT ("COM5")

void configure_serial_port(HANDLE hComm)
{
    DCB dcbSerialParams = { 0 }; // Initializing DCB structure
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    int Status = 0;

    Status = GetCommState(hComm, &dcbSerialParams);

    dcbSerialParams.BaudRate = CBR_115200;  // Setting BaudRate = 115200
    dcbSerialParams.ByteSize = 8;           // Setting ByteSize = 8
    dcbSerialParams.StopBits = ONESTOPBIT;  // Setting StopBits = 1
    dcbSerialParams.Parity   = NOPARITY;    // Setting Parity = None

    SetCommState(hComm, &dcbSerialParams);

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout         = 100; // in milliseconds
    timeouts.ReadTotalTimeoutConstant    = 10; // in milliseconds
    timeouts.ReadTotalTimeoutMultiplier  = 50; // in milliseconds
    timeouts.WriteTotalTimeoutConstant   = 10; // in milliseconds
    timeouts.WriteTotalTimeoutMultiplier = 50; // in milliseconds

    SetCommTimeouts(hComm, &timeouts);
}

int write_to_serial(HANDLE hComm, unsigned char *data, int len)
{
    DWORD dNoOFBytestoWrite = len;   // No of bytes to write into the port
    DWORD dNoOfBytesWritten = 0;     // No of bytes written to the port
    int Status = 0;

    Status = WriteFile(hComm,        // Handle to the Serial port
                       data,         // Data to be written to the port
                       dNoOFBytestoWrite,  //No of bytes to write
                       &dNoOfBytesWritten, //Bytes written
                       NULL);
                     
    if (Status == 0) {
      printf("Error in WriteFile, status=%d, bytes_written=%d, last_error=%d\n", Status, dNoOfBytesWritten, GetLastError());
    } else {
      printf("Bytes written = %d\n", dNoOfBytesWritten);
    }

    return dNoOfBytesWritten;
}

void write_command_to_arduino(HANDLE hComm, unsigned char command, unsigned short value)
{
    unsigned char buffer[3] = {0};
    DWORD bytes_written = 0;
    int Status = 0;

    buffer[0] = (unsigned char)command;
    memcpy(&buffer[1], (char *)&value, sizeof(value));
    
    printf("writing: %02x%02x%02x\n", buffer[0], buffer[1], buffer[2]);

    write_to_serial(hComm, buffer, sizeof(buffer));    
}

int read_from_serial(HANDLE hComm, unsigned char *out, int len)
{
  DWORD dwBytesRead = 0;
  int Status = 0;

  Status = ReadFile(hComm, out, len, &dwBytesRead, NULL);
  if (Status == 0) {
      printf("Error in ReadFile, status=%d, bytes_read=%d, last_error=%d\n", Status, dwBytesRead, GetLastError());
  } else {
    printf("number of bytes read: %d\n", dwBytesRead);
  }
}

void loop(HANDLE hComm)
{
  unsigned char read_bytes[2] = {0};

  unsigned char data[2] = {0x00, 0x00};

  printf("Writing to serial %02x%02x\n", data[0], data[1]);
  write_to_serial(hComm, data, sizeof(data));
  read_from_serial(hComm, &read_bytes[0], sizeof(read_bytes));

  printf("Read bytes: %02x%02x\n", read_bytes[0], read_bytes[1]);
}

int main()
{
  HANDLE hComm;

  hComm = CreateFile(ARDUINO_PORT,                //port name
                      GENERIC_READ | GENERIC_WRITE, //Read/Write
                      0,                            // No Sharing
                      NULL,                         // No Security
                      OPEN_EXISTING,// Open existing port only
                      0,            // Non Overlapped I/O
                      NULL);        // Null for Comm Devices
                      

  if (hComm == INVALID_HANDLE_VALUE) {
      printf("Error in opening serial port\n");
      return 1;
  }
  else {
      printf("opening serial port successful\n");
  }
  
  PurgeComm(hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);

  configure_serial_port(hComm);

  Sleep(1000);

  unsigned char read_bytes[2] = {0};
  
  //loop(hComm);
  
  
  write_command_to_arduino(hComm, 0, 0);
  write_command_to_arduino(hComm, 1, 0);
  
  //Sleep(100);

  read_from_serial(hComm, &read_bytes[0], sizeof(read_bytes));
  printf("bytes: %02x%02x\n", read_bytes[1], read_bytes[0]);
  
  write_command_to_arduino(hComm, 0, 0x7202);
  write_command_to_arduino(hComm, 1, 0);
  
  //Sleep(100);
  
  read_from_serial(hComm, &read_bytes[0], sizeof(read_bytes));
  printf("bytes: %02x%02x\n", read_bytes[1], read_bytes[0]);

  CloseHandle(hComm);//Closing the Serial Port

  return 0;
}
