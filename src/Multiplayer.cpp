#include "Multiplayer.h"

// Convert a user-entered host string into an IPv4 address.
// Accepts both "127.0.0.1" style IPs and DNS names (via gethostbyname).
static bool resolveHostIPv4(const std::string& host, sockaddr_in& outAddr) {
    outAddr = sockaddr_in{};
    outAddr.sin_family = AF_INET;

    unsigned long ip = inet_addr(host.c_str());
    if (ip != INADDR_NONE) {
        outAddr.sin_addr.s_addr = ip;
        return true;
    }

    hostent* he = gethostbyname(host.c_str());
    if (!he || !he->h_addr_list || !he->h_addr_list[0]) {
        return false;
    }
    outAddr.sin_addr = *reinterpret_cast<in_addr*>(he->h_addr_list[0]);
    return true;
}

void SfmlMultiplayer::closeSock() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

void SfmlMultiplayer::stopHosting() {
    if (hosting) {
        stopServer(server);
        hosting = false;
    }
}

void SfmlMultiplayer::setStatus(const std::string& s) {
    statusLine = s;
    hint.setString(statusLine);
}

void SfmlMultiplayer::applyMenuLayout() {
    if (menuPane == MenuPane::Root) {
        hostBtn.setPosition({500.f, 250.f});
        joinBtn.setPosition({500.f, 330.f});
        backBtn.setPosition({500.f, 410.f});
    } else {
        connectBtn.setPosition({500.f, 410.f});
        backBtn.setPosition({500.f, 490.f});
    }
}

void SfmlMultiplayer::startClient(const std::string& ip, int port, bool host) {
    beginConnect(ip, port, host);
}

void SfmlMultiplayer::beginConnect(const std::string& ip, int port, bool host) {
    // Connection setup is done as a small state machine:
    // - We start a non-blocking connect() to keep the UI responsive.
    // - While the OS is still connecting, `mode == Connecting` and we poll.
    // - When it succeeds/fails, we either start the session or return to the menu.

    // reset/cleanup any previous session
    backToMainMenuRequested = false;
    pauseConfirmActive = false;
    running = false;
    closeSock();

    isHost = host;
    sessionIp = ip;
    sessionPort = port;

    sockaddr_in addr{};
    if (!resolveHostIPv4(ip, addr)) {
        setStatus("Could not resolve host.");
        return;
    }
    addr.sin_port = htons((u_short)port);

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        setStatus("socket() failed.");
        return;
    }

    // Non-blocking connect: avoids freezing the menu on unreachable IPs.
    u_long nb = 1;
    if (ioctlsocket(s, FIONBIO, &nb) != 0) {
        closesocket(s);
        setStatus("ioctlsocket(FIONBIO) failed.");
        return;
    }

    int rc = connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == 0) {
        sock = s;
        startSessionAfterConnect();
        return;
    }

    const int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS || err == WSAEALREADY) {
        sock = s;
        connectStart = clock::now();
        mode = Mode::Connecting;
        cancelConnectBtn.setPosition({500.f, 520.f});
        setStatus("Connecting... (Cancel to abort)");
        return;
    }

    closesocket(s);
    setStatus("Failed to connect.");
}

bool SfmlMultiplayer::pollConnect(bool& outConnected, std::string& outError) {
    outConnected = false;
    outError.clear();

    if (sock == INVALID_SOCKET) {
        outError = "No socket.";
        return true;
    }

    // On Windows, a non-blocking connect completes when the socket becomes
    // writable (or reports an error). We use select() + getsockopt(SO_ERROR)
    // to detect the final result without blocking.
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);

    fd_set efds;
    FD_ZERO(&efds);
    FD_SET(sock, &efds);

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int r = select(0, nullptr, &wfds, &efds, &tv);
    if (r == 0) {
        return false; // still connecting
    }
    if (r == SOCKET_ERROR) {
        outError = "select() failed.";
        return true;
    }

    int soError = 0;
    int soErrorLen = sizeof(soError);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soError), &soErrorLen) == SOCKET_ERROR) {
        outError = "getsockopt(SO_ERROR) failed.";
        return true;
    }

    if (soError != 0) {
        outError = "connect() failed.";
        return true;
    }

    outConnected = true;
    return true;
}

void SfmlMultiplayer::startSessionAfterConnect() {
    running = true;
    started = false;
    initialized = false;
    sentGameOver = false;
    postGameWaiting = false;
    pendingIncomingGarbage = 0;
    opp.hasBoard = false;
    ta.reset(false);
    rxBuffer.clear();

    lastFall = clock::now();
    mode = Mode::Waiting;
    menuPane = MenuPane::Root;
    setStatus("Connected. Waiting for opponent...");
}

void SfmlMultiplayer::processLine(const std::string& line) {
    // The server sends line-based commands. Each command may update the local
    // state machine (Mode) or update remote data (opponent board snapshot).
    if (line == "WAITING") {
        started = false;
        initialized = false;
        sentGameOver = false;
        postGameWaiting = false;
        pendingIncomingGarbage = 0;
        opp.hasBoard = false;
        ta.reset(false);
        mode = Mode::Waiting;
        setStatus("Waiting for opponent...");
        return;
    }
    if (line == "ROLE HOST") { isHost = true; return; }
    if (line == "ROLE GUEST") { isHost = false; return; }

    if (line == "START" || line == "REMATCH_START") {
        started = true;
        postGameWaiting = false;
        sentGameOver = false;

        ta.reset(true);
        initialized = true;
        pendingIncomingGarbage = 0;
        opp.hasBoard = false;

        lastFall = clock::now();

        sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
        mode = Mode::Playing;
        setStatus("Match started!");
        return;
    }

    if (line == "REMATCH_ABORT") {
        setStatus("Opponent declined the rematch.");
        mode = Mode::Menu;
        running = false;
        return;
    }

    if (line.rfind("GARBAGE ", 0) == 0) {
        int n = 0;
        try { n = std::stoi(line.substr(8)); } catch (...) { n = 0; }
        if (n > 0) {
            // Incoming garbage is queued and applied when the current piece locks.
            pendingIncomingGarbage += n;
        }
        return;
    }

    if (line.rfind("BOARD ", 0) == 0) {
        // BOARD <score> <data>
        size_t sp1 = line.find(' ');
        size_t sp2 = line.find(' ', sp1 + 1);
        if (sp2 == std::string::npos) return;
        try {
            opp.score = std::stoi(line.substr(sp1 + 1, sp2 - (sp1 + 1)));
        } catch (...) {
            opp.score = 0;
        }
        std::string data = line.substr(sp2 + 1);
        if ((int)data.size() >= 10 * 22) {
            opp.data = data.substr(0, 10 * 22);
            opp.hasBoard = true;
        }
        return;
    }

    if (line == "YOU_WIN") {
        mode = Mode::PostGame;
        setStatus("YOU WIN");
        return;
    }

    if (line == "OPPONENT_LEFT") {
        setStatus("Opponent left.");
        mode = Mode::Menu;
        running = false;
        return;
    }

    if (line == "REMATCH YES") {
        // host receives it; but server will broadcast REMATCH_START
        return;
    }

    if (line == "LEAVE") {
        running = false;
        return;
    }
}

void SfmlMultiplayer::pumpNetwork() {
    if (sock == INVALID_SOCKET) return;

    // Non-blocking receive loop:
    // - read as much as possible
    // - stop when recv() would block
    // - split by '\n' into protocol lines
    char temp[512];
    while (true) {
        int r = recv(sock, temp, sizeof(temp), 0);
        if (r > 0) {
            rxBuffer.append(temp, temp + r);
            continue;
        }
        if (r == 0) { running = false; return; }

        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) break;
        running = false;
        return;
    }

    size_t pos = 0;
    while (true) {
        size_t nl = rxBuffer.find('\n', pos);
        if (nl == std::string::npos) break;
        std::string line = rxBuffer.substr(pos, nl - pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        processLine(line);
        pos = nl + 1;
    }
    if (pos > 0) rxBuffer.erase(0, pos);
}

SfmlMultiplayer::SfmlMultiplayer(const sf::Font& font, sf::RenderWindow& window)
    : hostBtn(font, {500.f, 250.f}, "Host"),
      joinBtn(font, {500.f, 330.f}, "Join"),
      backBtn(font, {500.f, 410.f}, "Back"),
      connectBtn(font, {500.f, 490.f}, "Connect"),
    cancelConnectBtn(font, {500.f, 520.f}, "Cancel"),
      ipField(font, {460.f, 250.f}, {280.f, 40.f}),
      portField(font, {460.f, 310.f}, {280.f, 40.f}),
      title(font),
      hint(font),
      ta(),
      opp(),
      localRenderer(ta, window, font),
      oppRenderer({SfmlGame::GRID_POS_X - (SfmlGame::GRID_WIDTH + 60.f), SfmlGame::GRID_POS_Y}),
      rematchBtn(font, {500.f, 520.f}, "Rematch"),
      leaveBtn(font, {500.f, 600.f}, "Leave") {

    title.setString("Multiplayer");
    title.setCharacterSize(28);
    title.setFillColor(sf::Color::White);
    title.setPosition({500.f, 160.f});

    hint.setCharacterSize(18);
    hint.setFillColor(sf::Color::White);
    hint.setPosition({420.f, 700.f});

    ipField.setAllowDot(true);
    ipField.setMaxLen(32);
    ipField.setValue("127.0.0.1");

    portField.setAllowDot(false);
    portField.setMaxLen(5);
    portField.setValue("5555");

    setStatus("Choose Host or Join");
}

void SfmlMultiplayer::onEnter() {
    // Prevent the click that switched states from immediately triggering a button.
    enterCooldownFrames = 2;
}

SfmlMultiplayer::~SfmlMultiplayer() {
    if (sock != INVALID_SOCKET) {
        sendLine(sock, "LEAVE");
    }
    closeSock();
    stopHosting();
}

void SfmlMultiplayer::handleEvent(const sf::Event& ev, sf::RenderWindow& window) {
    if (mode == Mode::Menu && menuPane == MenuPane::JoinForm) {
        ipField.handleEvent(ev, window);
        portField.handleEvent(ev, window);
    }

    if (const auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
        if (mode == Mode::Playing || mode == Mode::Waiting) {
            if (!pauseConfirmActive && kp->code == sf::Keyboard::Key::Escape) {
                pauseConfirmActive = true;
                return;
            }
            if (pauseConfirmActive) {
                if (kp->code == sf::Keyboard::Key::Y) {
                    if (sock != INVALID_SOCKET) sendLine(sock, "LEAVE");
                    running = false;
                    pauseConfirmActive = false;
                    backToMainMenuRequested = true;
                    return;
                }
                if (kp->code == sf::Keyboard::Key::N || kp->code == sf::Keyboard::Key::Escape) {
                    pauseConfirmActive = false;
                    return;
                }
            }
        }

        if (kp->code == sf::Keyboard::Key::Escape) {
            if (mode == Mode::Connecting) {
                closeSock();
                mode = Mode::Menu;
                menuPane = MenuPane::JoinForm;
                setStatus("Connection cancelled.");
                return;
            }
            if (mode == Mode::Menu && menuPane != MenuPane::Root) {
                menuPane = MenuPane::Root;
                setStatus("Choose Host or Join");
                return;
            }
            // Leave multiplayer.
            if (sock != INVALID_SOCKET) sendLine(sock, "LEAVE");
            running = false;
        }
    }
}

bool SfmlMultiplayer::update(Mouse& mouse) {
    if (mode == Mode::Connecting) {
        // While connecting we keep the UI alive: allow Cancel + timeout.
        cancelConnectBtn.Update(mouse);
        if (cancelConnectBtn.getOnRelease()) {
            closeSock();
            mode = Mode::Menu;
            menuPane = MenuPane::JoinForm;
            setStatus("Connection cancelled.");
            return false;
        }

        const auto now = clock::now();
        if (now - connectStart >= connectTimeout) {
            closeSock();
            mode = Mode::Menu;
            menuPane = MenuPane::JoinForm;
            setStatus("Connection timed out.");
            return false;
        }

        bool connected = false;
        std::string err;
        const bool finished = pollConnect(connected, err);
        if (finished) {
            if (connected) {
                startSessionAfterConnect();
            } else {
                closeSock();
                mode = Mode::Menu;
                menuPane = MenuPane::JoinForm;
                setStatus("Failed to connect.");
            }
        }
        return false;
    }

    // Network tick
    if (running) pumpNetwork();
    if (running == false && mode != Mode::Menu) {
        closeSock();
        stopHosting();
        mode = Mode::Menu;
        setStatus("Disconnected.");
    }

    // Game logic (playing)
    if (mode == Mode::Playing) {
        // Important UX detail:
        // - In multiplayer, the pause confirm only disables LOCAL controls.
        // - The match (gravity + networking) keeps running in the background.
        auto now = clock::now();
        if (now - lastFall >= fallInterval) {
            ta.block_descend();
            lastFall = now;

            int cleared = ta.pop_cleared_lines_event();
            if (cleared > 0) sendLine(sock, "CLEARED " + std::to_string(cleared));

            int landed = ta.pop_landed_event();
            if (landed > 0) {
                if (pendingIncomingGarbage > 0) {
                    ta.apply_garbage(pendingIncomingGarbage);
                    pendingIncomingGarbage = 0;
                }
                ta.spawn_if_needed();
            }

            sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
        }

        // Local controls
        if (!pauseConfirmActive) {
            localRenderer.HandleEvents();
        }

        int cleared = ta.pop_cleared_lines_event();
        if (cleared > 0) sendLine(sock, "CLEARED " + std::to_string(cleared));

        int landed = ta.pop_landed_event();
        if (landed > 0) {
            if (pendingIncomingGarbage > 0) {
                ta.apply_garbage(pendingIncomingGarbage);
                pendingIncomingGarbage = 0;
            }
            ta.spawn_if_needed();
        }

        sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());

        if (ta.is_game_over() && !sentGameOver) {
            sendLine(sock, "GAMEOVER");
            sentGameOver = true;
            sendLine(sock, "BOARD " + std::to_string(ta.get_score()) + " " + ta.serialize_board());
            mode = Mode::PostGame;
            setStatus("YOU LOSE");
        }
    }

    // UI interactions
    if (mode == Mode::Menu) {
        applyMenuLayout();
        if (enterCooldownFrames > 0) {
            --enterCooldownFrames;
            return false;
        }
        if (menuPane == MenuPane::Root) {
            hostBtn.Update(mouse);
            joinBtn.Update(mouse);
            backBtn.Update(mouse);

            if (hostBtn.getOnRelease()) {
                const int port = 5555;
                std::string err;
                if (!startServer(port, server, err)) {
                    setStatus(err);
                } else {
                    hosting = true;
                    startClient("127.0.0.1", port, true);
                    setStatus("Room created. Waiting for opponent...");
                }
            }

            if (joinBtn.getOnRelease()) {
                menuPane = MenuPane::JoinForm;
                setStatus("Enter IP and port, then click Connect.");
            }

            if (backBtn.getOnRelease()) {
                return true;
            }
        } else {
            connectBtn.Update(mouse);
            backBtn.Update(mouse);

            if (connectBtn.getOnRelease()) {
                int port = 5555;
                try { port = std::stoi(portField.getValue()); } catch (...) {}
                startClient(ipField.getValue(), port, false);
            }

            if (backBtn.getOnRelease()) {
                menuPane = MenuPane::Root;
                setStatus("Choose Host or Join");
            }
        }
    }

    if (mode == Mode::PostGame) {
        rematchBtn.Update(mouse);
        leaveBtn.Update(mouse);

        if (rematchBtn.getOnRelease()) {
            sendLine(sock, "REMATCH YES");
            postGameWaiting = true;
            mode = Mode::Waiting;
            setStatus("Waiting for opponent to accept rematch...");
        }
        if (leaveBtn.getOnRelease()) {
            sendLine(sock, "LEAVE");
            running = false;
        }
    }

    if (backToMainMenuRequested) {
        backToMainMenuRequested = false;
        return true;
    }

    return false;
}

void SfmlMultiplayer::draw(sf::RenderWindow& window) {
    const sf::Color kHudBg(0, 0, 0, 210);
    const sf::Color kHudOutline(255, 255, 255, 230);

    if (mode == Mode::Menu) {
        applyMenuLayout();
        drawTextWithBox(window, title, 10.f, kHudBg, kHudOutline, 2.f);

        if (menuPane == MenuPane::Root) {
            hostBtn.draw(window);
            joinBtn.draw(window);
            backBtn.draw(window);
        } else {
            sf::Text ipLabel(title);
            ipLabel.setString("IP:");
            ipLabel.setCharacterSize(18);
            ipLabel.setFillColor(sf::Color::White);
            ipLabel.setPosition({380.f, 255.f});
            drawTextWithBox(window, ipLabel, 6.f, kHudBg, kHudOutline, 2.f);

            sf::Text portLabel(title);
            portLabel.setString("Port:");
            portLabel.setCharacterSize(18);
            portLabel.setFillColor(sf::Color::White);
            portLabel.setPosition({380.f, 315.f});
            drawTextWithBox(window, portLabel, 6.f, kHudBg, kHudOutline, 2.f);

            ipField.draw(window);
            portField.draw(window);

            connectBtn.draw(window);
            backBtn.draw(window);
        }
    }
    else if (mode == Mode::Connecting) {
        sf::Text t(title);
        t.setFillColor(sf::Color::White);
        t.setString(statusLine + "\n" + sessionIp + ":" + std::to_string(sessionPort));
        t.setCharacterSize(28);
        wrapTextToWidth(t, 1040.f);
        t.setPosition({80.f, 180.f});
        drawTextWithBox(window, t, 12.f, kHudBg, kHudOutline, 2.f);

        cancelConnectBtn.draw(window);
    }
    else if (mode == Mode::Waiting) {
        sf::Text t(title);
        t.setFillColor(sf::Color::White);
        t.setString(statusLine);
        t.setCharacterSize(30);
        wrapTextToWidth(t, 1040.f);
        t.setPosition({80.f, 160.f});
        drawTextWithBox(window, t, 12.f, kHudBg, kHudOutline, 2.f);

        if (isHost && sessionPort > 0) {
            sf::Text info(title);
            info.setFillColor(sf::Color::White);
            info.setCharacterSize(22);
            info.setString("Host is running. Ask your opponent to join using the host IP on the network, port " + std::to_string(sessionPort));
            wrapTextToWidth(info, 1040.f);
            info.setPosition({80.f, 230.f});
            drawTextWithBox(window, info, 12.f, kHudBg, kHudOutline, 2.f);
        }
    }
    else if (mode == Mode::Playing) {
        oppRenderer.draw(window, opp);
        localRenderer.draw_game();

        sf::Text g(title);
        g.setString("Garbage: +" + std::to_string(pendingIncomingGarbage));
        g.setCharacterSize(16);
        g.setPosition({SfmlGame::PANEL_POS_X, SfmlGame::GRID_POS_Y + 700.f});
        g.setFillColor(sf::Color::White);
        drawTextWithBox(window, g, 8.f, kHudBg, kHudOutline, 2.f);
    }
    else if (mode == Mode::PostGame) {
        oppRenderer.draw(window, opp);
        localRenderer.draw_game();

        sf::RectangleShape overlay;
        overlay.setSize({1200.f, 800.f});
        overlay.setFillColor(sf::Color(0, 0, 0, 120));
        window.draw(overlay);

        sf::Text msg(title);
        msg.setString(statusLine);
        msg.setCharacterSize(28);
        msg.setFillColor(sf::Color::White);
        msg.setPosition({450.f, 420.f});
        drawTextWithBox(window, msg, 14.f, kHudBg, kHudOutline, 2.f);

        rematchBtn.draw(window);
        leaveBtn.draw(window);
    }

    if (pauseConfirmActive && (mode == Mode::Playing || mode == Mode::Waiting)) {
        sf::RectangleShape overlay;
        overlay.setSize({1200.f, 800.f});
        overlay.setFillColor(sf::Color(0, 0, 0, 140));
        window.draw(overlay);

        sf::Text msg(title);
        msg.setString("Exit to menu?\nY = yes\nN = no");
        msg.setCharacterSize(32);
        msg.setFillColor(sf::Color::White);
        msg.setPosition({390.f, 320.f});
        drawTextWithBox(window, msg, 14.f, kHudBg, kHudOutline, 2.f);
    }

    hint.setFillColor(sf::Color::White);
    hint.setCharacterSize(20);
    drawTextWithBox(window, hint, 10.f, kHudBg, kHudOutline, 2.f);
}
