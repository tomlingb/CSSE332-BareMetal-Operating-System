/*
  BareMetalOS Project Milestone 1
  By: Geoffrey Tomlinson, Terry Cheney
  For: Dr. Song
  Created: 3/25/2021

  Creating a kernel and learning how to print a string, read from
  the keyboard, and read a sector from the disk.
*/

#define ACTIVE 1
#define INACTIVE 0
#define WAITING 2
#define MAX_PROCESSES 8

/* Initialize functions */
void printString(char* chars);
void readString(char*);
int mod(int, int);
int div(int, int);
void readSector(char*, int);
void writeSector(char*, int);
void handleInterrupt21(int ax, int bx, int cx, int dx);
void readFile(char*, char*);
void writeFile(char*, char*, int);
void executeProgram(char*, int);
void handleTimerInterrupt(int, int);
void terminate();
void killProcess(int);

struct entry {
  int status;
  int sp;
  int waiting_on;
};

struct entry process_table[MAX_PROCESSES];

int currentProcess = -1;

/* Main Function */
int main() {
  int i;

  makeInterrupt21(); /* calls the makeInterrupt21 function from kernel.asm */

  for (i = 0; i < MAX_PROCESSES; i++) {
    process_table[i].status = INACTIVE;
    process_table[i].sp = 0xFF00;
  }

  makeTimerInterrupt();

  interrupt(0x21, 4, "shell\0", 0x2000, 0);

  while (1) {
  } /* stop the kernel */
}

/*
  PrintString Function
    prints a given string

  chars: string to be printed
*/
void printString(char* chars) {
  int j;
  j = 0;

  while (chars[j] != '\0') { /* stops execution on the last character of the string */
    interrupt(0x10, 0xe * 256 + chars[j], 0, 0, 0);
    j++;
  }
}

/*
  ReadString Function
    reads a line from the console using Interrupt 0x16 and stores in a given character array.

  line: empty array with space for 80 characters
*/
void readString(char* line) {
  int i, lineLength, ax;
  char charRead, backSpace, enter;
  lineLength = 80;
  i = 0;
  ax = 0;
  backSpace = 0x8;
  enter = 0xd;
  charRead = interrupt(0x16, ax, 0, 0, 0);

  while (charRead != enter && i < lineLength - 2) {

    if (charRead != backSpace) {
      interrupt(0x10, 0xe * 256 + charRead, 0, 0, 0);
      line[i] = charRead;
      i++;
    } else {
      i--;
      if (i >= 0) {
        interrupt(0x10, 0xe * 256 + charRead, 0, 0, 0);
        interrupt(0x10, 0xe * 256 + '\0', 0, 0, 0);
        interrupt(0x10, 0xe * 256 + backSpace, 0, 0, 0);
      } else {
        i = 0;
      }
    }
    charRead = interrupt(0x16, ax, 0, 0, 0);
  }
  line[i] = 0xa;
  line[i + 1] = 0x0;

  printString("\r\n"); /* new line */

  return;
}

/*
  Mod Function
    calculates (a MOD b)
*/
int mod(int a, int b) {
  int temp;
  temp = a;
  while (temp >= b) {
    temp = temp - b;
  }
  return temp;
}

/*
  Div Function
    Calculates (a DIV b) rounding down
*/
int div(int a, int b) {
  int quotient;
  quotient = 0;
  while ((quotient + 1) * b <= a) {
    quotient++;
  }
  return quotient;
}

/*
  ReadSector Function
    reads a sector from the floppy image and stores in a given character array

  buffer: empty character array with space for 512 characters
  sector: the number of the sector to be read
*/
void readSector(char* buffer, int sector) {
  int relSector, head, track;

  /* given calculations */
  relSector = mod(sector, 18) + 1;
  head = mod(div(sector, 18), 2);
  track = div(sector, 36);

  interrupt(0x13, 2 * 256 + 1, buffer, track * 256 + relSector, head * 256);
}

void writeSector(char* buffer, int sector) {
  int relSector, head, track;

  /* given calculations */
  relSector = mod(sector, 18) + 1;
  head = mod(div(sector, 18), 2);
  track = div(sector, 36);

  interrupt(0x13, 3 * 256 + 1, buffer, track * 256 + relSector, head * 256);
}

/*
  HandleInterrupt21
    interrupt service handler that decides what function to run based off provided arguments

  ax: decides what function to run
  bx: the character array to store or print from
  cx: the sector number (used only in case ax=2)
  dx: currently unused

  i.e. handleInterrupt21(2,buffer,30,0) calls readSector and stores in buffer
*/
void handleInterrupt21(int ax, int bx, int cx, int dx) {
  switch (ax) {
  case 0:
    printString(bx);
    break;

  case 1:
    readString(bx);
    break;

  case 2:
    readSector(bx, cx);
    break;

  case 3:
    readFile(bx, cx);
    break;

  case 4:
    executeProgram(bx, 0);
    break;

  case 5:
    terminate();
    break;

  case 6:
    writeSector(bx, cx);
    break;

  case 8:
    writeFile(bx, cx, dx);
    break;

  case 9:
    killProcess(bx);
    break;

  case 10:
    executeProgram(bx, 1);
    break;

  default: /* Prints an error message in the event of ax being an undefined value */
    printString("Error in interrupt21\0");
    printString("\r\n"); /* new line */
    break;
  }
}

void readFile(char* filename, char* buf) {
  char directory_buf[512];
  int flag = 0;
  int i = 0, j;
  char* test;
  readSector(directory_buf, 2);

  for (i = 0; i < 16; i++) {

    for (j = 0; j < 7; j++) {

      if (j == 6 || (filename[j] == '\0' && directory_buf[i * 32 + j] == 0)) {

        flag = 0;
        break;
      }
      // printString("Checking character: ");
      // printString(directory_buf[i * 32]);
      // printString(" ");
      if (directory_buf[i * 32 + j] != filename[j]) {
        flag = 1;
        break;
      }
    }
    // printString("Next file ");
    if (flag == 0) {
      // printString("Found file ");
      break;
    }
  }
  j = 0;
  if (flag == 0) {
    for (j = 6; j < 32; j++) {
      // printString("Found file ");
      readSector(buf, directory_buf[i * 32 + j]);
      buf += 512;
    }
  }
}

void writeFile(char* filename, char* buffer, int num_sectors) {
  char map_buffer[512];
  char dir_buffer[512];
  int sectors[26];
  int dir_offset, sector_index;
  int i, j;

  readSector(map_buffer, 1);
  readSector(dir_buffer, 2);

  for (dir_offset = 0; dir_offset < 512; dir_offset += 32) {
    if (dir_buffer[dir_offset] == '\0') {
      break;
    }
  }
  for (i = 0; i < 512 && sector_index < num_sectors; i++) {
    if (map_buffer[i] == 0) {
      sectors[sector_index++] = i;
    }
  }

  if (dir_offset >= 512) {
    return;
  }

  for (i = 0; i < num_sectors; i++) {
    writeSector(buffer + 512 * i, sectors[i]);
    map_buffer[sectors[i]] = 0xFF;
  }
  writeSector(map_buffer, 1);

  for (i = 0; i < 6; i++) {
    if (filename[i] != '\0') {
      dir_buffer[dir_offset + i] = filename[i];
    } else {
      for (j = i; j < 6; j++) {
        dir_buffer[dir_offset + j] = '\0';
      }
      break;
    }
  }

  for (i = 0; i < num_sectors; i++) {
    dir_buffer[dir_offset + i + 6] = (char)sectors[i];
  }
  for (i = num_sectors; i < 26; i++) {
    dir_buffer[dir_offset + i + 6] = 0x00;
  }

  writeSector(dir_buffer, 2);

}

void executeProgram(char* name, int wait) {
  char buf[13312];
  int i, j, segment;
  readFile(name, buf);
  if (buf[0] == '\0') {
    return;
  }

  setKernelDataSegment();
  for (i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].status == INACTIVE) {
      break;
    }
  }
  restoreDataSegment();

  if (i >= MAX_PROCESSES) {
    return;
  }

  segment = (i + 2) * 0x1000;
  for (j = 0; j < 13312; j++) {
    putInMemory(segment, j, buf[j]);
  }

  setKernelDataSegment();
  process_table[i].status = ACTIVE;
  process_table[i].sp = 0xFF00;
  process_table[i].waiting_on = -1;
  if (wait != 0 && currentProcess != -1) {
    process_table[currentProcess].status = WAITING;
    process_table[currentProcess].waiting_on = i;
  }
  restoreDataSegment();

  initializeProgram(segment);

}

void terminate() {
  int i;
  setKernelDataSegment();
  process_table[currentProcess].status = INACTIVE;
  for (i = 0; i < MAX_PROCESSES; i++) {
    if (process_table[i].waiting_on == currentProcess && currentProcess != i) {
      process_table[i].status = ACTIVE;
    }
  }
  while (1) {
    //do nothing
  }
  //interrupt(0x21, 4, "shell\0", 0x2000, 0);
}

void handleTimerInterrupt(int segment, int sp) {
  int i, nextSeg, nextSP;
  if (currentProcess == -1) {
    //do nothing
  } else {
    process_table[currentProcess].sp = sp;
  }

  for (i = currentProcess + 1;; i++) {
    if (i >= MAX_PROCESSES) {
      i = 0;
    }
    if (process_table[i].status == ACTIVE) {
      nextSeg = (i + 2) * 0x1000;
      nextSP = process_table[i].sp;
      currentProcess = i;
      break;
    }
  }
  returnFromTimer(nextSeg, nextSP);
}

void killProcess(int pid) {
  if (pid >= MAX_PROCESSES || pid < 0) {
    return;
  }
  setKernelDataSegment();
  process_table[pid].status = INACTIVE;
  restoreDataSegment();
}