#include "shell.h"
#include <stddef.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "host.h"

extern const unsigned char _sromfs;

static uint32_t get_unaligned(const uint8_t* d){
	return ((uint32_t)d[0]) | ((uint32_t)(d[1]<<8)) | ((uint32_t)(d[2]<<16)) | ((uint32_t)(d[3] <<24));
}


typedef struct {
	const char *name;
	cmdfunc *fptr;
	const char *desc;
} cmdlist;

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);

int fibonacci(int num);
int myatoi(const char*);
void myitoa(char* buf, int num);

#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}

cmdlist cl[]={
	MKCL(ls, "List directory"),
	MKCL(man, "Show the manual of the command"),
	MKCL(cat, "Concatenate files and print on the stdout"),
	MKCL(ps, "Report a snapshot of the current processes"),
	MKCL(host, "Run command on host"),
	MKCL(mmtest, "heap memory allocation test"),
	MKCL(help, "help"),
	MKCL(test, "test new function")
};

int parse_command(char *str, char *argv[]){
	int b_quote=0, b_dbquote=0;
	int i;
	int count=0, p=0;
	for(i=0; str[i]; ++i){
		if(str[i]=='\'')
			++b_quote;
		if(str[i]=='"')
			++b_dbquote;
		if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0){
			str[i]='\0';
			argv[count++]=&str[p];
			p=i+1;
		}
	}
	/* last one */
	argv[count++]=&str[p];

	return count;
}

void ls_command(int n, char *argv[]){

	const uint8_t * meta;
	meta = &_sromfs; 
	while(get_unaligned(meta) && get_unaligned(meta+4))
	{
		fio_printf(1,"\r\n%s",(meta+8));
		long int a = get_unaligned((meta+4));
		meta = meta + (20+a);
	}
	fio_printf(1,"\r\n");

}

int filedump(const char *filename){
	char buf[128];

	int fd=fs_open(filename, 0, O_RDONLY);

	if(fd==OPENFAIL)
		return 0;

	fio_printf(1, "\r\n");

	int count;
	while((count=fio_read(fd, buf, sizeof(buf)))>0){
		fio_write(1, buf, count);
	}

	fio_close(fd);
	return 1;
}

void ps_command(int n, char *argv[]){
	signed char buf[1024];
	vTaskList(buf);
        fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
        fio_printf(1, "*******************************************\n\r");
	fio_printf(1, "%s\r\n", buf + 2);	
}

void cat_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: cat <filename>\r\n");
		return;
	}

	if(!filedump(argv[1]))
		fio_printf(2, "\r\n%s no such file or directory.\r\n", argv[1]);
}

void man_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: man <command>\r\n");
		return;
	}

	char buf[128]="/romfs/manual/";
	strcat(buf, argv[1]);

	if(!filedump(buf))
		fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[]){
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1){
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } 
    else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void help_command(int n,char *argv[]){
	int i;
	fio_printf(1, "\r\n");
	for(i=0;i<sizeof(cl)/sizeof(cl[0]); ++i){
		fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
	}
}

void test_command(int n, char *argv[]) {
    int handle;
    int error;

    fio_printf(1, "\r\n");
    
   /* handle = host_action(SYS_OPEN, "output/syslog", 8);
    if(handle == -1) {
	handle = host_action(SYS_SYSTEM,"mkdir output");
	if(handle == -1)
	{
	    fio_printf(1, "Open file error!\n\r");
            return;
        }
        handle = host_action(SYS_OPEN, "output/syslog", 8);
    }*/
    handle = host_action(SYS_OPEN, "output/test", 8);
    if(handle == -1) {
        handle = host_action(SYS_SYSTEM,"mkdir output");
        if(handle == -1)
        {
            fio_printf(1, "Open file error!\n\r");
            return;
        }
        handle = host_action(SYS_OPEN, "output/test", 8);
    }
    
    int num = myatoi(argv[1]);
    fio_printf(1,"Fibonacci number %d is %d\r\n", num, fibonacci(num));
    
    char *buffer = "Fibonacci number ";
    char *buffer1 = " is " ;
    char buffer3[] = {0};
    myitoa(buffer3, fibonacci(num));

 //   char *buffer = ""
  //  char *buffer = "Test host_write function which can write data to output/syslog\n";
    error = host_action(SYS_WRITE, handle, (void *)buffer, strlen(buffer));
    error += host_action(SYS_WRITE, handle, (void *)argv[1], strlen(argv[1]));
    error += host_action(SYS_WRITE, handle, (void *)buffer1, strlen(buffer1));
    error += host_action(SYS_WRITE, handle, (void *)buffer3, strlen(buffer3));
    if(error != 0) {
        fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
        host_action(SYS_CLOSE, handle);
        return;
    }

    host_action(SYS_CLOSE, handle);
}

cmdfunc *do_command(const char *cmd){

	int i;

	for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i){
		if(strcmp(cl[i].name, cmd)==0)
			return cl[i].fptr;
	}
	return NULL;	
}

int fibonacci(int num)
{
	if(num <= 0) return 0;
	if(num <= 1) return 1;
	return fibonacci(num-1) + fibonacci(num-2);
}

int myatoi(const char* str)
{
	int result = 0;
	while(*str!=0){
	result *= 10;
	result += *str-'0';
	str++;
	}
	return result;
}

void myitoa(char* buf, int num)
{
	// check how long the number is
	int length = 0;
	int i = 0;
	for(i = 1 ; i<num; i *=10){
	length += 1;
	}
	// max digit
	int max = 1;
	for(i=0; i<(length-1); i++){
		max *= 10;
	}
	// translate to char
	for(i=0; i<length; i++){
	buf[i] = (char)(num/max)+'0';
	num = num % max;
	max = max / 10;
	}
	buf[i] = '\0';
}	

