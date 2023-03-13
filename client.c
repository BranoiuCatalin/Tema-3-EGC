#include <arpa/inet.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */

#include "helpers.h"
#include "parson.h"
#include "requests.h"

int main(int argc, char *argv[]) {
  char *p, *message, *response;
  char token[1024];
  char **cookie = malloc(sizeof(char *));
  int sockfd;

  char command[50], arg1[50], arg2[50], arg3[50], arg4[50];
  int arg5 = 0;

  // deschid conexiunea
  sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

  while (1) {
    // mentin conexiuneaa deschisa
    close_connection(sockfd);
    sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

    scanf("%s", command);
    // identific comanda
    if (strcmp(command, "register") == 0) {
      printf("username=");
      scanf("%s", arg1);
      printf("password=");
      scanf("%s", arg2);

      // formez payloadul json pentru a il trimite
      JSON_Value *root_value = json_value_init_object();
      JSON_Object *root_object = json_value_get_object(root_value);
      char *serialized_string = NULL;
      json_object_set_string(root_object, "username", arg1);
      json_object_set_string(root_object, "password", arg2);
      serialized_string = json_serialize_to_string_pretty(root_value);

      // formez mesajul
      message = compute_post_request(
          "34.241.4.235:8080", "/api/v1/tema/auth/register", "application/json",
          serialized_string, NULL, 0, NULL);

      // trimit mesajul
      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // verific raspunsul serverului pentru rezultatul operatiunii
      if (response[9] == '4' || response[9] == '5') {
        printf("User already exists\n");
      } else {
        printf("User successfully created\n");
      }
    }

    if (strcmp(command, "login") == 0) {
      printf("username=");
      scanf("%s", arg1);
      printf("password=");
      scanf("%s", arg2);

      // formez payloadul json pentru a il trimite
      JSON_Value *root_value = json_value_init_object();
      JSON_Object *root_object = json_value_get_object(root_value);
      char *serialized_string = NULL;
      json_object_set_string(root_object, "username", arg1);
      json_object_set_string(root_object, "password", arg2);
      serialized_string = json_serialize_to_string_pretty(root_value);

      // formez mesajul
      message = compute_post_request(
          "34.241.4.235:8080", "/api/v1/tema/auth/login", "application/json",
          serialized_string, NULL, 0, NULL);

      // trimit mesajul
      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // obtin cookieul din raspunsul serverului
      cookie[0] = malloc(sizeof(char) * 100);
      char *p = strtok(response, " ;\n");
      while (p) {
        if (strcmp(p, "Set-Cookie:") == 0) {
          p = strtok(NULL, " ;\n");
          strcpy(cookie[0], p);
        }
        if (p) p = strtok(NULL, " \n");
      }

      // verific raspunsul serverului pentru rezultatul operatiunii
      if (response[9] == '4' || response[9] == '5') {
        printf("Username/password is not valid. Please try again.\n");
      } else {
        printf("Successfully logged in\n");
      }
    }

    if (strcmp(command, "enter_library") == 0) {
      // formez mesajul
      message = compute_get_request("34.241.4.235:8080",
                                    "/api/v1/tema/library/access", NULL, cookie,
                                    1, NULL);
      // trimit mesajul
      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // verific raspunsul serverului pentru rezultatul operatiunii
      if (response[9] == '4' || response[9] == '5') {
        printf("You are not logged in!\n");
      } else {
        printf("Successfully entered library\n");

        //obtin tokenul
        strcpy(token, basic_extract_json_response(response));
        char *p;
        p = strtok(token, "\":{}");
        p = strtok(NULL, "\":{}");
        strcpy(token, p);
      }
    }

    if (strcmp(command, "get_books") == 0) {
      // formez mesajul
      message =
          compute_get_request("34.241.4.235:8080", "/api/v1/tema/library/books",
                              NULL, NULL, 1, token);

      // trimit mesajul
      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // verific raspunsul serverului pentru rezultatul operatiunii
      if (response[9] == '4' || response[9] == '5') {
        printf("You don't have access to the library!\n");
      } else {
        p = strtok(response, "[]");
        p = strtok(NULL, "[]");
        strcpy(response, p);

        // afisez cartile din biblioteca
        printf("Books in library:\n");
        for (int i = 0; i < strlen(response); i++) {
          printf("%c", response[i]);
          if (response[i] == '}') {
            printf("\n");
            i++;
          }
        }
      }
    }

    if (strcmp(command, "get_book") == 0) {
      printf("id=");
      scanf("%s", arg1);
      char *url = malloc(256);

      // formez mesajul
      strcpy(url, "/api/v1/tema/library/books/");
      strcat(url, arg1);
      message =
          compute_get_request("34.241.4.235:8080", url, NULL, cookie, 1, token);

      // trimit mesajul
      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // verific raspunsul serverului pentru rezultatul operatiunii
      if (response[9] == '4' && response[11] == '4') {
        printf("This book doesn't exist!\n");
      } else if (response[9] == '5' || response[9] == '4') {
        printf("You don't have access to the library!\n");
      } else {
        p = strtok(response, "[]");
        p = strtok(NULL, "[]");
        strcpy(response, p);

        // afisez detaliile cartii
        printf("Book details:\n");
        for (int i = 0; i < strlen(response); i++) {
          printf("%c", response[i]);
          if (response[i] == '}' || response[i] == ',') {
            printf("\n");
            i++;
          }
        }
      }
    }

    if (strcmp(command, "add_book") == 0) {
      printf("title=");
      getchar();
      read_with_space(arg1);

      printf("author=");
      read_with_space(arg2);

      printf("genre=");
      read_with_space(arg3);

      printf("publisher=");
      read_with_space(arg4);

      printf("page_count=");
      scanf("%d", &arg5);

      // formez informatiile cartii in format JSON
      JSON_Value *root_value = json_value_init_object();
      JSON_Object *root_object = json_value_get_object(root_value);
      char *serialized_string = NULL;
      json_object_set_string(root_object, "title", arg1);
      json_object_set_string(root_object, "author", arg2);
      json_object_set_string(root_object, "genre", arg3);
      json_object_set_string(root_object, "publisher", arg4);
      json_object_set_number(root_object, "page_count", arg5);
      serialized_string = json_serialize_to_string_pretty(root_value);

      // formez mesajul
      message = compute_post_request(
          "34.241.4.235:8080", "/api/v1/tema/library/books", "application/json",
          serialized_string, cookie, 1, token);

      // trimit mesajul
      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // verific raspunsul serverului pentru rezultatul operatiunii
      if (response[9] == '5') {
        printf("You don't have access to the library!\n");
      } else {
        printf("Book successfully added!\n");
      }
    }

    if (strcmp(command, "delete_book") == 0) {
      printf("id=");
      scanf("%s", arg1);
      char *url = malloc(256);

      // formez mesajul
      strcpy(url, "/api/v1/tema/library/books/");
      strcat(url, arg1);
      message = compute_delete_request("34.241.4.235:8080", url, NULL, cookie,
                                       1, token);

      // trimit mesajul
      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // verific raspunsul serverului pentru rezultatul operatiunii
      if (response[9] == '4' && response[11] == '4') {
        printf("Book not found!\n");
      } else if (response[9] == '5') {
        printf("You don't have access to the library!\n");
      } else {
        printf("The book was deleted!\n");
      }
    }

    if (strcmp(command, "logout") == 0) {
      // formez mesajul
      message =
          compute_get_request("34.241.4.235:8080", "/api/v1/tema/auth/logout",
                              NULL, cookie, 1, NULL);

      // trimit measjul
      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // verific raspunsul serverului pentru rezultatul operatiunii
      if (response[9] == '4' || response[9] == '5') {
        printf("Already logged out\n");
      } else {
        // sterg tokenul si cookieul pentru a nu permite accesarea bibliotecii
        // dupa logout
        memset(token, 0, sizeof(token));
        memset(cookie[0], 0, sizeof(cookie[0]));
        printf("Successfully logged out\n");
      }
    }

    if (strcmp(command, "exit") == 0) {
      printf("Exiting client...\n");
      break;
    }
  }

  // inchidem conexiunea
  close_connection(sockfd);

  return 0;
}
