#ifndef CLI_TH_H
#define CLI_TH_H

void read_serial_cli(void* param);
void print_motd();
void print_help();
void handle_cmd(const char* cmd);
void trim_newline(char* str);

#endif // CLI_TH_H
