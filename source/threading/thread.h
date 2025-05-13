#ifndef THREAD_H
#define THREAD_H

#include <QObject>
#include <thread>

class Thread : public QObject {
    Q_OBJECT

public:
    explicit Thread(QObject *parent = nullptr)
        : QObject(parent), running(false) {}

    virtual ~Thread() {
        if (worker.joinable()) {
            worker.join();
        }
    }

    void start() {
        if (running) return;

        if (worker.joinable()) {
            worker.join();
        }

        running = true;
        worker = std::thread([this]() { run(); });
    }

    void stop() {
        running = false;
        if (worker.joinable()) {
            worker.join();
        }
    }

    bool isRunning() const { return running; }

protected:
    virtual void run() = 0;

    std::thread worker;
    std::atomic<bool> running;
};

#endif // THREAD_H
