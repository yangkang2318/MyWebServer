#include "server/server.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("usage: %s <port> <threadNum> <connPoolNum>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);
    assert(port > 1024);
    int threadNum = atoi(argv[2]);
    assert(threadNum > 0);
    int connPoolNum = atoi(argv[3]);
    assert(connPoolNum > 0);
    Server server(port, 3, 60000, false, 3306, "root", "root", "server", connPoolNum, threadNum, true, 1, 1024);
    server.Start();
    return 0;
}
