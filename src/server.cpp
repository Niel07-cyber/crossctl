#include "crossctl/server.h"
#include "ctelegram/telegram.h"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <vector>

namespace crossctl {

namespace {

volatile sig_atomic_t g_stop = 0;

void on_signal(int) { g_stop = 1; }

// Telegram type codes.
constexpr uint8_t TYPE_EVENT  = 0x10;
constexpr uint8_t TYPE_STATUS = 0x20;
constexpr uint8_t TYPE_ERROR  = 0x30;

bool event_from_payload(const uint8_t *p, uint16_t len, Event &out)
{
    const std::string s(reinterpret_cast<const char *>(p), len);
    if (s == "TRAIN_DETECTED") { out = Event::TrainDetected;        return true; }
    if (s == "TRAIN_CLEARED")  { out = Event::TrainCleared;         return true; }
    if (s == "BARRIER_DOWN")   { out = Event::BarrierDownConfirmed; return true; }
    if (s == "BARRIER_UP")     { out = Event::BarrierUpConfirmed;   return true; }
    if (s == "TICK")           { out = Event::Tick;                 return true; }
    if (s == "RESET")          { out = Event::Reset;                return true; }
    return false;
}

bool send_frame(int fd, uint8_t type, uint16_t seq, const std::string &body)
{
    tg_frame_t f;
    std::memset(&f, 0, sizeof(f));
    f.type = type;
    f.seq  = seq;
    f.len  = static_cast<uint16_t>(body.size() > TG_MAX_PAYLOAD
                                   ? TG_MAX_PAYLOAD : body.size());
    std::memcpy(f.payload, body.data(), f.len);

    uint8_t wire[TG_MAX_FRAME];
    size_t  wire_len = 0;
    if (tg_encode(&f, wire, sizeof(wire), &wire_len) != TG_OK) return false;

    size_t sent = 0;
    while (sent < wire_len) {
        ssize_t n = ::send(fd, wire + sent, wire_len - sent, 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

} // namespace

int run_server(const ServerConfig &cfg)
{
    // sigaction without SA_RESTART: blocking accept()/recv() must return
    // EINTR so the shutdown flag is actually observed. Plain signal() has
    // restart semantics on BSD/macOS and would hang here.
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);
    ::sigaction(SIGINT,  &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
    std::signal(SIGPIPE, SIG_IGN);

    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { std::perror("socket"); return 1; }

    int yes = 1;
    ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port        = htons(cfg.port);

    if (::bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::perror("bind");
        ::close(listen_fd);
        return 1;
    }
    if (::listen(listen_fd, 1) < 0) {
        std::perror("listen");
        ::close(listen_fd);
        return 1;
    }

    // Printed so test harnesses can wait for readiness instead of sleeping.
    std::fprintf(stderr, "crossctl listening on 127.0.0.1:%u\n", cfg.port);
    std::fflush(stderr);

    StateMachine sm(cfg.move_timeout);

    while (!g_stop) {
        int fd = ::accept(listen_fd, nullptr, nullptr);
        if (fd < 0) {
            if (g_stop) break;
            if (errno == EINTR) continue;
            continue;
        }
        std::fprintf(stderr, "client connected\n");
        std::fflush(stderr);

        std::vector<uint8_t> acc;
        uint8_t chunk[512];

        for (;;) {
            ssize_t n = ::recv(fd, chunk, sizeof(chunk), 0);
            if (n <= 0) break;
            acc.insert(acc.end(), chunk, chunk + n);

            // Drain every complete frame currently buffered.
            for (;;) {
                tg_frame_t f;
                size_t consumed = 0;
                tg_status_t st = tg_decode(acc.data(), acc.size(), &f, &consumed);

                if (st == TG_ERR_TOO_SHORT) break;   // wait for more bytes

                if (st != TG_OK) {
                    std::fprintf(stderr, "reject: %s\n", tg_strerror(st));
                    std::fflush(stderr);
                    send_frame(fd, TYPE_ERROR, 0, tg_strerror(st));
                    acc.clear();                      // resync
                    break;
                }

                acc.erase(acc.begin(),
                          acc.begin() + static_cast<long>(consumed));

                if (f.type != TYPE_EVENT) {
                    send_frame(fd, TYPE_ERROR, f.seq, "unexpected type");
                    continue;
                }

                Event ev;
                if (!event_from_payload(f.payload, f.len, ev)) {
                    send_frame(fd, TYPE_ERROR, f.seq, "unknown event");
                    continue;
                }

                State before = sm.state();
                sm.handle(ev);
                Output o = sm.output();

                std::fprintf(stderr, "%s: %s -> %s [%s/%s]\n",
                             to_string(ev), to_string(before),
                             to_string(sm.state()),
                             to_string(o.barrier), to_string(o.signal));
                std::fflush(stderr);

                std::string status = std::string(to_string(sm.state())) + " " +
                                     to_string(o.barrier) + " " +
                                     to_string(o.signal);
                send_frame(fd, TYPE_STATUS, f.seq, status);
            }
        }

        std::fprintf(stderr, "client disconnected\n");
        std::fflush(stderr);
        ::close(fd);
    }

    ::close(listen_fd);
    std::fprintf(stderr, "shutting down\n");
    return 0;
}

} // namespace crossctl
