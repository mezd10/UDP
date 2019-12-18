#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <list>
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include "requsts.cpp"
#include <map>

#define LENGTH 500
#define LENGTH_SEND 2000
#define LENGTH_DATA 12
#define NEW 1
#define REPEAT 2
#define OLD 3
//using namespace std;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
std::list<std::string> threadsID;
std::list<std::thread> threads;
int sockfd;

std::string locked_str;

struct data_client {
    int last_code;
    int state;
    std::string last_result;
    std::string name;
    std::string last_name;
    std::string role;
};
std::map<int, data_client> map_data;

void send_data(const std::string &text, sockaddr_in address) {
    std::string result_for_send;
    char buffer[LENGTH_SEND];
    bzero(buffer, LENGTH_SEND);
    std::string num_char;
    int port;
    port = ntohs(address.sin_port);
    auto num = std::to_string(map_data[port].last_code);

    int size = num.length();
    int i = 0;
    num_char.append(num);
    for (i = size; i < LENGTH_DATA; i++) {
        num_char.append(" ");
    }
    result_for_send.append(num_char);
    result_for_send.append(text);
//    if (map_data[port].last_code != 2)
    if (sendto(sockfd, (const char *) result_for_send.c_str(), strlen(result_for_send.c_str()),
               0, (const struct sockaddr *) &address,
               sizeof(address)) < 0) {
        perror("send error");
    }
//    printf("send - %s\n", result_for_send.c_str());
//    number++;
}

void update_state(int number, unsigned int port) {
    auto map_value = map_data.find(port);
    if (map_value != map_data.end()) {
        data_client value = map_data[port];
        if (value.last_code + 1 == number) {
            value.last_code = number;
            value.state = NEW;
        } else if (value.last_code == number) {
            value.state = REPEAT;
        } else {
            value.state = OLD;
        }
        map_data[port] = value;
    } else {
        data_client dataClient = {};
        dataClient.last_code = number;
        dataClient.state = NEW;
        map_data[port] = dataClient;
    }

}

std::string read_data(sockaddr_in* address) {
    std::string answer;
    char buffer[LENGTH];
    bzero(buffer, LENGTH);
    int i, n, len, number;
    sleep(1);

    //SG_WAITALL
    n = recvfrom(sockfd, buffer, LENGTH,
                 MSG_WAITALL, (struct sockaddr*) address, reinterpret_cast<socklen_t *>(&len));
    buffer[n] = '\0';

    char num_recv[LENGTH_DATA];
    bzero(num_recv, LENGTH_DATA);
    std::string text_recv;

    for (i = 0; i < LENGTH_DATA; i++) {
        num_recv[i] = buffer[i];
    }
    unsigned int port;
    //address->sin_port;
    port = ntohs(address->sin_port);
    number = atoi(num_recv);
    update_state(number, port);

    for (i = LENGTH_DATA; i < strlen(buffer); i++) {
        text_recv += buffer[i];
    }
    answer = text_recv;
    return answer;
}

std::string toStr(std::thread::id id) {
    std::stringstream ss;
    ss << id;
    return ss.str();
}

std::string listToString() {
    std::string listThreads = "list threadsID: ";
    for (const std::string &v : threadsID) {
        listThreads += v;
        listThreads += " ";
    }
    listThreads += "\n";
    for (std::thread &v : threads) {
        listThreads += toStr(v.get_id());
        listThreads += " ";
    }
    listThreads += "\n";

    return listThreads;
}

std::list<std::string> split(const std::string &s, char delimiter) {
    std::list<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void exit() {
    shutdown(sockfd, 2);
    close(sockfd);
}

bool containsThread(const std::string &id) {
    for (const std::string &elem : threadsID) {
        if (strcmp(elem.c_str(), id.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

void kill(const std::string &id, std::string &pString) {
    if (containsThread(id)) {
        threadsID.remove(id);
        pString = "killed ";
        pString += id;
    } else {
        pString = "Can not kill this";
    }
    pString += "\n";
}

void commandLine() {
    while (true) {
        std::string message;
        getline(std::cin, message);
        std::list<std::string> listSplit = split(message, ' ');
        if (strcmp(message.c_str(), std::string("list").c_str()) == 0) {
            std::cout << listToString();
        } else if (strcmp(message.c_str(), std::string("exit").c_str()) == 0) {
            std::cout << "Exit";
            exit();
            break;
        } else {
            listSplit = split(message, ' ');
            if (strcmp(listSplit.front().c_str(), std::string("kill").c_str()) == 0) {
                listSplit.pop_front();
                kill(listSplit.front(), message);
                std::cout << message;
            }
        }
    }
}

std::string listCommand() {
    std::string message;
    message += "exit - выход\n";
    message += "signUp (Ваше имя) (Ваше Фамилия) XXXX (роль) [PROGRAMMER, TESTER] - создать нового пользователя\n";
    message += "signIn (Ваше имя) (Ваше фамилия) XXXX - вход\n";
    message += "listMy - список ваших задач для выполнения\n";
    message += "listAll - список всех задач\n";
    message += "listOpen - список всех открытых задач\n";
    message += "update (ID задачи) NewState(состояние) [OPEN, CLOSE, DOING] - обновить состояние проекта\n";
    message += "insert IDResponsible(ID на кого данная задача) (название проекта) (описание) - добавить новый проекта\n";
    return message;
}

void client_handler(sockaddr_in *address) {
    std::string buf = locked_str;
    printf("req - %s\n", buf.c_str());
    pthread_mutex_unlock(&mutex);

    int port;
    port = ntohs(address->sin_port);

    data_client value = map_data[port];





    std::string name = value.name;
    std::string last_name = value.last_name;
    std::string position = value.role;
    std::string message;
    if (value.state == OLD) {
        pthread_mutex_lock(&mutex2);
        threadsID.remove(toStr(std::this_thread::get_id()));
        pthread_mutex_unlock(&mutex2);
        return;
    }
    if (value.state == REPEAT) {
        message = value.last_result;
    } else {
        //std::cout << buf << "\n";
        if (strcmp(buf.c_str(), std::string("help").c_str()) == 0) {
            message = listCommand();
        } else if (strcmp(buf.c_str(), std::string("exit").c_str()) == 0) {
            message = "Good bye";
        } else {
            std::list<std::string> listSplit = split(buf, ' ');
            std::string com = listSplit.front();

            if (strcmp(com.c_str(), std::string("signUp").c_str()) == 0) {
                if (listSplit.size() < 4) {
                    message = listCommand();
                } else {
                    listSplit.pop_front();
                    name = listSplit.front();
                    listSplit.pop_front();
                    last_name = listSplit.front();
                    listSplit.pop_front();
                    position = listSplit.front();
                    message = insertEmploy(name, last_name, position);
                }
            } else if (strcmp(com.c_str(), std::string("signIn").c_str()) == 0) {
                if (listSplit.size() < 3) {
                    message = listCommand();
                } else {
                    listSplit.pop_front();
                    name = listSplit.front();
                    value.name = name;
                    listSplit.pop_front();
                    last_name = listSplit.front();
                    value.last_name = last_name;
                    position = selectRoleByName(name, last_name);
                    value.role = position;
                    if (position.length() > 0) {
                        message = "Hello ";
                        message += name;
                        message += " you are ";
                        message += position;
                    } else {
                        message = "Sorry, but user with name ";
                        message += name;
                        message += " could'n find...";
                    }
                }
            } else if (position.length() == 0) {
                message = "You are not signIn";
            } else if (strcmp(buf.c_str(), std::string("listMy").c_str()) == 0) {
                if (strcmp(position.c_str(), std::string("TESTER").c_str()) == 0) {
                    int i = 1;
                    message = selectMyTasks(name, last_name,  i);
                } else {
                    int i = 0;
                    message = selectMyTasks(name, last_name, i);
                }
            } else if (strcmp(buf.c_str(), std::string("listAll").c_str()) == 0) {
                message = selectAllTasks();
            } else if (strcmp(buf.c_str(), std::string("listOpen").c_str()) == 0) {
                message = selectOpenTasks();
            } else if (strcmp(com.c_str(), std::string("update").c_str()) == 0) {
                if (listSplit.size() < 2) {
                    message = listCommand();
                } else {
                    listSplit.pop_front();
                    std::string id = listSplit.front();
                    listSplit.pop_front();
                    std::string status = listSplit.front();
                    if (strcmp(position.c_str(), std::string("TESTER").c_str()) == 0) {
                        message = updateTasks(id, status);
                    } else if (strcmp(position.c_str(), std::string("PROGRAMMER").c_str()) == 0) {
                        if (listSplit.front() == "CLOSE") {
                            message = "Sorry, but we can not do it, because TESTER can close only";
                        } else {
                            message = updateTasks(id, status);
                        }
                    } else {
                        message = "Sorry, but we can not do it";
                    }
                }
            } else if (strcmp(listSplit.front().c_str(), std::string("insert").c_str()) == 0) {
                sleep(10);
                listSplit.pop_front();

                if (listSplit.size() < 2) {
                    message = listCommand();
                } else {
                    std::string id_res = listSplit.front();
                    listSplit.pop_front();
                    std::string name_pro = listSplit.front();
                    listSplit.pop_front();
                    std::string comment = listSplit.front();
                    message = insertTasks(name, last_name, id_res, name_pro, "OPEN", comment);

                }
            } else {
                message = listCommand();
            }
        }
    }
    value.last_result = message;
    map_data[port] = value;
    //send_data(message,address);
    if (strcmp(buf.c_str(), "exit") == 0) {
        map_data.erase(port);
    }
    pthread_mutex_lock(&mutex2);
    threadsID.remove(toStr(std::this_thread::get_id()));
    pthread_mutex_unlock(&mutex2);
}

int main(int argc, char *argv[]) {
  //  connect();

    struct sockaddr_in servaddr{}, cliaddr{};

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    char input_buf[100];
    printf("Please, write IP address:\n"); fflush(stdout);
    memset(input_buf, 0, sizeof(input_buf));
    fgets(input_buf, sizeof(input_buf), stdin);
    input_buf[strlen(input_buf) - 1] = '\0';
    servaddr.sin_addr.s_addr = inet_addr(input_buf);

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
   // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Please, write port:\n"); fflush(stdout);
    memset(input_buf, 0, sizeof(input_buf));
    fgets(input_buf, sizeof(input_buf), stdin);
    input_buf[strlen(input_buf) - 1] = '\0';
    servaddr.sin_port = htons(atoi(input_buf));

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *) &servaddr,
             sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    std::thread cl(commandLine);

    while (true) {
        std::string request = read_data(&cliaddr);
        if (request.length() == 0) { break; }

        pthread_mutex_lock(&mutex);
        locked_str = request;

        threads.emplace_back([&] { client_handler(&cliaddr); });
        pthread_mutex_lock(&mutex2);
        threadsID.emplace_back(toStr(threads.back().get_id()));
        pthread_mutex_unlock(&mutex2);
    }

    cl.join();

    for (std::thread &thread : threads) {
        if (containsThread(toStr(thread.get_id()))) {
            thread.join();
        }
    }

    PQfinish(conn);
    printf("\nServer exit\n");
    return 0;

}
