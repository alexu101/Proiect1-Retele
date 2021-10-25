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
#include <string.h>
#include <sys/wait.h>
#include <utmp.h>
#include <sys/socket.h>
int logged = 0; //variabila ce ne spune daca userul e logat sau nu

//subprogram ce prefixeaza un sir cu lungimea sa
void createResponse(char s[200], char response[200])
{
    char aux[200];
    int len = strlen(s);
    sprintf(aux, "%d", len);
    strcat(aux, " ");
    strcat(aux, s);
    strcpy(response, aux);
}

//subprogram care in functie de comanda trimite un raspuns la client
void getResponse(char command[200], char response[200])
{
    if (strncmp("login : ", command, 8) == 0) //comanda de login
    {
        logged = 1; //marcam faptul ca s-a logat
        //extrage din comanda primita numelui utiizatorului
        char user_name[200];
        strcpy(user_name, command + (strchr(command, ':') - command) + 1);
        strcpy(user_name, user_name + 1);

        //verificare daca usernameul introdus exista in fisierul de useri
        char actual_user_name[200];
        FILE *ufd;
        ufd = fopen("users", "r");
        if (ufd == NULL)
        {
            printf("error opening user file\n");
        }
        int ok = 0;                                                  // variabila ce determina daca exista userul
        while (fgets(actual_user_name, 200, ufd) != NULL && ok == 0) //cautam userul in
        {
            actual_user_name[strcspn(actual_user_name, "\n")] = '\0'; //eliminam endlineul de la finalul cuvantului citit din fisierul users
            if (strcmp(actual_user_name, user_name) == 0)             //daca am gasit userul in fisier
                ok = 1;                                               //oprim cautarea
        }

        fclose(ufd); //ichidem fisierul

        //cream raspunsul aferent userului
        if (ok == 1)
        {
            //createResponse("Successfully logged in!\n", response);
        }
        else
        {
            //createResponse("Unknown user! Try again!\n", response);
        }
    }

    else if (strncmp("get-proc-info :", command, 15) == 0) //comanda get-proc-info : pid
    {
        if (logged == 1) //comanda se executa doar daca este logat
        {                //extrage din comanda primita pidul procesului
            char pid_name[200];
            char pid_status_filename[200];
            char actual_pid_name[200];
            strcpy(pid_name, command + (strchr(command, ':') - command) + 1);
            strcpy(pid_name, pid_name + 1); //scapam de spatiu ' ' de dupa ':'

            //crearea numelui fisierului /proc/pid/stat
            strcpy(pid_status_filename, "/proc/");
            strcat(pid_status_filename, pid_name); //adaugam in nume pidul procesului din comanda
            strcat(pid_status_filename, "/status");

            //verificare daca pidul procesului introdus exista
            FILE *ufd;
            ufd = fopen(pid_status_filename, "r");

            char rsp[200]; //sirul in care vom construi raspunsul(fara prefix)
            int ok;
            if (ufd == NULL) //fisierul asociat pidului nu exista
            {
                printf("Process doesn't exist!\n");
                ok = 0;
            }
            else // daca exista extragem datele cerute
            {
                ok = 1; //marcam faptul ca procesul exista

                //ne creeam respunsul treptat cu toate informatiile
                char info[200];
                while (fgets(info, 200, ufd))
                    if (strstr(info, "State:"))
                    {
                        char s[200];
                        strcpy(s, info);
                        createResponse(s, info);
                        strcpy(rsp, info);
                    }
                    else if (strstr(info, "PPid:"))
                    {
                        char s[200];
                        strcpy(s, info);
                        createResponse(s, info);
                        strcat(rsp, info);
                    }
                    else if (strstr(info, "Uid:"))
                    {
                        char s[200];
                        strcpy(s, info);
                        createResponse(s, info);
                        strcat(rsp, info);
                        strcat(rsp, "\n");
                    }
            }

            fclose(ufd); //ichidem fisierul

            //raspuns daca pidul nu exista
            if (ok == 0)
            {
                createResponse("Pid doesn't exist! Try again!\n", response);
            }
            else //daca exista
            {
                strcpy(response, rsp);
            }
        }
        else
        {
            createResponse("Login to execute this command!\n", response);
        }
    }

    else if (strcmp(command, "get-logged-users") == 0) //comanda get-logged-users
    {
        if (logged)
        {
            struct utmp *ptr;
            ptr = getutent(); //acesam informatiile userilor

            while (ptr != NULL)
            {
                char rsp[200];
                strcat(rsp, "username: ");
                strcat(rsp, ptr->ut_user); //extragem usernameul
                strcat(rsp, "; hostname: ");
                strcat(rsp, ptr->ut_host); //extragem hostul
                char tm[200];
                sprintf(tm, "%d", ptr->ut_tv.tv_sec); //exragem timpul intrarii
                strcat(rsp, "; time entry was made: ");
                strcat(rsp, tm);
                createResponse(rsp, response); //cream raspusul final
            }
        }
        else
        {
            createResponse("Login to execute this command!\n", response);
        }
    }

    if (strcmp(command, "quit") == 0)
    {
        logged = 0; //il si delogam
        strcpy(response, "21 Successfully quitted\n");
    }

    else

        if (strcmp(command, "logout") == 0)
    {
        logged = 0;
        strcpy(response, "24 Successfully logged out\n");
    }

    else
    {
        strcpy(response, "77 Unknown command!\nTry: <login : username>, <get-logged-users> , <get-proc-info : pid>, <quit>, <logout>");
    }
}

int main()
{
    // fifo pentru transmiterea comenzilor
    int ec = mkfifo("ctsfifo", 0777); // cts = Client To Server
    if (ec == -1 && errno != EEXIST)
    {
        printf("Error creating fifo file\n");
        return 1;
    }

    //fifo pentru transmiterea raspunsurilor inapoi catre server
    int ec1 = mkfifo("stcfifo", 0777); // stc = Server To Client
    if (ec1 == -1 && errno != EEXIST)
    {
        printf("Error creating fifo file\n");
        return 1;
    }

    char command[200], response[200];

    //deschid fisierul fifo pentru citire comenzii
    int fd1 = open("ctsfifo", O_RDONLY);
    //primirea comenzii de la client
    read(fd1, &command, sizeof(command));
    close(fd1);

    //generarea raspunsului ce trebuie trimis catre client
    getResponse(command, response);
    //deschiderea fisierului pentru trimiterea raspunsului
    int fd2 = open("stcfifo", O_WRONLY);
    //trimiterea raspunsului inapoi catre client
    write(fd2, &response, sizeof(response));
    fsync(fd2);
    close(fd2);

    while (strcmp(command, "quit") != 0)
    {
        printf("Recieved command is'%s' \n", command); //temporara

        if(strcmp(command,"get-logged-users"))//folosesc socketuri pentru comanda aceasta
		{
            int sockets[2], pr;
            char buf[1024];
            if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
            {
                printf("error opening socket pair");
                return -1;
            }
            if ((pr == fork()) == -1)
                {
                    printf("fork error\n");
                }
            else if (pr != 0)
            {	//procesul parinte
                close(sockets[0]);
                //deschid fisierul fifo pentru citire comenzii din client
                int fd1 = open("ctsfifo", O_RDONLY);
                if (fd1 == -1) //testez posibile erori la deschidere
                    return 1;
                read(fd1, &command, sizeof(command));
                close(fd1);
                write(sockets[1], &command, sizeof(command));//trimit comanda catre copil
                fsync(sockets[1]);

                read(sockets[1], &buf, sizeof(buf));//citesc raspunsul de la copil
                getResponse(command,response);
                createResponse(buf, response);
                //deschid fisierul pentru trimiterea raspunsului catre client
                int fd2 = open("stcfifo", O_WRONLY);
                if (fd2 == -1) //testez posibile erori la deschidere
                    return 1;
                //trimiterea raspunsului
                write(fd2, &response, sizeof(response));
                fsync(fd2);
                close(fd2); //am trimis datele catre client
            }
            else
            {
                //proces copil
                //citesc comanda transmisa de parinte prin socket
                char cmd[200];
                read(sockets[0], &cmd, sizeof(cmd));

                //generarea raspunsului ce trebuie trimis catre tata
                getResponse(cmd, response); //l am generat
                //trimiterea raspunsului catre tata prin socket
                write(sockets[0], &response, sizeof(response));
                fsync(sockets[0]);
            }

		}
		else
        {
            //pipe pentru transmiterea comenzii catre copil
            int pfd1[2], pfd2[2];
            if (pipe(pfd1) == -1)
            {
                return 1;
            }

            if (pipe(pfd2) == -1)
            {
                return 1;
            }

            int status;
            int pid = fork();
            if (pid < 0)
            {
                return 2;
            }

            if (pid == 0)
            {
                //child process
                //citesc comanda transmisa de parinte prin primul pipe
                char cmd[200];
                close(pfd1[1]);
                read(pfd1[0], &cmd, sizeof(cmd));
                close(pfd1[0]); //am citit comanda de la tata

                //generarea raspunsului ce trebuie trimis catre tata
                getResponse(cmd, response); //l am generat
                //trimiterea raspunsului catre tata prin pipeul 2
                close(pfd2[0]);
                write(pfd2[1], &response, sizeof(response));
                fsync(pfd2[1]);
                close(pfd2[1]); //am trimis raspunsul la tata
            }
            else
            {
                //parent process
                char rsp[200];

                //trimit comanda catre copil prin pipeul 1
                //deschid fisierul fifo pentru citire comenzii din client
                int fd1 = open("ctsfifo", O_RDONLY);
                if (fd1 == -1) //testez posibile erori la deschidere
                    return 1;
                read(fd1, &command, sizeof(command));
                close(fd1);

                close(pfd1[0]);
                //directionarea catre copil prin pipe-ul 1
                write(pfd1[1], &command, sizeof(command));
                fsync(pfd1);
                close(pfd1[1]); //am trimis datele catre copil

                //primirea rezultatului de la copil prin pipeul 2
                close(pfd2[1]);
                read(pfd2[0], &rsp, sizeof(rsp));
                close(pfd2[0]); //am primit rezultatul de la copil

                getResponse(command,response);
                createResponse(rsp, response);
                //deschid fisierul pentru trimiterea raspunsului catre client
                int fd2 = open("stcfifo", O_WRONLY);
                if (fd2 == -1) //testez posibile erori la deschidere
                    return 1;
                //trimiterea raspunsului
                write(fd2, &response, sizeof(response));
                fsync(fd2);
                close(fd2); //am trimis datele catre client
            }
        }
    }
return 0;
}