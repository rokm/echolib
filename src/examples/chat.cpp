/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#include <iostream>
#include <thread>
#include <string>
#include <mutex>

#include "client.h"
#include "datatypes.h"

using namespace std;
using namespace echolib;

#define DELIMITER "$ "

namespace echolib {

template <> struct TypeIdentifier<pair<string, string> > {
    static const char* name;
};
const char* TypeIdentifier<pair<string, string> >::name = "string pair";

template<> inline shared_ptr<Message> echolib::Message::pack<pair<string, string> >(int channel, const pair<string, string> &data) {
    MessageWriter writer(data.first.size() + data.second.size() + 8);

    writer.write_string(data.first);
    writer.write_string(data.second);

    return make_shared<BufferedMessage>(channel, writer);
}

template<> inline shared_ptr<pair<string, string> > echolib::Message::unpack<pair<string, string> >(SharedMessage message) {
    MessageReader reader(message);

    string user = reader.read_string();
    string text = reader.read_string();

    return make_shared<pair<string, string> >(user, text);
}

}

int main(int argc, char** argv) {

    //Connect to local socket based daemon
    SharedClient client = make_shared<echolib::Client>(string(argv[1]));

    string name;
    cout << "Enter your name\n";
    std::getline(std::cin, name);

    std::mutex mutex;

    function<void(shared_ptr<pair<string, string> >)> chat_callback = [&](shared_ptr<pair<string, string> > m) {
        const string name = m->first;
        const string message = m->second;
        std::cout<< name << ": " << message << std::endl;
    };

    function<void(int)> subscribe_callback = [&](int m) {
        std::cout << "Total subscribers: " << m << std::endl;
    };

    TypedSubscriber<pair<string, string> > sub(client, "chat", chat_callback);
    SubscriptionWatcher watch(client, "chat", subscribe_callback);
    TypedPublisher<pair<string, string> > pub(client, "chat");

    std::thread write([&]() {
        while (client->is_connected()) {
            string message;
            std::getline(std::cin, message);
            pub.send(pair<string, string>(name, message));
        }

    });

    while(client->wait(10)) {
        // We have to give the write thread some space
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    exit(0);
}
