#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "friends.h"

#define MAX_BACKLOG 5
#define BUF_SIZE 128
#define DELIMITER " " // only delimiter for this program
#define WELCOME_MSG "What is your name?\r\n"

#ifndef PORT
  #define PORT 57509
#endif


typedef struct client {
    int sock_fd;
    char *username;
    User *user;
    struct client *next;
    
    char buf[BUF_SIZE + 1];
    int inbuf;           // How many bytes currently in buffer?
    int room;           // How many bytes remaining in buffer?
    char *after;       // Pointer to position after the data in buf
} Client;


/* 
 * Write a formatted error message to client.
 */
void error(char *msg, Client *client) {
    int msg_size = strlen("Error: ") + strlen(msg) + strlen("\n\r\n");
    char error_msg[msg_size];
    snprintf(error_msg, msg_size, "Error: %s\n\r\n", msg);
    write(client->sock_fd, error_msg, msg_size);
}

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found. The return value is the index into buf
 * where the current line ends.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n) {
	int index = 0;
	while (index < n - 1) { // first n chars means index can be at most n - 1 but we do  + 1 so strictly less
		if (buf[index] == '\r' && buf[index + 1] == '\n') {
			return index + 2;
		}
		index += 1;
	}
    return -1;
}

/* Initialises a mostly empty client. The only argument is client_fd. Use -1, if we want a 'null' client */
Client *init_client(int client_fd) {
    Client *new_client = Malloc(sizeof(Client));
    new_client->sock_fd = client_fd;
    new_client->username = NULL;
    new_client->next = NULL;
    new_client->user = NULL;

    memset(new_client->buf, '\0', BUF_SIZE + 1);
    new_client->inbuf = 0;
    new_client->room = BUF_SIZE;
    new_client->after = new_client->buf;
    return new_client;
}

/* Returns pointer to client with the given username. If more than one such client available returns the first instance. Returns NULL if first_client is NULL or if username not found. */
Client *find_client(char *username, Client *first_client) {
    if (first_client == NULL) {
        return NULL;
    }

    while (first_client != NULL){
        if (strcmp(username, first_client->username) == 0) {
            return first_client;
        }
        first_client = first_client->next;
    }
    return NULL;
}

/*
 * Accept a connection. Note that a new file descriptor is created for
 * communication with the client. The initial socket descriptor is used
 * to accept connections, but the new socket is used to communicate.
 * Return the new client's file descriptor or -1 on error.
 */
int accept_connection(int fd, Client **client_list) {
    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    // initialise new client on the heap
    Client *new_client = init_client(client_fd);

    // place new client at end of linked list
    if (*client_list == NULL) {
        *client_list = new_client;
    }
    else {
        Client *curr_client = *client_list;
        while (curr_client->next != NULL) {
            curr_client = curr_client->next;
        }
        curr_client->next = new_client;
    }
    return client_fd;

}

// Want to tokenise the user input so we can pass it to process_args

/* Find number of tokens in a line */
int find_num_args(char *user_input){
    char copy_line[strlen(user_input) + 1];
    strncpy(copy_line, user_input, strlen(user_input) + 1);
    copy_line[strlen(user_input)] = '\0';

    int count = 0;
    char *token = strtok(copy_line, DELIMITER);
    while (token != NULL){
        count += 1;
        token = strtok(NULL, DELIMITER);
    }
    return count;
}

// Convert user_input into array of inputs as expected by process_args. 
// IMPORTANT: Assumes user_input is null-terminated.
char **create_args_array(char *user_input, int num_inputs) {
    // create copy of user_input so we can modify this instead
    char input_cpy[strlen(user_input) + 1];
    strncpy(input_cpy, user_input, strlen(user_input));
    input_cpy[strlen(user_input)] = '\0';

    char **argv = Malloc(sizeof(char *) * num_inputs);
    char *token = strtok(input_cpy, DELIMITER);
    int i = 0;
    while (token != NULL){
        char *heap_token = Malloc(sizeof(char) * (strlen(token) + 1));
        argv[i] = heap_token;
        strncpy(heap_token, token, strlen(token));
        heap_token[strlen(token)] = '\0';    
        i++;
        token = strtok(NULL, DELIMITER);
    }
    return argv;
}

/* Send message to every instance of client in client list with username. */
void notify_client(char *username, char *message, Client *first_client) {
    if (first_client == NULL) {
        return;
    }
    while (first_client != NULL) {
        if ((first_client->sock_fd != -1) && strcmp(first_client->username, username) == 0) {
            write(first_client->sock_fd, message, strlen(message));
        }
        first_client = first_client->next;
    }
}

/* Removes client from list of clients pointed to by client_list. Does nothing if either argument is NULL.*/
void remove_client(Client* client, Client **client_list) {
    if (client_list == NULL || client == NULL) {
        return;
    } else if ((*client_list)->sock_fd == client->sock_fd) {
        *client_list = (*client_list)->next; // replace head of linked list
    } else {
        Client *prev_client, *curr_client;
        prev_client = *client_list;
        curr_client = prev_client;
        while (curr_client != NULL && curr_client->sock_fd != client->sock_fd) {
            prev_client = curr_client;
            curr_client = curr_client->next;
        }

        if (curr_client != *client_list) {
            // This means we found client in the linked list and it is set to curr_client
            prev_client->next = curr_client->next;
        }
    }
    close(client->sock_fd);
    free(client->username);
    free(client);
    return;
}



/* Processes the arguments from the user and calls the appropriate functions from friends.c. Returns -1 if client quit. */
int process_args(int cmd_argc, char **cmd_argv, Client *first_client, Client *client, User **users) {
    User *user_list = *users;
    char *buf;
    if (cmd_argc <= 0) {
        return 0;
    } else if (strcmp(cmd_argv[0], "quit") == 0 && cmd_argc == 1) {
        return -1;
    } else if (strcmp(cmd_argv[0], "list_users") == 0 && cmd_argc == 1) {
        buf = list_users(user_list);
        write(client->sock_fd, buf, strlen(buf));
        free(buf);

    } else if (strcmp(cmd_argv[0], "make_friends") == 0 && cmd_argc == 2) {
        int notif_size = strlen("You have been friended by \n\r\n") + strlen(client->username); // +1 for \n and +2 for \r\n
        char friend_message[notif_size + 1]; // +1 because snprintf always null terminates strings

        int client_message_size = strlen("You are now friends with \n\r\n") + strlen(cmd_argv[1]);
        char client_message[client_message_size + 1];
        switch (make_friends(cmd_argv[1], client->username, user_list)) {
            case 0:
                snprintf(friend_message, notif_size, "You have been friended by %s\n\r\n", client->username);
                notify_client(cmd_argv[1], friend_message, first_client);
                snprintf(client_message, client_message_size, "You are now friends with %s\n\r\n", cmd_argv[1]);
                write(client->sock_fd, client_message, client_message_size);
                break;
            case 1:
                error("users are already friends", client);
                break;
            case 2:
                error("at least one user you entered has the max number of friends", client);
                break;
            case 3:
                error("you must enter two different users", client);
                break;
            case 4:
                error("User you entered does not exist", client);
                break;
        }
    } else if (strcmp(cmd_argv[0], "post") == 0 && cmd_argc >= 3) {
        // first determine how long a string we need
        int space_needed = 0;
        for (int i = 2; i < cmd_argc; i++) {
            space_needed += strlen(cmd_argv[i]) + 1;
        }

        // allocate the space
        char *contents = malloc(space_needed);
        if (contents == NULL) {
            perror("malloc");
            exit(1);
        }

        // copy in the bits to make a single string
        strcpy(contents, cmd_argv[2]);
        for (int i = 3; i < cmd_argc; i++) {
            strcat(contents, " ");
            strcat(contents, cmd_argv[i]);
        }

        User *author = client->user;
        User *target = find_user(cmd_argv[1], user_list);

        int friend_message_size = strlen("From : \n\r\n") + strlen(client->username) + space_needed;
        char friend_message[friend_message_size];
        switch (make_post(author, target, contents)) {
            case 0:
                snprintf(friend_message, friend_message_size, "From %s: %s\r\n", client->username, contents);
                notify_client(target->name, friend_message, first_client);
                break;
            case 1:
                error("the users are not friends", client);
                break;
            case 2:
                error("at least one user you entered does not exist", client);
                break;
        }
    } else if (strcmp(cmd_argv[0], "profile") == 0 && cmd_argc == 2) {
        User *user = find_user(cmd_argv[1], user_list);
        buf = print_user(user);
        if (buf == NULL) {
            error("user not found", client);
        } else {
            write(client->sock_fd, buf, strlen(buf));
            free(buf);
        }
    } else {
        error("Incorrect syntax", client);
    }
    return 0;
}

/* Parses user command. Expects a full line with null termination. Returns -1 if client sent quit command and 0 otherwise. */
int parse_input(char *user_input, Client *first_client, Client *client, User **users) {
    // new client, they are sending in a username instead of commands 
    if (client->username == NULL) {
        // decided how long the name is, truncating if necessary
        int username_len = strlen(user_input);
        if (username_len > MAX_NAME - 1) {
            username_len = MAX_NAME - 1;
            write(client->sock_fd, "Username too long. Username truncated to %d characters", MAX_NAME - 1);
        }

        // Allocate appropriate amount space
        char *username = Malloc(sizeof(char) * (username_len + 1)); 
        strncpy(username, user_input, username_len);
        username[username_len] = '\0'; // should be null terminated anyway but just to make sure

        if (create_user(username, users) == 1) {
            // means username is in users list
            write(client->sock_fd, "Welcome back.\r\n", strlen("Welcome back.\r\n"));
        } 
        client->username = username;
        client->user = find_user(username, *users);

        return 0;
    }

    // Run the other
    int num_inputs = find_num_args(user_input);
    char **args = create_args_array(user_input, num_inputs);
    if (process_args(num_inputs, args, first_client, client, users) == -1) {
        return -1;
    }
    return 0;
}


/*
 * Read a message from client_index and add it to the client's buffer.
 * Returns client_fd if the fd has been closed, 0 otherwise.
 */
int read_from(Client *client, Client *first_client, User **users) {
    int client_fd = client->sock_fd;
    int num_read = read(client_fd, client->after, client->room);
    // nothing was read so writing end has been closed
    if (num_read == 0) {
        return client_fd;
    }
    client->inbuf += num_read;

    int where;
    while ((where = find_network_newline(client->buf, client->inbuf)) > 0) {
        (client->buf)[where - 2] = '\0'; // replace network newline with null character
        if (parse_input(client->buf, first_client, client, users) == -1) {
            return client_fd;
        }
        client->inbuf = client->inbuf - where;
        memmove(client->buf, client->buf + where, client->inbuf);
    }

    client->room = BUF_SIZE - client->inbuf;
    client->after = client->buf + client->inbuf;
    return 0;
}


int main() {
    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("server: socket");
        exit(1);
    }

    // Set information about the port (and IP) we want to be connected to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    // This sets an option on the socket so that its port can be reused right
    // away. Since you are likely to run, stop, edit, compile and rerun your
    // server fairly quickly, this will mean you can reuse the same port.
    int on = 1;
    int status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
                            (const char *) &on, sizeof(on));
    if (status == -1) {
        perror("setsockopt -- REUSEADDR");
    }

    // This should always be zero. On some systems, it won't error if you
    // forget, but on others, you'll get mysterious errors. So zero it.
    memset(&server.sin_zero, 0, 8);

    // Bind the selected port to the socket.
    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("server: bind");
        close(sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(sock_fd);
        exit(1);
    }

    int max_fd = sock_fd;
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);

    Client *first_client = NULL; // serves as a constant head to the head of the linked list of clients
    User *user_list = NULL;
    while (1) {
        // select updates the fd_set it receives, so we always use a copy and retain the original.
        fd_set listen_fds = all_fds;
        if (select(max_fd + 1, &listen_fds, NULL, NULL, NULL) == -1) {
            perror("server: select");
            exit(1);
        }

        // Is it the original socket? Create a new connection ...
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd, &first_client);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            write(client_fd, WELCOME_MSG, strlen(WELCOME_MSG));
        }

        Client *curr_client = first_client;
        while (curr_client != NULL) {
            // Check whether or not socket is ready for reading + writing
            if (FD_ISSET(curr_client->sock_fd, &listen_fds)) {
                int client_fd = read_from(curr_client, first_client, &user_list);
                if (client_fd > 0) {
                    remove_client(curr_client, &first_client);
                    FD_CLR(client_fd, &all_fds);
                }
                else {
                    write(curr_client->sock_fd, "Go ahead and type in commands>\r\n", strlen("Go ahead and type in commands>\r\n"));
                }
            }
            curr_client = curr_client->next;
        }
    }

    // Should never get here
    return 1;
}
