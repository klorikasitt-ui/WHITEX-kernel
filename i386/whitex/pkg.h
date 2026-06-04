static void wx_pkg_listen(void) {
    /* Basit bir soket aciyoruz */
    int sockfd = wx_socket(2, 1, 6);
    if (sockfd < 0) {
        print("Soket acilamadi\n");
        return;
    }

    /* TCP state'i dinleme moduna al */
    tcp_sockets[sockfd].state = 2; 
    print("Paketler bekleniyor...\n");
}

static void wx_pkg_cli_handler(char *arg) {
    /* Sadece "start" komutuyla dinlemeyi baslat */
    if (arg[0] == 's') {
        wx_pkg_listen();
    } else {
        print("Komut: start\n");
    }
}

static void wx_pkg_system_bootstrap(void) {}
