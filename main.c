#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <time.h>
#define PORT 8081
#define API_KEY "0e0a4e6fdf5242ba4aaa74b67c947bdb" // wstaw swój klucz z OpenWeatherMap

struct string {
    char *ptr;
    size_t len;
};

void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(1);
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    return size * nmemb;
}

void send_response(int client_socket, const char *response) {
    char header[1024];
    sprintf(header,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n",
            strlen(response));
    send(client_socket, header, strlen(header), 0);
    send(client_socket, response, strlen(response), 0);
}

void handle_request(int client_socket, const char *request) {
    if (strstr(request, "GET / ") != NULL) {
        const char *form =
            "<html><body>"
            "<h1>Wybierz miasto</h1>"
            "<form method='GET' action='/pogoda'>"
            "<select name='miasto'>"
            "<option value='Warszawa'>Warszawa</option>"
            "<option value='Krakow'>Kraków</option>"
            "<option value='Berlin'>Berlin</option>"
            "</select>"
            "<input type='submit' value='Pokaż pogodę'/>"
            "</form>"
            "</body></html>";
        send_response(client_socket, form);
    } else if (strstr(request, "GET /pogoda") != NULL) {
        char city[50] = "Warszawa";
        char *start = strstr(request, "miasto=");
        if (start) {
            sscanf(start, "miasto=%49[^& ]", city);
        }

        char url[256];
        snprintf(url, sizeof(url),
                 "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric&lang=pl",
                 city, API_KEY);

        CURL *curl = curl_easy_init();
        struct string json;
        init_string(&json);

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        
        time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    // Pobranie aktualnego czasu systemowego
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Formatowanie daty i godziny
    strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", timeinfo);

        char response[4096];
        snprintf(response, sizeof(response),
                 "<html><body><h2>Pogoda dla: %s</h2><pre>%s</pre>"
                 "<div>Autor: Wiktor Godlewski</div>"
                 "<div>Port: %d</div>"
                 "<div>Data: %s</div></body></html>",
                 city, json.ptr, PORT, buffer);

        send_response(client_socket, response);
        free(json.ptr);
    } else {
        send_response(client_socket, "<html><body>404 Not Found</body></html>");
    }
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[4096];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 3);
    printf("Serwer działa na porcie %d...\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        read(client_socket, buffer, 4096);
        handle_request(client_socket, buffer);
        close(client_socket);
    }

    return 0;
}
