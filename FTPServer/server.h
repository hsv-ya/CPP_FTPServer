#include <iostream> 
#include <string>
#include <fstream>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Userenv.h>


#define WSVERS MAKEWORD(2,2)
#define DEFAULT_PORT "21"
#define USE_IPV6 true
#define BUFFER_SIZE 200
#define FILENAME_SIZE 1024
#define BIG_BUFFER_SIZE 65535

bool debugMode(int argc, char * argv[]);                                            // Returns true if user indicated that debug mode should be on.
int startWinsock(bool debug);                                                       // Starts WSA.
int getServerAddressInfo(struct addrinfo * &result, int argc, char *argv[], bool debug);    // Gets the servers address information based on arguments.
int allocateServerSocket(SOCKET &s, struct addrinfo *result, bool debug);           // Allocates the server's socket.
int bindServerSocket(SOCKET &s, struct addrinfo *result, bool debug);               // Bind server socket to result address.
int getServerNameInfo(SOCKET &s, char serverHost[], char serverService[], bool debug);  // Gets the host and service information for the server.

int serverListen(SOCKET &s, bool debug);                                            // Listen for client communication and deal with it accordingly.
int startListen(SOCKET &s, bool debug);                                             // Starts server listening for incoming connections.
int acceptClients(SOCKET &s, bool debug);                                           // Accepts new clients and deals with commands.
int acceptNewClient(SOCKET &ns, SOCKET &s, struct sockaddr_storage &clientAddress, char clientHost[], char clientService[], bool debug);    // Takes incoming connection and assigns new socket.

bool communicateWithClient(SOCKET &ns, SOCKET &sDataActive, bool &authroisedLogin, bool debug, unsigned long &clientId, char currentDirectory[FILENAME_SIZE]); // Receive and handle messages from client, returns true if client ends connection.
bool receiveMessage(SOCKET &ns, char receiveBuffer[], bool debug);                  // Receives message and saves it in receive buffer, returns true if message was received.
void closeClientConnection(SOCKET &ns, bool debug);                                 // Sends client the closing connection method and closes the socket.
bool commandUserName(SOCKET &ns, char receiveBuffer[], char userName[], bool &authroisedLogin, bool debug); // Client sent USER command, returns false if fails.
bool commandPassword(SOCKET &ns, char receiveBuffer[], char password[], bool authroisedLogin, bool debug);  // Client sent PASS command, returns false if fails.
bool commandSystemInformation(SOCKET &ns, bool debug);                              // Client sent SYST command, returns false if fails.
bool commandQuit(SOCKET &ns, bool debug);                                           // Client sent QUIT command, returns false if fails.
bool commandDataPort(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug);    // Client sent PORT command, returns false if fails.
bool commandList(SOCKET &ns, SOCKET &sDataActive, bool debug, unsigned long &clientId, char currentDirectory[]);     // Client sent LIST command, returns false if fails.
bool commandRetrieve(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug, char currentDirectory[]);    // Client sent RETR command, returns false if fails.
bool commandStore(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug, char currentDirectory[]);   // Client sent STORE command, returns false if fails.
bool commandChangeWorkingDirectory(SOCKET &ns, char receiveBuffer[], bool debug, char currentDirectory[]);    // Client sent CWD command, returns false if connection ended.
bool commandUnknown(SOCKET &ns, bool debug);                                        // Client sent unknown command, returns false if send fails.
int executeSystemCommand(const char commandNameWithKeys[], const char fileName[], bool debug);  // Execute system command.

void removeCommand(const char inputString[], char outputString[], unsigned int skipCharacters = 4);    // Takes a string with a 4 letter command at beggining and saves an output string with this removed.
bool isValidUserName(const char userName[]);                                        // Returns true if valid user name.
bool isValidPassword(const char userName[], bool authroisedLogin);                  // Returns true if valid password.
bool isEmailAddress(const char address[]);                                          // Check is inputted string is valid email address.
bool isAlphabetical(const char c);                                                  // Returns true if the character is alphabetical.
bool isNumerical(const char c);                                                     // Returns true if the character is a number.
void killLastCRLF(char buffer[]);                                                   // Delete <CR> and <LF> in end of string.
void replaceBackslash(char buffer[]);                                               // Replace '/' to '\' for Windows

bool getClientIPPort(SOCKET &ns, char receiveBuffer[], char ipDecimal[], char portBuffer[], bool debug);    // Gets the client's IP and port number for active connection.
bool getClientAddressInfoActive(SOCKET &ns, struct addrinfo * &result, const char ipBuffer[], const char portBuffer[], bool debug); // Gets the client's address information based on arguments.
bool allocateDataTransferSocket(SOCKET &sDataActive, struct addrinfo *result, bool debug);  // Allocates the socket for data transfer.
bool connectDataTransferSocket(SOCKET &sDataActive, struct addrinfo *result, bool debug);   // Bind data transfer socket to result address.
bool sendFailedActiveConnection(SOCKET &ns, bool debug);                            // Sends the client a message to say data connection failed.
bool sendArgumentSyntaxError(SOCKET &ns, bool debug);                               // Sends the client a message to say data connection failed.

int sendFile(SOCKET &ns, SOCKET &sDataActive, const char fileName[], bool debug, unsigned long clientId, char currentDirectory[]);   // Sends specified file to client.
bool saveFile(SOCKET &ns, SOCKET &sDataActive, const char fileName[], bool debug, char currentDirectory[]);  // Sends specified file to client.
bool receiveFileContents(SOCKET &sDataActive, char receiveBuffer[], int &sizeBuffer);   // Receives message and saves it in receive buffer, returns false if connection ended.

