#ifndef CLI_DATA_H
#define CLI_DATA_H

void read_serial_cli(void* param);
void print_motd();
void handle_cmd(const char* cmd);
void trim_newline(char* str);

#endif // CLI_DATA_H
