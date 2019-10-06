#include "socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>

using namespace std;

void get_URL(const string &host, const string &path) {
    // Your code here.
    // service	name (from /etc/services, e.g., "http" is port 80)
    Address addr(host, "http");
    TCPSocket socket;
    socket.connect(addr);

    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.
    string s("GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n");
    cout << s << endl;
    socket.write(s);
    // 客户端要先shutdown write，这样服务端在回送响应后，才会shutdown write，
    // 这样客户端的read()才会返回eof(end of file)。
    // If you don’t shut down your outgoing byte stream,
    // the server will wait around for a while for you to send
    // additional requests and won’t end its outgoing byte stream either.
    socket.shutdown(SHUT_WR);

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).
    while (!socket.eof()) {
        socket.read(s);
        cout << s;  // 返回的HTTP文本自带\r\n。
    }
    socket.close();

    // cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
    // cerr << "Warning: get_URL() has not been implemented yet.\n";
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
