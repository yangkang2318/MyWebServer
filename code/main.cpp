#include "server/server.h"

int main(int argc, char const *argv[]) {
    Server server(1316, 3, 60000, false, 3306, "root", "root", "server", 12, 6, true, 1, 1024);
    server.Start();
    return 0;
}
