#include "common.h"

volatile bool connected = false;
pthread_mutex_t connec_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t pause_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pause_cond = PTHREAD_COND_INITIALIZER;
volatile bool pause_thread = false;

Client *    client_init(short server_port);
void        client_thread_init(Client *client);
void *      client_send(void *pclient);
void *      client_recv(void *client);
void        print_active_users(char buffer[]);
void        client_send_info(Client *client);
void        client_sendline(Client *client, char buffer[], int limit);
void        client_recvline(Client *client, char buffer[], int limit);
void        client_send_message(Client *client, MSG_TYPE request);
void        client_choose_partner(Client *client);

int main(int argc, char *argv[])
{
    char choice;
    char str_servaddr[MAXLINE];

    // Deciding on a server address 
    if (argc != 2)
    {
        printf("USAGE: %s <ipaddr>\n", argv[0]);
        printf("[?] Check localhost (y or n): ");
        if ((choice = getchar()) == 'y')
        {
            printf("[*] Checking localhost ....\n");
            snprintf(str_servaddr,IPV4_STRLEN, LOCALHOST) ;
        }
        else
        {
            printf("[!] Quiting!\n");
            return 1;
        }
        CLEAR_STDIN();
    }
    else
    {
        printf("[*] Checking %s .... \n", argv[1]);
        snprintf(str_servaddr, MAXLINE, "%s", argv[1]);
    }
    printf("[!] CONNECTED TO %s\n", 
            strcmp(str_servaddr,LOCALHOST) == 0 ? "localhost":str_servaddr);
    

    // Creating the client struct
    Client *client = client_init(SERVER_PORT);

    printf("[!] ENTER \"exit()\" TO QUIT '(^u^)'\n");
    client_thread_init(client);

    // Clean up
    printf("[!] Client Closed!\n");
    close(client->socket);
    free(client);

    return 0;
}

void client_thread_init(Client *client)
{
    pthread_t  tid1; 
    pthread_t  tid2;

    // Thread1: send message
    if (pthread_create(&tid2,NULL, client_send, client) != 0)
        ERROR_HANDLE();

    // Thread2: recieve message
    if (pthread_create(&tid1,NULL, client_recv, client) != 0)
        ERROR_HANDLE();

    pthread_join(tid2, NULL);
}

void * client_send(void *pclient)
{
    Client *client = (Client *)pclient;
    char pre_mssg[MAXLINE];
    char sendline[MAXLINE];
    char chr;

    snprintf(pre_mssg, MAXLINE, "%s : ", client->name);

    while (connected)
    {
        
        pthread_mutex_lock(&pause_lock);
        while (pause_thread)
        {
            pthread_cond_wait(&pause_cond, &pause_lock);
        }
        
        cstring_input(pre_mssg, sendline);
        pthread_mutex_unlock(&pause_lock);
        
        if (strcmp(sendline, "exit()") == 0)
        {
            printf("\n[?] Do you want to exit (y or n): ");
            if ((chr = getchar()) == 'n')
            {
                if (!pause_thread)
                {
                    cstring_input(NULL, sendline);
                }
                else 
                {
                    continue;
                }
            }
            else
            {
                pthread_mutex_lock(&connec_lock);
                connected = false;
                pthread_mutex_unlock(&connec_lock);
                return NULL;
            }
            CLEAR_STDIN();
        }
        else if (strcmp(sendline , "\0") == 0)
        {
            continue;
        }
        else if (strcmp(sendline, "cls") == 0)
        {
            system("clear");
            continue;
        }
        //TESTING
        else if (strcmp(sendline , "a") == 0)
        {
            client_send_message(client, ACTIVE_USERS);
        }
        else if (strcmp(sendline, "m") == 0)
        {
            client_send_message(client, CLIENT_WANTS_TO_CHAT);
        }
        else if (strcmp(sendline, "c") == 0)
        {
            client_send_message(client, CLIENT_CHOOSE_PARTNER);
        }
        else 
            client_sendline(client, sendline, MAXLINE);

        // FIXME:
        // hackish way for the proc to slow down and not take 
        // concurrent or accidental inputs
        /*CLEAR_STDIN();*/
    }
    return NULL;
}

void * client_recv(void *pclient)
{
    Client *client = (Client *)pclient;
    char recvline[MAXLINE];
    char choice[7];

    while (connected)
    {
        client_recvline(client, recvline, MAXLINE);

        switch(cstr_to_msg(recvline))
        {
            case ACTIVE_USERS:
                client_recvline(client, recvline, MAXLINE);
                print_active_users(recvline);
                break;

            case CLIENT_CHOOSE_PARTNER:
                /*
                printf("[!] Waiting ...\n");
                printf("Lock enabled\n");
                pthread_mutex_lock(&pause_lock);
                pause_thread = true;
                pthread_mutex_unlock(&pause_lock);
                printf("Lock disabled\n");
                */

                printf("[?] Who do u want to talk with: ");
                /*client_choose_partner(client);*/
                break;

            case CLIENT_WANTS_TO_CHAT:
                client_send_message(client, CLIENT_WANTS_TO_CHAT);
                break;

            case ASK:
                do {
                    cstring_input("[?] choice (yes or no): ", choice);
                    for (int i = 0; choice[i]; i++)
                        choice[i] = tolower(choice[i]);
               } while (strcmp(choice, "yes") != 0 && strcmp(choice, "no") != 0);

                client_sendline(client, choice, 4);
                break;

            case PAUSE_THREAD:
                printf("[!] Waiting ...\n");
                printf("Lock enabled\n");
                pthread_mutex_lock(&pause_lock);
                pause_thread = true;
                pthread_mutex_unlock(&pause_lock);
                printf("Lock disabled\n");
                sleep(1);
                break;

            case UNPAUSE_THREAD:
                printf("[!] Continue ...\n");
                pthread_mutex_lock(&pause_lock);
                pause_thread = false;
                pthread_cond_signal(&pause_cond);
                pthread_mutex_unlock(&pause_lock);
                break;

            case CLIENT_UNAVAILABLE:
                fprintf(stderr, "[!] Client Unavailable\n");
                break;
            case CLIENT_NOT_FOUND:
                fprintf(stderr, "[!] Client not found\n");
                break;
            case CLIENT_SAME_USER:
                fprintf(stderr, "[!] You cant talk to yourself\n");
                break;
            default:
                printf("\n%s\n", recvline);
                break;
        }
    }
    return NULL;

}

Client * client_init(short server_port)
{
    Client *client = malloc(sizeof(Client));

    do {
        cstring_input("[?] Name: ", client->name);
    } while(strcmp(client->name, "\0") == 0);

    client->available = true;
    client->next = NULL;
    client->partner = NULL;

    if ((client->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        ERROR_HANDLE();

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);

    if (connect(client->socket, 
                (struct sockaddr *) &servaddr, 
                sizeof(servaddr)) < 0)
        ERROR_HANDLE();

    // Sending client info to server
    client_send_info(client);
    connected = true;

    return client;
}

void client_send_info(Client *client)
{
    char recvline[MAXLINE];
    char new_name[MAXWORD];
    char name_taken[MAXWORD];
    bool username_is_changed = false;

    client_sendline(client, client->name, MAXWORD);

    client_recvline(client, recvline, MAXLINE);
    
    strcpy(name_taken, client->name);

    // While username is taken
    while (strcmp(recvline, msg_to_cstr(CLIENT_USERNAME_TAKEN)) == 0)
    {
        printf("[!] That name is taken\n");
        username_is_changed = true;
        do {
            cstring_input("[?] New Name: ", new_name);
        } while (strcmp(new_name, name_taken) == 0 || strcmp(new_name, "\0") == 0);

        client_sendline(client, new_name, MAXWORD);

        client_recvline(client, recvline, MAXLINE);

        if (strcmp(recvline,msg_to_cstr(CLIENT_USERNAME_TAKEN)) == 0)
        {
            strcpy(name_taken, new_name);
        }
    }
    if (username_is_changed) { strcpy(client->name, new_name); }

}

/*
void client_choose_partner(Client *client)
{
    char sendline[MAXWORD+1];
    printf("[?] Who do u want to talk with: ");

    pthread_mutex_lock(&pause_lock);
    pause_thread = false;
    pthread_cond_signal(&pause_cond);
    pthread_mutex_unlock(&pause_lock);
}
*/

void print_active_users(char buffer[])
{
    printf("\n-----------------\n");
    printf("  Active Users\n");
    printf("-----------------\n");
    printf("%s", buffer);
    printf("\n-----------------\n");
}

void client_sendline(Client *client, char buffer[], int limit)
{
    if (send(client->socket, buffer, limit, 0) < 0)
        ERROR_HANDLE();

}

void client_recvline(Client *client, char buffer[], int limit)
{
    memset(buffer, 0, limit);
    switch(recv(client->socket, buffer, limit, 0))
    {
        case 0:
            pthread_mutex_lock(&connec_lock);
            connected = false;
            pthread_mutex_unlock(&connec_lock);
            printf("[!] Connection closed by server\n");
            exit(-1);
        case -1:
            ERROR_HANDLE();
    }
}

void client_send_message(Client *client, MSG_TYPE request)
{
    client_sendline(client, msg_to_cstr(request), MAXMSG);
}
