#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <arpa/inet.h>

#define LENGTH 500
#define LENGTH_RECV 2000
#define LENGTH_DATA 12

using namespace std;

string message;

string sign_in_up() {
    string result;
    string currentAnswer;
    cout << "Вы зарегестрированы? y/n\n";
    getline(cin, currentAnswer);
    if (strcmp(currentAnswer.c_str(), "y") == 0) {
        result += "signIn ";
        cout << "Введите логин\n";
        getline(cin, currentAnswer);
        result += currentAnswer;
    } else {
        result += "signUp ";
        cout << "Введите логин для новой учетной записи\n";
        getline(cin, currentAnswer);
        result += currentAnswer;
        result += " ";

        cout << "Вы являетесь фрилансером? y/n\n";
        getline(cin, currentAnswer);
        if (strcmp(currentAnswer.c_str(), "y") == 0) {
            result += "CLIENT";
        } else {
            result += "PERFORMER";
        }
    }
    return result;
}

string update() {
    string result;
    string currentAnswer;
    cout << "Вы заказчик? y/n\n";
    getline(cin, currentAnswer);
    if (strcmp(currentAnswer.c_str(), "y") == 0) {
        result += "update";
        cout << "Введите id редактируемой задачи\n";
        getline(cin, currentAnswer);
        result += " ";
        result += currentAnswer;

        cout << "Введите новое состояние редактируемой задачи [OPEN, CLOSE, DOING]\n";
        getline(cin, currentAnswer);
        result += " ";
        result += currentAnswer;
    } else {
        result += "update ";
        cout << "Введите id задачи которую хотите выполнить\n";
        getline(cin, currentAnswer);
        result += currentAnswer;
    }
    return result;
}

string insert() {
    string result;
    string currentAnswer;
    cout << "Вы заказчик? y/n\n";
    getline(cin, currentAnswer);
    if (strcmp(currentAnswer.c_str(), "y") == 0) {
        result += "insert";
        cout << "Введите название новой задачи\n";
        getline(cin, currentAnswer);
        result += " \"";
        result += currentAnswer;
        result += "\"";

        cout << "Введите описание новой задачи\n";
        getline(cin, currentAnswer);
        result += " \"";
        result += currentAnswer;
        result += "\"";

        cout << "Введите стоимость новой задачи\n";
        getline(cin, currentAnswer);
        result += " ";
        result += currentAnswer;
    } else {
        cout << "Простите, но вы не можете выкладывать задания\n";
    }
    return result;
}

string send_data(const string &text, sockaddr_in servaddr, int sockfd, int &number) {
    string result_for_send;
    string answer;
    bool is_sended = false;
    int current_step = 0;
    if (text.length()==0){
        printf("Введите текст\n");
        return "Введите текст\n";
    }
    while (!is_sended && current_step < 4) {
        answer.clear();
        result_for_send.clear();
        char buffer[LENGTH_RECV];
        bzero(buffer, LENGTH_RECV);
        string num_char;
        auto num = to_string(number);
        int size = num.length();
        int i = 0;
        num_char.append(num);
        for (i = size; i < LENGTH_DATA; i++) {
            num_char.append(" ");
        }
        result_for_send.append(num_char);
        result_for_send.append(text);
        //MSG_CONFIRM
        if (sendto(sockfd, (const char *) result_for_send.c_str(), strlen(result_for_send.c_str()),
                   0, (const struct sockaddr *) &servaddr,
                   sizeof(servaddr)) < 0) {
            break;
        }
        int n, len;
        n = recvfrom(sockfd, (char *) buffer, LENGTH_RECV,
                     0, (sockaddr *) &servaddr, reinterpret_cast<socklen_t *>(&len));
        buffer[n] = '\0';
        if (n == 0) continue;
        char num_recv[LENGTH_DATA];
        bzero(num_recv, LENGTH_DATA);
        string text_recv;

        for (i = 0; i < LENGTH_DATA; i++) {
            num_recv[i] = buffer[i];
        }

        for (i = LENGTH_DATA; i < strlen(buffer); i++) {
            text_recv += buffer[i];
        }
//        printf("num - %s\n", num_char.c_str());
        if (atoi(num_recv) == atoi(num_char.c_str())) {
            answer = text_recv;
            is_sended = true;
        }
        current_step++;
    }
    if (!is_sended) {
        answer = "Ошибка сети\n";
    } else {
        number++;
    }
    printf("%s\n", answer.c_str());
    return answer;
}


void Event(int port, int adr) {
    int sock;
    struct sockaddr_in addr{};

    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));

    // Filling server information
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = adr;
    int number = 0;
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); //без этого вызова висим вечно

    bool isWork = true;
    send_data("Hi", addr, sock, number);
    send_data(sign_in_up(), addr, sock, number);
    while (isWork) {
        std::getline(std::cin, message);
        if (strcmp(message.c_str(), std::string("update").c_str()) == 0) {
            message = update();
        }
        if (strcmp(message.c_str(), std::string("insert").c_str()) == 0) {
            message = insert();
        }

        send_data(message, addr, sock, number);
        if (strcmp(message.c_str(), std::string("exit").c_str()) == 0) {
            isWork = false;
        }
    }
    shutdown(sock, 2);
    close(sock);
}

int main(int argc, char *argv[]) {
    char input_buf[1000];
    printf("Please, write port:\n"); fflush(stdout);
    memset(input_buf, 0, sizeof(input_buf));
    fgets(input_buf, sizeof(input_buf), stdin);
    input_buf[strlen(input_buf) - 1] = '\0';

    int port = atoi(input_buf);

    printf("Please, write port:\n"); fflush(stdout);
    memset(input_buf, 0, sizeof(input_buf));
    fgets(input_buf, sizeof(input_buf), stdin);
    input_buf[strlen(input_buf) - 1] = '\0';

    int addr = inet_addr(input_buf);

    Event(port, addr);
    return 0;
}


