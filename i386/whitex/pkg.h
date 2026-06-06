static void wx_pkg_listen(void) {
    int sockfd = wx_socket(2, 1, 6);
    if (sockfd < 0) {
        return;
    }

    tcp_sockets[sockfd].state = 2; 

}

static void wx_pkg_cli_handler(char *arg) {
    if (arg[0] == 's') {
        wx_pkg_listen();
    } else {
        print("pkg start\n");
    }
}

static void wx_pkg_system_bootstrap(void) {}
