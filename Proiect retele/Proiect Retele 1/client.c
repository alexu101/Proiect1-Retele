#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{	
	// fifo pentru transmiterea comenzilor
	int ec = mkfifo("ctsfifo",0777);// cts = Client To Server
	if(ec == -1 && errno!=EEXIST)
		{
			printf("Error creating fifo file\n");
			return 1;
		}

	//fifo pentru transmiterea raspunsului
	int ec1 = mkfifo("stcfifo",0777);// stc = Server To Client
	if(ec1 == -1 && errno!=EEXIST)
		{
			printf("Error creating fifo file\n");
			return 1;
		}


	// citirea unei comenzi
	char command[25], response[25];
	printf("Enter command : ");
	scanf("%[^\n]%*c", command);

	while(strcmp(command,"quit")!=0)
	{
	//deschid fisierul fifo pentru citirea comenzii
	int fd1 = open("ctsfifo", O_WRONLY);
	
	if( fd1 == -1)//testez posibile erori la deschidere
		return 1;

	write(fd1, &command, sizeof(command));
	//printf("Sent command '%s' \n",command);
	close(fd1);
	//deschiderea fisierului pentru primirea raspunsului
	int fd2 = open("stcfifo", O_RDONLY);
	if( fd2 == -1)//testez posibile erori la deschidere
		{printf("alx\n");return 1;}
	read(fd2,&response,sizeof(response));
	close(fd2);
	printf("%s\n\n",response);
	

	printf("Enter command : ");
	scanf("%[^\n]%*c", command);

}

return 0;
}