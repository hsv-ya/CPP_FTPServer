//=======================================================================================================================
//ACTIVE FTP SERVER Start-up Code for Assignment 1 (WinSock 2)

//This code gives parts of the answers away.
//Firstly, you must change parts of this program to make it IPv6-compliant (replace all data structures that work only with IPv4).
//This step would require creating a makefile, as the IPv6-compliant functions require data structures that can be found only by linking with the appropritate library files. 
//The sample TCP server codes will help you accomplish this.

//OVERVIEW
//The connection is established by ignoring USER and PASS, but sending the appropriate 3 digit codes back
//only the active FTP mode connection is implemented (watch out for firewall issues - do not block your own FTP server!).

//The ftp LIST command is fully implemented, in a very naive way using redirection and a temporary file.
//The list may have duplications, extra lines etc, don't worry about these. You can fix it as an exercise, 
//but no need to do that for the assignment.
//In order to implement RETR you can use the LIST part as a startup.  RETR carries a filename, 
//so you need to replace the name when opening the file to send.

//STOR is also a few steps away, use your implementation of RETR and invert it to save the file on the server's dir
//=======================================================================================================================

#include "server.h"

const char g_sMonths[12][4] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

const char systemCommandDEL[] = "del";
const char systemCommandMKDIR[] = "mkdir";
const char systemCommandRMDIR[] = "rmdir";

bool g_convertKirillica = false;

// Arguments:
//      0:  Program name
//      1:  Port number
//      2:  Debug mode (true/false)
//      3:  Use convert kirillica file and directory name between Android and Windows 7 (true/false)
int main(int argc, char *argv[])
{
    bool debug = debugMode(argc, argv);                                             // Debug mode off by default.

    g_convertKirillica = useConvertKirillica(argc, argv);                           // Off by default.

	if (NULL == getenv("TEMP"))                                                     // in Windows environment string <TEMP> MUST HAVE!!
	{
		if (debug)
		{
			std::cout << "Error, not find environment <TEMP>!!" << std::endl;
		}
		return 1;
	}
	else
	{
		if (strlen(getenv("TEMP")) > FILENAME_SIZE - 50)
		{
			if (debug)
			{
				std::cout << "Error, very long size for environment <TEMP>!!" << std::endl;
			}
			return 1;
		}
	}

    int error = startWinsock(debug);                                                // Start winsock.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    struct addrinfo *result = NULL;                                                 // Structure for server's address information.

    error = getServerAddressInfo(result, argc, argv, debug);                        // Get server's address information.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    SOCKET s = INVALID_SOCKET;                                                      // Socket for listening.

    error = allocateServerSocket(s, result, debug);                                 // Allocate server socket.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    error = bindServerSocket(s, result, debug);                                     // Bind the server socket
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    freeaddrinfo(result);                                                           // Free memory.

    char serverHost[NI_MAXHOST];                                                    // The server's IP number.
    char serverService[NI_MAXSERV];                                                 // The server's port number.

    error = getServerNameInfo(s, serverHost, serverService, debug);                 // Get the server name information.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    error = serverListen(s, debug);                                                 // Listen for client connections and deal with them accordingly.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    return 0;                                                                       // Program completed without error.
}

// Returns true if user indicated that debug mode should be on.
bool debugMode(int argc, char * argv[])
{
    if (argc > 2)                                                                   // Check if there are more than 2 arguments.
    {
        if (strcmp(argv[2], "true") == 0)                                           // Check if argument 3 is debug command.
        {
            return true;                                                            // Set debug mode on.
        }
    }

    return false;                                                                   // Set debug mode off.
}

// Returns true if user indicated that convert kirillica should be on.
bool useConvertKirillica(int argc, char * argv[])
{
    if (argc > 3)                                                                   // Check if there are more than 3 arguments.
    {
        if (strcmp(argv[3], "true") == 0)                                           // Check if argument 4 is convert kirillica command.
        {
            return true;                                                            // Set convert kirillica on.
        }
    }

    return false;                                                                   // Set convert kirillica off.
}

// Starts WSA.
int startWinsock(bool debug)
{
    WSADATA wsadata;
    int err = WSAStartup(WSVERS, &wsadata);                                         // Initialise use of winsock dll and save error code.

    if (err != 0)                                                                   // Check if there was an error starting winsock.
    {
        std::cout << "WSAStartup failed with error: " << err << std::endl;          // Tell the user that we could not fInd a usable WinsockDLL.
        WSACleanup();                                                               // Clean up winsock.

        return 1;                                                                   // Return error code.
    }
    else if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2)        // Ensure the use of winsock 2.2 is supported.
    {
        std::cout << "Could not fInd a usable version of Winsock.dll" << std::endl;;
        WSACleanup();                                                               // Clean up winsock.

        return 2;                                                                   // Return error code.
    }
    else
    {
        std::cout << std::endl << "===============================" << std::endl;
        std::cout <<              "     159.334 FTP Server        ";
        std::cout << std::endl << "===============================" << std::endl;
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Winsock started." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Gets the servers address information based on arguments.
int getServerAddressInfo(struct addrinfo * &result, int argc, char *argv[], bool debug)
{
    struct addrinfo hints;                                                          // Create address info hint structure.
    memset(&hints, 0, sizeof(struct addrinfo));                                     // Clean up the structure.

    hints.ai_family = AF_INET;                                                      // Set address family as internet (IPv4).
    hints.ai_socktype = SOCK_STREAM;                                                // Set socket type as socket stream (for TCP).
    hints.ai_protocol = IPPROTO_TCP;                                                // Set protocol as TCP.
    hints.ai_flags = AI_PASSIVE;                                                    // Set flags as passive.

    int iResult;                                                                    // Create return value.
    if (argc > 1)
    {
        iResult = getaddrinfo(NULL, argv[1], &hints, &result);                      // Resolve local address and port to be used by the server.
    }
    else
    {
        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);                 // Resolve local address and port to be used by the server.
    }

    if (iResult != 0)                                                               // Check for expected execution of getaddrinfo.
    {
        std::cout << "getaddrinfo() failed: " << iResult << std::endl;
        WSACleanup();                                                               // Clean up winsock.

        return 3;                                                                   // Return error code.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server address information created." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Allocates the server's socket.
int allocateServerSocket(SOCKET &s, struct addrinfo *result, bool debug)
{
    s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);        // Allocate socket.

    if (s == INVALID_SOCKET)                                                        // Check for error in socket allocation.
    {
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);                                                       // Free memory.
        WSACleanup();                                                               // Clean up winsock.

        return 4;                                                                   // Return error code.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server socket allocated." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Bind server socket to result address.
int bindServerSocket(SOCKET &s, struct addrinfo *result, bool debug)
{
    int iResult = bind(s, result->ai_addr, (int) result->ai_addrlen);               // Bind the listening socket to the server's IP address and port number.

    if (iResult == SOCKET_ERROR)                                                    // Check if error occurred in socket binding.
    {
        std::cout << "Bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);                                                       // Free memory.
        closesocket(s);                                                             // Close socket.
        WSACleanup();                                                               // Clean up winsock.

        return 5;                                                                   // Return error code.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server socket bound." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Gets the host and service information for the server.
int getServerNameInfo(SOCKET &s, char serverHost[], char serverService[], bool debug)
{
    struct sockaddr_storage serverAddress;                                          // The server's address information.

    int addressLength = sizeof(serverAddress);                                      // Get the size of the client address sockaddr structure.

    DWORD returnValue = getsockname(s, (struct sockaddr *) &serverAddress, (socklen_t *) &addressLength);   // Get socket information.
    if(returnValue != 0)                                                            // Check that sock name obtained as expected.
    {
        std::cout << "getsockname() failed: " << returnValue;

        return 6;                                                                   // Return error code.
    }

    returnValue = getnameinfo((struct sockaddr *) &serverAddress, addressLength,    // Get name info for server.
                              serverHost, NI_MAXHOST,
                              serverService, NI_MAXSERV,
                              NI_NUMERICHOST);

    if(returnValue != 0)                                                            // Check that name info obtained as expected.
    {
        std::cout << "getnameinfo() failed: " << returnValue;

        return 7;                                                                   // Return error code.
    }
    else
    {
        std::cout << "Sever intialised at Port: " << serverService << std::endl;
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server name information collected." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Listen for client communication and deal with it accordingly.
int serverListen(SOCKET &s, bool debug)
{
    int error = startListen(s, debug);                                              // Start server listening for incoming connections.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    while (!error)                                                                  // Start main listening loop which continues infInitely while server runs.
    {
        error = acceptClients(s, debug);                                            // Accept clients and deal with commands.
    }

    closesocket(s);                                                                 // Close listening socket.
    std::cout << "SERVER SHUTTING DOWN..." << std::endl;

    return error;                                                                   // Return error code.
}

// Starts server listening for incoming connections.
int startListen(SOCKET &s, bool debug)
{
    if (listen(s, SOMAXCONN) == SOCKET_ERROR)                                       // Start listening to socket s and check if error.
    {
        std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(s);                                                             // Close socket.
        WSACleanup();                                                               // Clean up winsock.

        return 8;                                                                   // Exit with error code.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server started listening." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Accepts new clients and deals with commands.
int acceptClients(SOCKET &s, bool debug)
{
    struct sockaddr_storage clientAddress;                                          // The client's address information.
    char clientHost[NI_MAXHOST];                                                    // The client's IP number.
    char clientService[NI_MAXSERV];                                                 // The client's port number.

    memset(clientHost, 0, sizeof(clientHost));                                      // Ensure blank.
    memset(clientService, 0, sizeof(clientService));                                // Ensure blank.

    SOCKET ns = INVALID_SOCKET;                                                     // New socket for new client.

    int error = acceptNewClient(ns, s, clientAddress, clientHost, clientService, debug);    // Takes incoming connection and assigns new socket.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"220 FTP Server ready.\r\n");                                // Add accept message for client to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send accept message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if ((bytes == SOCKET_ERROR) || (bytes == 0))                                    // If send failed client has disconnected.
    {
        closeClientConnection(ns, debug);                                           // Close client connection.

        return 0;                                                                   // Connection ended, leave function.
    }

    bool success = true;                                                            // The status of the connection with the client.
    bool authroisedLogin = false;                                                   // Flag for if user logging in is authorised or not.

    SOCKET sDataActive = INVALID_SOCKET;                                            // Socket for data transfer.

	unsigned long clientId = 1;                                                     // For check send <Directory> or <FILE DATA>.

	char currentDirectory[FILENAME_SIZE];                                           // For store currect directory.
    memset(&currentDirectory, 0, FILENAME_SIZE);                                    // Ensure blank.

    while (success)
    {
        success = communicateWithClient(ns, sDataActive, authroisedLogin, debug, clientId, currentDirectory);   // Receive and handle messages from client.
    }

    closeClientConnection(ns, debug);                                               // Close client connection.

    return 0;                                                                       // Completed without error.
}

// Takes incoming connection and assigns new socket.
int acceptNewClient(SOCKET &ns, SOCKET &s, struct sockaddr_storage &clientAddress, char clientHost[], char clientService[], bool debug)
{
    int addressLength = sizeof(clientAddress);                                      // Get the size of the client address sockaddr structure.

    ns = accept(s, (struct sockaddr *) &clientAddress, (socklen_t *) &addressLength);   // Accept a new client on new socket.

    if (ns == INVALID_SOCKET)                                                       // Check if client not accepted as expected.
    {
        std::cout << "Accept failed: " << WSAGetLastError() << std::endl;
        closesocket(s);                                                             // Close socket.
        WSACleanup();                                                               // Clean up winsock.

        return 9;                                                                   // Exit with error code.
    }
    else
    {
        std::cout << std::endl << "A client has been accepted." << std::endl;

        DWORD returnValue = getnameinfo((struct sockaddr *)&clientAddress, addressLength,   // Get name info for client.
                                  clientHost, NI_MAXHOST,
                                  clientService, NI_MAXSERV,
                                  NI_NUMERICHOST);

        if (returnValue != 0)                                                       // Check that name info obtained as expected.
        {
            std::cout << "getnameinfo() failed: " << returnValue;

            return 10;                                                              // Exit with error code.
        }
        else
        {
            std::cout << "Connected to client with IP address: " << clientHost;
            std::cout << ", at Port: " << clientService << std::endl;
        }
    }

    return 0;                                                                       // Completed without error.
}

// Receive and handle messages from client, returns false if client ends connection.
bool communicateWithClient(SOCKET &ns, SOCKET &sDataActive, bool &authroisedLogin, bool debug, unsigned long &clientId, char currentDirectory[])
{
    char receiveBuffer[BUFFER_SIZE];                                                // Set up receive buffer.
    char userName[80];                                                              // To store the client's username.
    char password[80];                                                              // To store the client's password.

    memset(&receiveBuffer, 0, BUFFER_SIZE);                                         // Ensure blank.
    memset(&userName, 0, 80);                                                       // Initialise as blank string.
    memset(&password, 0, 80);                                                       // Initialise as blank string.

    bool receiptSuccessful = true;                                                  // True if reply sent successfully.

    receiptSuccessful = receiveMessage(ns, receiveBuffer, debug);                   // Receives message and saves it in receive buffer.
    if (!receiptSuccessful)                                                         // Check if user ended connection.
    {
        return receiptSuccessful;                                                   // Return that user ended connection.
    }

    bool success = true;                                                            // True if reply sent successfully.

    if (strncmp(receiveBuffer, "USER", 4) == 0)                                     // Check if USER message received from client.
    {
        int i = 0;                                                                  // Count number of login attempts.

        do
        {
            success = commandUserName(ns, receiveBuffer, userName, authroisedLogin, debug); // Handle command.

            if (!success)
            {
                i++;                                                                // Increment number of login attempts.

                receiptSuccessful = receiveMessage(ns, receiveBuffer, debug);       // Receives message and saves it in receive buffer.
                if (!receiptSuccessful)                                             // Check if user ended connection.
                {
                    return receiptSuccessful;                                       // Return that user ended connection.
                }
            }
        } while (!success && i < 3);                                                // Allow 3 unsuccessful login attempts.
    }

    else if (strncmp(receiveBuffer, "PASS", 4) == 0)                                // Check if PASS message received from client.
    {
        success = commandPassword(ns, receiveBuffer, password, authroisedLogin, debug); // Handle command.
    }

    else if (strncmp(receiveBuffer, "SYST", 4) == 0)                                // Check if SYST message received from client.
    {
        success = commandSystemInformation(ns, debug);                              // Handle command.
    }

    else if (strncmp(receiveBuffer, "QUIT", 4) == 0)                                // Check if QUIT message received from client.
    {
        success = commandQuit(ns, debug);                                           // Handle command.
    }

    else if (strncmp(receiveBuffer, "PORT", 4) == 0)                                // Check if PORT message received from client.
    {
        success = commandDataPort(ns, sDataActive, receiveBuffer, debug);           // Handle command.
    }

    else if (strncmp(receiveBuffer, "LIST", 4) == 0)                                // Check if LIST message received from client.
    {
        success = commandList(ns, sDataActive, debug, clientId, currentDirectory);  // Handle command.
    }

    else if (strncmp(receiveBuffer, "RETR", 4) == 0)                                // Check if RETR message received from client.
    {
        success = commandRetrieve(ns, sDataActive, receiveBuffer, debug, currentDirectory); // Handle command.
    }

    else if (strncmp(receiveBuffer, "STOR", 4) == 0)                                // Check if STOR message received from client.
    {
        success = commandStore(ns, sDataActive, receiveBuffer, debug, currentDirectory); // Handle command.
    }

    else if (strncmp(receiveBuffer, "CWD", 3) == 0)                                 // Check if CWD message received from client.
    {
        success = commandChangeWorkingDirectory(ns, receiveBuffer, debug, currentDirectory); // Handle command.
    }

    else if (strncmp(receiveBuffer, "DELE", 4) == 0)                                // Check if DELE message received from client.
    {
        success = commandDelete(ns, receiveBuffer, debug);                          // Handle command.
    }

    else if (strncmp(receiveBuffer, "MKD", 3) == 0)                                 // Check if MKD message received from client.
    {
        success = commandMakeDirectory(ns, receiveBuffer, debug, currentDirectory); // Handle command.
    }

    else if (strncmp(receiveBuffer, "RMD", 3) == 0)                                 // Check if RMD message received from client.
    {
        success = commandDeleteDirectory(ns, receiveBuffer, debug);                 // Handle command.
    }

    else if (strncmp(receiveBuffer, "TYPE", 4) == 0)                                // Check if TYPE message received from client.
    {
        success = commandType(ns, receiveBuffer, debug);                            // Handle command.
    }

    else if (strncmp(receiveBuffer, "FEAT", 4) == 0)                                // Check if FEAT message received from client.
    {
        success = commandFeat(ns, debug);                                           // Handle command.
    }
    
    else if (strncmp(receiveBuffer, "OPTS", 4) == 0)                                // Check if OPTS message received from client.
    {
        success = commandOpts(ns, receiveBuffer, debug);                            // Handle command.
    }

    else                                                                            // Command not known.
    {
        success = commandUnknown(ns, debug);                                        // Tell client command not known.
    }

    return success;                                                                 // Return whether reply sent successfully.
}

// Receives message and saves it in receive buffer, returns false if connection ended.
bool receiveMessage(SOCKET &ns, char receiveBuffer[], bool debug)
{
    int i = 0;                                                                      // Index of character of the receive buffer to look at.
    int bytes = 0;                                                                  // Response of receive function.

    bool messageToRead = true;                                                      // If more message to read this is true.

    while (messageToRead)                                                           // Read each byte of message.
    {
        bytes = recv(ns, &receiveBuffer[i], 1, 0);                                  // Inspect receive buffer byte by byte.

        if ((bytes == SOCKET_ERROR) || (bytes == 0))                                // If nothing in receive buffer client has disconnected.
        {
            messageToRead = false;                                                  // No message, end read loop.
        }
        else if (receiveBuffer[i] == '\n')                                          // Message ends on line-feed.
        {
            receiveBuffer[i] = '\0';                                                // Cap string.
            messageToRead = false;                                                  // All of message read, end read loop.
        }
        else if (receiveBuffer[i] != '\r')                                          // Ignore carriage-return.
        {
            i++;                                                                    // Go to next 
        }
    }

    if ((bytes == SOCKET_ERROR) || (bytes == 0))                                    // Client disconnected.
    {
        return false;                                                               // No message, end client message loop.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<--- " << receiveBuffer << "\\r\\n" << std::endl;
    }

    return true;                                                                    // Client still connected.
}

// Client sent USER command, returns false if fails.
bool commandUserName(SOCKET &ns, char receiveBuffer[], char userName[], bool &authroisedLogin, bool debug)
{
    removeCommand(receiveBuffer, userName);                                         // Remove the command from the received command and save as user name.

    std::cout << "User: \"" << userName << "\" attempting to login." << std::endl;

    bool validUserName = isValidUserName(userName);                                 // Check if user name is valid.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    if (validUserName)                                                              // Check validity.
    {
        std::cout << "User name valid. Password required." << std::endl;

        authroisedLogin = true;                                                     // Need authorised password to log in.

        sprintf(sendBuffer, "331 Authorised login requested, please specify the password.\r\n");    // Add message to send buffer.
    }
    else                                                                            // User name invalid
    {
        std::cout << "User name unauthorised. Public access only." << std::endl;

        authroisedLogin = false;                                                    // Don't need authorised password to log in.

        sprintf(sendBuffer, "331 Public login requested, please specify email as password.\r\n");   // Add message to send buffer.
    }

    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Return whether a valid user name was entered.
}

// Returns true if valid user name.
bool isValidUserName(const char userName[])
{
    return !strcmp(userName, "nhreyes");                                            // Only valid user name is nhreyes, for testing purposes.
}

// Client sent PASS command, returns false if fails.
bool commandPassword(SOCKET &ns, char receiveBuffer[], char password[], bool authroisedLogin, bool debug)
{
    removeCommand(receiveBuffer, password);                                         // Get the inputted password.

    bool validPassword = isValidPassword(password, authroisedLogin);                // Check if password is email address.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    if (validPassword)                                                              // Check if valid email address.
    {
        std::cout << "Password valid. User logged in." << std::endl;

        sprintf(sendBuffer, "230 Login successful.\r\n");                           // Add message to send buffer.
    }
    else                                                                            // Invalid email address.
    {
        std::cout << "Password invalid. Login failed." << std::endl;

        sprintf(sendBuffer, "530 Login authentication failed.\r\n");                // Add message to send buffer.
    }

    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return validPassword;                                                           // Return true if connection should end (if login unsuccessful).
}

// Returns true if valid password.
bool isValidPassword(const char password[], bool authroisedLogin)
{
    if (authroisedLogin)                                                            // Check if user name is for authorised user.
    {
        return !strcmp(password, "334");                                            // Only valid password is 334 for user nhreyes, for testing purposes.
    }
    else
    {
        return isEmailAddress(password);                                            // Public users must have email address for password.
    }
}

// Client sent SYST command, returns false if fails.
bool commandSystemInformation(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    std::cout << "System information requested." << std::endl;

    sprintf(sendBuffer,"215 Windows Type: WIN64\r\n");                              // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent QUIT command, returns false if fails.
bool commandQuit(SOCKET &ns, bool debug)
{
    std::cout << "Client has quit the session." << std::endl;

    return false;                                                                   // Return that connection ended.
}

// Client sent PORT command, returns false if fails.
bool commandDataPort(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug)
{
    std::cout << "===================================================" << std::endl;
    std::cout << "\tActive FTP mode, the client is listening..." << std::endl;

    char ipBuffer[40];                                                              // Stores decimal representation of client IP.
    char portBuffer[6];                                                             // Stores decimal representation of client port.

    memset(&ipBuffer, 0, 40);                                                       // Ensure blank.
    memset(&portBuffer, 0, 6);                                                      // Ensure blank.

    bool success = getClientIPPort(ns, receiveBuffer, ipBuffer, portBuffer, debug); // Get the IP address and port of the client.
    if (!success)                                                                   // Did not succeed.
    {
        return sendArgumentSyntaxError(ns, debug);                                  // Send error to client.
    }

    struct addrinfo *result = NULL;                                                 // Structure for client's address information.

    success = getClientAddressInfoActive(ns, result, ipBuffer, portBuffer, debug);  // Get the address information for the connection.
    if (!success)                                                                   // Did not succeed.
    {
        freeaddrinfo(result);                                                       // Free memory.
        
        return sendFailedActiveConnection(ns, debug);                               // Send error to client.
    }

    success = allocateDataTransferSocket(sDataActive, result, debug);               // Allocate socket for data transfer.
    if (!success)
    {
        closesocket(sDataActive);                                                   // Close socket.
        freeaddrinfo(result);                                                       // Free memory.
        
        return sendFailedActiveConnection(ns, debug);                               // Send error to client.
    }

    success = connectDataTransferSocket(sDataActive, result, debug);                // Connect to data transfer socket.
    if (!success)
    {
        closesocket(sDataActive);                                                   // Close socket.
        freeaddrinfo(result);                                                       // Free memory.

        return sendFailedActiveConnection(ns, debug);                               // Send error to client.
    }

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"200 PORT Command successful.\r\n");                         // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "<<<DEBUG INFO>>>: Connected to client's data connection." << std::endl;
    }

    return success;                                                                 // Connection not ended, command handled.
}

// Gets the client's IP and port number for active connection.
bool getClientIPPort(SOCKET &ns, char receiveBuffer[], char ipBuffer[], char portBuffer[], bool debug)
{
    int activePort[2], activeIP[4];                                                 // Stores port and IP for client transfer.

    int scannedItems = sscanf(receiveBuffer, "PORT %d,%d,%d,%d,%d,%d",              // Get port and IP information from client message.
                              &activeIP[0], &activeIP[1], &activeIP[2], &activeIP[3],
                              &activePort[0], &activePort[1]);

    if (scannedItems < 6)                                                           // Check that syntax as expected.
    {
        return sendArgumentSyntaxError(ns, debug);                                  // Failed but don't end connection.
    }

    sprintf(ipBuffer, "%d.%d.%d.%d", activeIP[0], activeIP[1], activeIP[2], activeIP[3]); // Create decimal representation of IP (IPv4 format).

    std::cout << "\tClient's IP is " << ipBuffer << std::endl;                      // IPv4 format client address.

    int portDecimal = activePort[0];                                                // The port number as a decimal.
    portDecimal = portDecimal << 8;                                                 // First number is most significant 8 bits.
    portDecimal += activePort[1];                                                   // Second number is least significant 8 bits.

    sprintf(portBuffer, "%hu", portDecimal);                                        // Copy port decimal into port buffer.

    std::cout << "\tClient's Port is " << portBuffer << std::endl;
    printf("===================================================\n");

    return true;
}

// Sends the client a message to say data connection failed.
bool sendArgumentSyntaxError(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"501 Syntax error in arguments.\r\n");                       // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;
}

// Gets the servers address information based on arguments.
bool getClientAddressInfoActive(SOCKET &ns, struct addrinfo * &result, const char ipBuffer[], const char portBuffer[], bool debug)
{
    struct addrinfo hints;                                                          // Create address info hint structure.
    memset(&hints, 0, sizeof(struct addrinfo));                                     // Clean up the structure.

    hints.ai_family = AF_INET;                                                      // Set address family as internet (IPv4).
    hints.ai_socktype = SOCK_STREAM;                                                // Set socket type as socket stream (for TCP).
    hints.ai_protocol = 0;                                                          // Set protocol as TCP.

    int iResult;                                                                    // Create return value.
    iResult = getaddrinfo(ipBuffer, portBuffer, &hints, &result);                   // Resolve address information for client connection.

    if (iResult != 0)                                                               // Check for expected execution of getaddrinfo.
    {
        std::cout << "getaddrinfo() for client failed: " << iResult << std::endl;

        return false;                                                               // Failed.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Client address information created." << std::endl;
    }

    return true;                                                                    // Completed without error.
}

// Allocates the socket for data transfer..
bool allocateDataTransferSocket(SOCKET &sDataActive, struct addrinfo *result, bool debug)
{
    sDataActive = socket(result->ai_family, result->ai_socktype, result->ai_protocol);  // Allocate socket.

    if (sDataActive == INVALID_SOCKET)                                              // Check for error in socket allocation.
    {
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;

        return false;                                                               // Failed, return false.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Data transfer socket allocated." << std::endl;
    }

    return true;                                                                    // Completed without error.
}

// Bind data transfer socket to result address.
bool connectDataTransferSocket(SOCKET &sDataActive, struct addrinfo *result, bool debug)
{
    int iResult = connect(sDataActive, result->ai_addr, (int) result->ai_addrlen);  // Connect the data transfer socket to the client's IP address and port number.

    if (iResult == SOCKET_ERROR)                                                    // Check if error occurred in socket connection.
    {
        std::cout << "Active connection failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sDataActive);                                                   // Close socket.

        return false;                                                               // Failed, return false.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Data transfer socket connected." << std::endl;
    }

    return true;                                                                    // Completed without error.
}

// Sends the client a message to say data connection failed.
bool sendFailedActiveConnection(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"425 Something is wrong, can't start active connection.\r\n");   // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;
}

// Client sent LIST command, returns false if fails.
bool commandList(SOCKET &ns, SOCKET &sDataActive, bool debug, unsigned long &clientId, char currentDirectory[])
{
	char tmpDir[FILENAME_SIZE] = { 0 };                                             // Temp file name for clientId
	char* pathTemp = getenv("TEMP");                                                // in Windows environment string <TEMP> MUST HAVE!!

	sprintf(tmpDir, "%s\\%lu_tmpDir.txt", pathTemp, clientId);                      // Path to file where store directories list and files list for sent.

    int successLevel = sendFile(ns, sDataActive, tmpDir, debug, clientId, currentDirectory); // Create and send directory listing.
    if (successLevel != 1)                                                          // Check if direcoty listing sent correctly.
    {
        closesocket(sDataActive);                                                   // Close active connection.

        return successLevel;                                                        // Returns 0 if total failure, -1 if just incorrect syntax.
    }

    closesocket(sDataActive);                                                       // Close active connection.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "226 File transfer complete.\r\n");                         // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Sends specified file to client.
int sendFile(SOCKET &ns, SOCKET &sDataActive, const char fileName[], bool debug, unsigned long clientId, char currentDirectory[])
{
	char tmpDir[FILENAME_SIZE] = { 0 };                                             // Temp file name for sent as LIST
	char tmpDir_DIR[FILENAME_SIZE] = { 0 };                                         // Temp file name with container <Directory> for clientId
	char tmpDir_FILE[FILENAME_SIZE] = { 0 };                                        // Temp file name with container <Files> for clientId
	char tmpDir_dirDirectory[FILENAME_SIZE] = { "dir /A:D /B" };                    // System command for create list <Directory>
	char tmpDir_dirFiles[FILENAME_SIZE] = { "dir /A:-D /-C" };                      // System command for create list <Files>
	char* pathTemp = NULL;

	if (clientId)                                                                   // Check if LIST call.
    {
        std::cout << "Client has requested the directory listing." << std::endl;

		pathTemp = getenv("TEMP");                                                  // in Windows environment string <TEMP> MUST HAVE!!

		sprintf(tmpDir, "%s\\%lu_tmpDir.txt", pathTemp, clientId);
		sprintf(tmpDir_DIR, "%s\\%lu_tmpDir2.txt", pathTemp, clientId);
		sprintf(tmpDir_FILE, "%s\\%lu_tmpDir3.txt", pathTemp, clientId);

		strcat(tmpDir_dirDirectory, " >");
		strcat(tmpDir_dirDirectory, tmpDir_DIR);

		strcat(tmpDir_dirFiles, " >");
		strcat(tmpDir_dirFiles, tmpDir_FILE);

        // Save directory information in temp file.
		executeSystemCommand(tmpDir_dirDirectory, currentDirectory, debug);

		if (debug)
        {
			std::cout << "<<<DEBUG INFO>>>: " << tmpDir_dirDirectory << " " << currentDirectory << std::endl;
        }

		executeSystemCommand(tmpDir_dirFiles, currentDirectory, debug);             // Save file information in temp file.

		if (debug)
        {
			std::cout << "<<<DEBUG INFO>>>: " << tmpDir_dirFiles << " " << currentDirectory << std::endl;
        }

		FILE *fInDIR = fopen(tmpDir, "w");                                          // Open file.

		FILE *fInDirectory = fopen(tmpDir_DIR, "r");                                // Open file.

		char tmpBuffer[BUFFER_SIZE];
		char tmpBufferDir[BUFFER_SIZE];
		bool isFirst = true;

		while (!feof(fInDirectory))                                                 // Scan till end of file.
		{
			memset(&tmpBuffer, 0, BUFFER_SIZE);                                     // Clean buffer.
			if (NULL == fgets(tmpBuffer, BUFFER_SIZE, fInDirectory))                // Read data.
            {
				break;
            }
			killLastCRLF(tmpBuffer);
			memset(&tmpBufferDir, 0, BUFFER_SIZE);
			strcpy(tmpBufferDir, "drw-rw-rw-    1 user       group        512 Oct 15  2024 ");
			if (!g_convertKirillica)
			{
				strcat(tmpBufferDir, tmpBuffer);
			}
			else
			{
				char bufferForNewFileName[FILENAME_SIZE];
				simple_conv(tmpBuffer, strlen(tmpBuffer), bufferForNewFileName, FILENAME_SIZE, false);
				strcat(tmpBufferDir, bufferForNewFileName);
			}
			if (!isFirst)
            {
				fputs("\n", fInDIR);
            }
			else
            {
				isFirst = false;
            }
			fputs(tmpBufferDir, fInDIR);
			if (debug)                                                              // Check if debug on.
            {
				std::cout << tmpBufferDir << std::endl;
            }
			if (ferror(fInDIR))
            {
				break;
            }
		}

		fclose(fInDirectory);                                                       // Close the file.

		FILE *fInFiles = fopen(tmpDir_FILE, "r");                                   // Open file.

		int skipLines = 5;
		while (!feof(fInFiles) && skipLines > 0)
		{
			memset(&tmpBuffer, 0, BUFFER_SIZE);
			if (NULL == fgets(tmpBuffer, BUFFER_SIZE, fInFiles))
            {
				break;
            }
			skipLines--;
		}

		int iDay, iMonths, iYear, iHour, iMinute;
		unsigned long ulFileSize;
		char tmpFileName[FILENAME_SIZE];
		char tmpBufferFile[FILENAME_SIZE];

		while (!feof(fInFiles))                                                     // Scan till end of file.
		{
			memset(&tmpBuffer, 0, BUFFER_SIZE);
			if (NULL == fgets(tmpBuffer, BUFFER_SIZE, fInFiles))
            {
				break;
            }
			if (isNumerical(tmpBuffer[0]))                                          // Parsing the string if there is data
			{
				memset(&tmpFileName, 0, FILENAME_SIZE);
				sscanf(tmpBuffer, "%2d.%2d.%4d  %2d:%2d %17lu %*s", &iDay, &iMonths, &iYear, &iHour, &iMinute, &ulFileSize);
				strcpy(tmpFileName, tmpBuffer + 36);
				killLastCRLF(tmpFileName);
				memset(&tmpBufferFile, 0, FILENAME_SIZE);
				sprintf(tmpBufferFile, "-rw-rw-rw-    1 user       group %10lu %s %2d  %4d ", ulFileSize, g_sMonths[iMonths - 1], iDay, iYear);
                if (!g_convertKirillica)
                {
                    strcat(tmpBufferFile, tmpFileName);
                }
                else
                {
                    char bufferForNewFileName[FILENAME_SIZE];
                    simple_conv(tmpFileName, strlen(tmpFileName), bufferForNewFileName, FILENAME_SIZE, false);
                    strcat(tmpBufferFile, bufferForNewFileName);
                }
                if (!isFirst)
                {
					fputs("\n", fInDIR);
                }
				else
                {
					isFirst = false;
                }
				fputs(tmpBufferFile, fInDIR);
				if (debug)                                                          // Check if debug on.
                {
					std::cout << tmpBufferFile << std::endl;
                }
				if (ferror(fInDIR))
                {
					break;
                }
			}
		}

		fclose(fInFiles);                                                           // Close the file.

		fclose(fInDIR);                                                             // Close the file.
    }
    else
    {
        std::cout << "Client has requested to retrieve the file: \"" << fileName << "\"." << std::endl;
    }

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

	FILE *fIn = NULL;

	if (clientId)                                                                   // Check if LIST call.
    {
		fIn = fopen(fileName, "rb");                                                // Open file for command LIST.
    }
	else
	{
		char fileNameFull[FILENAME_SIZE];                                           // Set up buffer for FULLFILENAME.
		memset(&fileNameFull, 0, FILENAME_SIZE);                                    // Ensure blank.

		strcpy(fileNameFull, currentDirectory);                                     // Add current directory.

		if (strlen(fileNameFull) > 0)                                               // Check if not blank current directory.
        {
			strcat(fileNameFull, "\\");                                             // Add separator.
        }
		strcat(fileNameFull, fileName);                                             // Add file name.

        if (!g_convertKirillica)
        {
            fIn = fopen(fileNameFull, "rb");                                        // Open file.
        }
        else
        {
            char bufferForNewFileName[FILENAME_SIZE];
            simple_conv(fileNameFull, strlen(fileNameFull), bufferForNewFileName, FILENAME_SIZE, true);
            fIn = fopen(bufferForNewFileName, "rb");                                // Open file.
        }
	}

    if (fIn == NULL)                                                                // Check if valid file.
    {
        std::cout << "The file: \"" << fileName << "\" does not exist." << std::endl;

        sprintf(sendBuffer, "550 File name invalid.\r\n");                          // Add message to send buffer.
        int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                    // Send reply to client.

        if (debug)                                                                  // Check if debug on.
        {
            std::cout << "---> " << sendBuffer;
        }

        if (bytes < 0)                                                              // Check if message sent.
        {
            return 0;                                                               // Message not sent, return that connection ended.
        }

        return -1;                                                                  // Failure, but don't end connection.
    }
    else
    {
        sprintf(sendBuffer, "150 Data connection ready. \r\n");                     // Add message to send buffer.
        int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                    // Send reply to client.

        if (debug)                                                                  // Check if debug on.
        {
            std::cout << "---> " << sendBuffer;
        }

        if (bytes < 0)                                                              // Check if message sent.
        {
            fclose(fIn);                                                            // Close the file.

			if (clientId)                                                           // Check if LIST call.
            {
				if (!debug)                                                         // Check if debug off.
				{
					executeSystemCommand(systemCommandDEL, tmpDir, debug);          // Delete temp file.
					executeSystemCommand(systemCommandDEL, tmpDir_DIR, debug);      // Delete temp file.
					executeSystemCommand(systemCommandDEL, tmpDir_FILE, debug);     // Delete temp file.
				}	
            }

            return 0;                                                               // Message not sent, return that connection ended.
        }
    }

    char tempBuffer[BIG_BUFFER_SIZE + 1];                                           // Temporary character buffer.

    while (!feof(fIn))                                                              // Scan till end of file.
    {
		size_t result = fread(tempBuffer, 1, BIG_BUFFER_SIZE, fIn);                 // Get data from file into temp buffer.

		// send buffer to client
		size_t sent = 0;
		while (sent < result)                                                       // While not all sent
		{
			int n = send(sDataActive, tempBuffer + sent, result - sent, 0);         // Send over active connection.

			if (n == -1) {                                                          // Check if ERROR
				fclose(fIn);                                                        // Close the file.

				if (clientId)                                                       // Check if LIST call.
				{
					if (!debug)                                                     // Check if debug off.
					{
						executeSystemCommand(systemCommandDEL, tmpDir, debug);      // Delete temp file.
						executeSystemCommand(systemCommandDEL, tmpDir_DIR, debug);  // Delete temp file.
						executeSystemCommand(systemCommandDEL, tmpDir_FILE, debug); // Delete temp file.
					}
				}

				return 0;                                                           // Message not sent, return that connection ended.
			}

			sent += n;                                                              // Counting the bytes sent
		}
    }

    fclose(fIn);                                                                    // Close the file.

    if (clientId)                                                                   // Check if LIST call.
    {
		if (!debug)                                                                 // Check if debug off.
		{
			executeSystemCommand(systemCommandDEL, tmpDir, debug);                  // Delete temp file.
			executeSystemCommand(systemCommandDEL, tmpDir_DIR, debug);              // Delete temp file.
			executeSystemCommand(systemCommandDEL, tmpDir_FILE, debug);             // Delete temp file.
		}
    }

    std::cout << "File sent successfully."<< std::endl;

    return 1;                                                                       // Return success.
}

// return '0' if not have error.
int executeSystemCommand(const char commandNameWithKeys[], const char fileName[], bool debug)
{
	char executeCommand[FILENAME_SIZE];                                             // Store for the generated command
	memset(&executeCommand, 0, FILENAME_SIZE);                                      // Ensure empty.

	strcpy(executeCommand, commandNameWithKeys);                                    // Set command from parameter 'commandNameWithKeys'

	strcat(executeCommand, " ");                                                    // Add <SPACE> between the command and the parameter

	if (NULL != strchr(fileName, ' '))                                              // Check if <SPACE> in name.
	{
		strcat(executeCommand, "\"");                                               // Add quotation marks.
	}

	strcat(executeCommand, fileName);                                               // Add parameter to command.

	if (NULL != strchr(fileName, ' '))                                              // Check if <SPACE> in name.
	{
		strcat(executeCommand, "\"");                                               // Add quotation marks.
	}

	if (debug)                                                                      // Check if debug on.
	{
		std::cout << "Execute command: " << executeCommand << std::endl;
	}

	return system(executeCommand);                                                  // Execute created system command.
}

// Client sent RETR command, returns false if fails.
bool commandRetrieve(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug, char currentDirectory[])
{
    char fileName[FILENAME_SIZE];                                                   // Stores the name of the file to transfer.
    memset(&fileName, 0, FILENAME_SIZE);                                            // Ensure empty.

    removeCommand(receiveBuffer, fileName);                                         // Get file name from command.

    bool success = sendFile(ns, sDataActive, fileName, debug, 0, currentDirectory); // Send file by name.
    if (!success)                                                                   // Check if file sent correctly.
    {
        closesocket(sDataActive);                                                   // Close active connection.

        return success;
    }

    closesocket(sDataActive);                                                       // Close active connection.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "226 File transfer complete.\r\n");                         // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent STORE command, returns false if fails.
bool commandStore(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug, char currentDirectory[])
{
    char fileName[FILENAME_SIZE];                                                   // Stores the name of the file to transfer.
    memset(&fileName, 0, FILENAME_SIZE);                                            // Ensure empty.

    removeCommand(receiveBuffer, fileName);                                         // Get file name from command.

    bool success = saveFile(ns, sDataActive, fileName, debug, currentDirectory);    // Save the file to drive.
    if (!success)                                                                   // Check if direcoty listing sent correctly.
    {
        closesocket(sDataActive);                                                   // Close active connection.

        return success;
    }

    closesocket(sDataActive);                                                       // Close active connection.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "226 File transfer complete.\r\n");                         // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Sends specified file to client.
bool saveFile(SOCKET &ns, SOCKET &sDataActive, const char fileName[], bool debug, char currentDirectory[])
{
    std::cout << "Client has requested to store the file: \"" << fileName << "\"." << std::endl;

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "150 Data connection ready.\r\n");                          // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

	char fileNameFull[FILENAME_SIZE];                                               // Set up buffer for FULLFILENAME.
	memset(&fileNameFull, 0, FILENAME_SIZE);                                        // Ensure blank.

	strcat(fileNameFull, currentDirectory);                                         // Add current directory.

	if (strlen(fileNameFull) > 0)                                                   // Check if not blank current directory.
	{
		strcat(fileNameFull, "\\");                                                 // Add separator.
	}

	strcat(fileNameFull, fileName);                                                 // Add file name.

    std::ofstream fOut;                                                             // Output file stream.

    if (!g_convertKirillica)
    {
        fOut.open(fileNameFull, std::ofstream::binary);                             // Open file.
    }
    else
    {
        char fileNameFullNorm[FILENAME_SIZE];
        simple_conv(fileNameFull, strlen(fileNameFull), fileNameFullNorm, FILENAME_SIZE, true);
        fOut.open(fileNameFullNorm, std::ofstream::binary);                         // Open file.
    }

    char tempBuffer[BIG_BUFFER_SIZE];                                               // Temporary character buffer.
	int sizeBuffer = 0;
    bool moreFile = true;                                                           // Flag for more file to read.

    while (moreFile)                                                                // Scan till end of file.
    {
        moreFile = receiveFileContents(sDataActive, tempBuffer, sizeBuffer);        // Receive file contents.

		if (sizeBuffer > 0)                                                         // Save only if there is data.
		{
			fOut.write(tempBuffer, sizeBuffer);                                     // Save buffer to file.
		}
    }

    fOut.close();                                                                   // Close the file.

    std::cout << "File saved successfully."<< std::endl;

    return true;                                                                    // Return success.
}

// Receives message and saves it in receive buffer, returns false if connection ended.
bool receiveFileContents(SOCKET &sDataActive, char receiveBuffer[], int &sizeBuffer)
{
    int i = 0;                                                                      // Index of character of the receive buffer to look at.
    int bytes = 0;                                                                  // Response of receive function.

    bool fileToRead = true;                                                         // If more file to read this is true.

    while (fileToRead && i < BIG_BUFFER_SIZE - 1)                                   // Read each byte of file.
    {
        bytes = recv(sDataActive, receiveBuffer + i, BIG_BUFFER_SIZE - 1 - i, 0);   // Inspect receive buffer byte by byte.

        if ((bytes == SOCKET_ERROR) || (bytes == 0))                                // If nothing in receive buffer client has disconnected.
        {
            fileToRead = false;                                                     // No message, end read loop.

			break;                                                                  // To avoid writing an extra character.
        }

        i += bytes;
    }

    sizeBuffer = i;                                                                 // Set how much data was received.

    if ((bytes == SOCKET_ERROR) || (bytes == 0))                                    // Client disconnected.
    {
        return false;                                                               // No message, end client message loop.
    }

    return true;                                                                    // Client still connected.
}

// Client sent CWD command, returns false if connection ended.
bool commandChangeWorkingDirectory(SOCKET &ns, char receiveBuffer[], bool debug, char currentDirectory[FILENAME_SIZE])
{
    removeCommand(receiveBuffer, currentDirectory);                                 // Get directory name from command.

	replaceBackslash(currentDirectory);                                             // Replace '/' to '\' for Windows

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "250 Directory successfully changed.\r\n");                 // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent DELETE command, returns false if connection ended.
bool commandDelete(SOCKET &ns, char receiveBuffer[], bool debug)
{
    char fileName[FILENAME_SIZE];                                                   // Stores the name of the file to delete.
    memset(&fileName, 0, FILENAME_SIZE);                                            // Ensure empty.

    removeCommand(receiveBuffer, fileName, 5);                                      // Get file name from command.

	replaceBackslash(fileName);                                                     // Replace '/' to '\' for Windows

	executeSystemCommand(systemCommandDEL, fileName, debug);

	if (debug)
	{
		std::cout << "<<<DEBUG INFO>>>: " << systemCommandDEL << " " << fileName << std::endl;
	}

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "250 Requested file action okay, completed.\r\n");          // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent MKD command, returns false if connection ended.
bool commandMakeDirectory(SOCKET &ns, char receiveBuffer[], bool debug, char currentDirectory[FILENAME_SIZE])
{
	char directoryName[FILENAME_SIZE];                                              // Stores the name of the directory to create.
    memset(&directoryName, 0, FILENAME_SIZE);                                       // Ensure empty.

    removeCommand(receiveBuffer, directoryName);                                    // Get directory name from command.

	replaceBackslash(directoryName);                                                // Replace '/' to '\' for Windows

	executeSystemCommand(systemCommandMKDIR, directoryName, debug);

	if (debug)                                                                      // Check if debug on.
	{
		std::cout << "<<<DEBUG INFO>>>: " << systemCommandMKDIR << " " << directoryName << std::endl;
	}

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "257 '/%s' directory created\r\n", directoryName);          // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent RMD command, returns false if connection ended.
bool commandDeleteDirectory(SOCKET &ns, char receiveBuffer[], bool debug)
{
	char directoryName[FILENAME_SIZE];                                              // Stores the name of the directory to delete.
    memset(&directoryName, 0, FILENAME_SIZE);                                       // Ensure empty.

    removeCommand(receiveBuffer, directoryName);                                    // Get directory name from command.

	replaceBackslash(directoryName);                                                // Replace '/' to '\' for Windows

	executeSystemCommand(systemCommandRMDIR, directoryName, debug);

	if (debug)
	{
		std::cout << "<<<DEBUG INFO>>>: " << systemCommandRMDIR << " " << directoryName << std::endl;
	}

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "250 Requested file action okay, completed.\r\n");          // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent TYPE command, returns false if connection ended.
bool commandType(SOCKET &ns, char receiveBuffer[], bool debug)
{
	char typeName[BUFFER_SIZE];                                                     // Stores the name of the type.
    memset(&typeName, 0, BUFFER_SIZE);                                              // Ensure empty.

    removeCommand(receiveBuffer, typeName);                                         // Get TYPE name from command.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "200 Type set to %s.\r\n", typeName);                       // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent FEAT command, returns false if fails.
bool commandFeat(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"211-Extensions supported\r\n UTF8\r\n211 end\r\n");         // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent OPTS command, returns false if connection ended.
bool commandOpts(SOCKET &ns, char receiveBuffer[], bool debug)
{
	char optsName[BUFFER_SIZE];
    memset(&optsName, 0, BUFFER_SIZE);                                              // Ensure empty.

    removeCommand(receiveBuffer, optsName);                                         // Get TYPE name from command.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.
    
    if (strncmp(optsName, "UTF8 ON", 8) == 0)
    {
        sprintf(sendBuffer, "200 UTF8 ON.\r\n");                                    // Add message to send buffer.
    }
    else
    {
        sprintf(sendBuffer, "501 Syntax error in parameters or arguments.\r\n");    // Add message to send buffer.
    }

    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent unknown command, returns false if fails.
bool commandUnknown(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"550 unrecognised command.\r\n");                            // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Takes a string with a 4 letter command at beginning and saves an output string with this removed.
void removeCommand(const char inputString[], char outputString[], unsigned int skipCharacters)
{
    unsigned int i = 0;                                                             // Array index.
    unsigned int length = strlen(inputString);                                      // Length of string.

    for (; i + skipCharacters + 1 < length; i++)                                    // Scan over input string.
    {
        outputString[i] = inputString[i + skipCharacters + 1];                      // Copy character to output string.
    }

    outputString[i] = '\0';                                                         // Cap output string.
}

// Check is inputted string is valid email address (only requires an '@' before a '.').
bool isEmailAddress(const char address[])
{
    if (!isAlphabetical(address[0]))                                                // First character must be a-z or A-Z.
    {
        return false;                                                               // Invalid first character.
    }

    int atIndex = -1, dotIndex = -1;                                                // To store the index of @ and . characters.

    unsigned int length = strlen(address);                                          // The length of the address.

    for (unsigned int i = 1; i < length; i++)                                       // Loop over email address.
    {
        const char c = address[i];                                                  // Get the current character from the string.

        if (!isAlphabetical(c) && !isNumerical(c))                                  // Check if not alphanumeric.
        {
            if (c == '@')                                                           // See if character is @ symbol.
            {
                atIndex = i;                                                        // Save index of @ symbol.
            }
            else if (c == '.')                                                      // See if character is . symbol.
            {
                dotIndex = i;                                                       // Save index of . symbol.
            }
            else                                                                    // Invalid character.
            {
                return false;                                                       // Not valid email address.
            }
        }
    }

    return (atIndex != -1 && dotIndex != -1) && (atIndex < dotIndex);               // Return true if both symbols exist and are in correct order.
}

// Returns true if the character is alphabetical.
bool isAlphabetical(const char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// Returns true if the character is a number.
bool isNumerical(const char c)
{
    return (c >= '0' && c <= '9');
}

// Sends client the closing connection method and closes the socket.
void closeClientConnection(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "221 FTP server closed the connection.\r\n");               // Create buffer to send to client.
    send(ns, sendBuffer, strlen(sendBuffer), 0);                                    // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    std::cout << "Disconnected from client." << std::endl;

    closesocket(ns);                                                                // Close socket.
}

// Delete <CR> and <LF> in end of string.
void killLastCRLF(char buffer[])
{
	while(0 < strlen(buffer) && 
	     ('\r' == buffer[strlen(buffer) - 1] ||
		  '\n' == buffer[strlen(buffer) - 1])
		 )
    {
		buffer[strlen(buffer) - 1] = 0;
    }
}

// Replace '/' to '\' for Windows
void replaceBackslash(char buffer[])
{
    size_t i = 0;                                                                   // Array index.
    size_t length = strlen(buffer);                                                 // Length of character buffer.

    for (; i < length; i++)                                                         // Scan over buffer.
    {
		if ('/' == buffer[i])                                                       // Check if right character
        {
			buffer[i] = '\\';                                                       // Set backslash.
        }
    }
}

// Converting kirillic characters between Android and Windows 7
void simple_conv(const char inString[], const int inLen, char outString[], const int outMaxLen, bool tudaSuda)
{
    #define ALL_SYMBOLS_FOR_CONVERT (31 + 31 + 4)

    static const char table_for_convert_Tuda[ALL_SYMBOLS_FOR_CONVERT][4] = {
        // small
        "\xd0\xb9\xE9",
        "\xd1\x86\xF6",
        "\xd1\x83\xF3",
        "\xd0\xba\xEA",
        "\xd0\xb5\xE5",
        "\xd0\xbd\xED",
        "\xd0\xb3\xE3",
        "\xd1\x88\xF8",
        "\xd1\x89\xF9",
        "\xd0\xb7\xE7",
        "\xd1\x85\xF5",
        "\xd1\x84\xF4",
        "\xd1\x8b\xFB",
        "\xd0\xb2\xE2",
        "\xd0\xb0\xE0",
        "\xd0\xbf\xEF",
        "\xd1\x80\xF0",
        "\xd0\xbe\xEE",
        "\xd0\xbb\xEB",
        "\xd0\xb4\xE4",
        "\xd0\xb6\xE6",
        "\xd1\x8d\xFD",
        "\xd1\x8f\xFF",
        "\xd1\x87\xF7",
        "\xd1\x81\xF1",
        "\xd0\xbc\xEC",
        "\xd0\xb8\xE8",
        "\xd1\x82\xF2",
        "\xd1\x8c\xFC",
        "\xd0\xb1\xE1",
        "\xd1\x8e\xFE",
        // big
        "\xd0\x99\xC9",
        "\xd0\xa6\xD6",
        "\xd0\xa3\xD3",
        "\xd0\x9a\xCA",
        "\xd0\x95\xC5",
        "\xd0\x9d\xCD",
        "\xd0\x93\xC3",
        "\xd0\xa8\xD8",
        "\xd0\xa9\xD9",
        "\xd0\x97\xC7",
        "\xd0\xa5\xD5",
        "\xd0\xa4\xD4",
        "\xd0\xab\xDB",
        "\xd0\x92\xC2",
        "\xd0\x90\xC0",
        "\xd0\x9f\xCF",
        "\xd0\xa0\xD0",
        "\xd0\x9e\xCE",
        "\xd0\x9b\xCB",
        "\xd0\x94\xC4",
        "\xd0\x96\xC6",
        "\xd0\xad\xDD",
        "\xd0\xaf\xDF",
        "\xd0\xa7\xD7",
        "\xd0\xa1\xD1",
        "\xd0\x9c\xCC",
        "\xd0\x98\xC8",
        "\xd0\xa2\xD2",
        "\xd0\xac\xDC",
        "\xd0\x91\xC1",
        "\xd0\xae\xDE",

        "\xd0\xaa\xda", // big "b
        "\xd1\x8a\xfa", // small "b
        "\xd0\x81\xa8", // big :E
        "\xd1\x91\xb8"  // small :e
    };

    static const char table_for_convert_Suda[ALL_SYMBOLS_FOR_CONVERT][4] = {
        // small
        "\xd0\xb9\xA9",
        "\xd1\x86\xE6",
        "\xd1\x83\xE3",
        "\xd0\xba\xAA",
        "\xd0\xb5\xA5",
        "\xd0\xbd\xAD",
        "\xd0\xb3\xA3",
        "\xd1\x88\xE8",
        "\xd1\x89\xE9",
        "\xd0\xb7\xA7",
        "\xd1\x85\xE5",
        "\xd1\x84\xE4",
        "\xd1\x8b\xEB",
        "\xd0\xb2\xA2",
        "\xd0\xb0\xA0",
        "\xd0\xbf\xAF",
        "\xd1\x80\xE0",
        "\xd0\xbe\xAE",
        "\xd0\xbb\xAB",
        "\xd0\xb4\xA4",
        "\xd0\xb6\xA6",
        "\xd1\x8d\xED",
        "\xd1\x8f\xEF",
        "\xd1\x87\xE7",
        "\xd1\x81\xE1",
        "\xd0\xbc\xAC",
        "\xd0\xb8\xA8",
        "\xd1\x82\xE2",
        "\xd1\x8c\xEC",
        "\xd0\xb1\xA1",
        "\xd1\x8e\xEE",
        // big
        "\xd0\x99\x89",
        "\xd0\xa6\x96",
        "\xd0\xa3\x93",
        "\xd0\x9a\x8A",
        "\xd0\x95\x85",
        "\xd0\x9d\x8D",
        "\xd0\x93\x83",
        "\xd0\xa8\x98",
        "\xd0\xa9\x99",
        "\xd0\x97\x87",
        "\xd0\xa5\x95",
        "\xd0\xa4\x94",
        "\xd0\xab\x9B",
        "\xd0\x92\x82",
        "\xd0\x90\x80",
        "\xd0\x9f\x8F",
        "\xd0\xa0\x90",
        "\xd0\x9e\x8E",
        "\xd0\x9b\x8B",
        "\xd0\x94\x84",
        "\xd0\x96\x86",
        "\xd0\xad\x9D",
        "\xd0\xaf\x9F",
        "\xd0\xa7\x97",
        "\xd0\xa1\x91",
        "\xd0\x9c\x8C",
        "\xd0\x98\x88",
        "\xd0\xa2\x92",
        "\xd0\xac\x9C",
        "\xd0\x91\x81",
        "\xd0\xae\x9E",

        "\xd0\xaa\xda", // big "b
        "\xd1\x8a\xfa", // small "b
        "\xd0\x81\xa8", // big :E
        "\xd1\x91\xb8"  // small :e
    };

    int pos = 0;

    if (tudaSuda)
    {
        for (int i = 0; i < inLen;)
        {
            if ('\xd0' == inString[i] || '\xd1' == inString[i])
            {
                bool isFound = false;

                for (int q = 0; q < ALL_SYMBOLS_FOR_CONVERT; q++)
                {
                    if (table_for_convert_Tuda[q][0] == inString[i] && table_for_convert_Tuda[q][1] == inString[i + 1])
                    {
                        outString[pos] = table_for_convert_Tuda[q][2];
                        isFound = true;
                        break;
                    }
                }

                if (isFound)
                {
                    pos++;
                    i++;
                }
            }
            else
            {
                outString[pos] = inString[i];
                pos++;
            }

            i++;

            if (pos > outMaxLen)
            {
                outString[outMaxLen - 1] = 0;
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < inLen;)
        {
            bool isFound = false;

            for (int q = 0; q < ALL_SYMBOLS_FOR_CONVERT; q++)
            {
                if (table_for_convert_Suda[q][2] == inString[i])
                {
                    outString[pos] = table_for_convert_Suda[q][0];
                    outString[pos + 1] = table_for_convert_Suda[q][1];
                    isFound = true;
                    break;
                }
            }

            if (isFound)
            {
                pos++;
            }
            else
            {
                outString[pos] = inString[i];
            }

            pos++;
            i++;

            if (pos > outMaxLen)
            {
                outString[outMaxLen - 1] = 0;
                break;
            }
        }
    }
    if (pos < outMaxLen)
    {
        outString[pos] = 0;
    }
}
