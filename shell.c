/*
  BareMetalOS Project Milestone 2
  By: Geoffrey Tomlinson, Terry Cheney
  For: Dr. Song
  Created: 4/8/2021

  Creating a kernel and learning how to print a string, read from
  the keyboard, and read a sector from the disk.
*/

/* Initialize functions */
void handleCommand(char*);
int strcmp(char*, char*);

int main() {
    //char line[80];
    int len_1, i;
    char parsed_command[80];
    char program_buf[13312];
    int num_args;

    enableInterrupts();

    interrupt(0x21, 0, "\r\nSHELL>\0", 0, 0);
    interrupt(0x21, 1, parsed_command, 0, 0);
    interrupt(0x21, 0, "\r\n\0", 0, 0);
    handleCommand(parsed_command);
}

/* command is a double indexed char array containing
   the command at command[0] and an argument at command[1] */

void handleCommand(char* command) {
    char memory[13312];
    char dir_buff[512];
    char dir_list[6];
    char argument[7];
    char argument2[7];
    int i, j, end_of_arg_one;

    for (i = 0; i < 13312; i++) {
        memory[i] = '\0';
    }

    if (strcmp(command, "type \0") == 1) {
        for (i = 0; i < 6 && command[5 + i] != '\0'; i++) {
            argument[i] = command[5 + i];
        }
        interrupt(0x21, 3, argument, memory, 0);
        interrupt(0x21, 0, memory, 0, 0);
        return;
    } else if (strcmp(command, "execute \0") == 1) {
        for (i = 0; i < 6 && command[8 + i] != '\0'; i++) {
            argument[i] = command[8 + i];
        }
        interrupt(0x21, 4, argument, 0x2000, 0);
        return;
    } else if (strcmp(command, "dir\0") == 1) {
        interrupt(0x21, 0, "Files in directory:\r\n", 0, 0);
        interrupt(0x21, 2, dir_buff, 2, 0);
        for (i = 0; i < 16; i++) {
            for (j = 0; j < 6; j++) {
                dir_list[j] = dir_buff[32 * i + j];
            }
            dir_list[6] = '\0';
            if (dir_list[0] == 0) {
                break;
            }
            interrupt(0x21, 0, dir_list, 0, 0);
            interrupt(0x21, 0, "\r\n\0", 0, 0);
        }
        return;
    } else if (strcmp(command, "copy \0") == 1) {
        for (i = 0; i < 6 && command[5 + i] != '\0'; i++) {
            argument[i] = command[5 + i];
            end_of_arg_one = 7 + i;
        }
        for (i = 0; i < 6 && command[end_of_arg_one + i] != '\0'; i++) {
            argument2[i] = command[end_of_arg_one + i];
        }

        interrupt(0x21, 3, argument, memory, 0);
        interrupt(0x21, 8, argument2, memory, 26);
        return;

    } else if (strcmp(command, "kill \0") == 1) {
        for (i = 0; i < 6 && command[5 + i] != '\0'; i++) {
            argument[i] = command[5 + i];
        }
        if (argument[i] < '0' || argument[0] > '9') {
            return;
        }
        interrupt(0x21, 9, (int)argument[0] - '0', 0, 0);
        return;
    } else if (strcmp(command, "execforeground \0") == 1) {
        for (i = 0; i < 6 && command[15 + i] != '\0'; i++) {
            argument[i] = command[15 + i];
        }
        interrupt(0x21, 10, argument, 0, 0);
        return;
    } else {
        interrupt(0x21, 0, "\r\nBAD COMMAND\r\n\0", 0, 0);
    }
}

int strcmp(char* str1, char* str2) {
    int i;
    for (i = 0; str1[i] != '\0' && str2[i] != '\0'; i++) {
        if (str1[i] != str2[i]) {
            return 0;
        }
    }
    return 1;
}