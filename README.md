# CPP_FTPServer
Hi, readme please

# Features
FTP active mode only

Some basic FTP commands are:
```
USER
PASS
SYST
QUIT
PORT
LIST
RETR
STOR
CWD
DELE
MKD
RMD
TYPE   only answer, but not really used
FEAT   parameter'UTF8' only for command 'OPTS'
OPTS   only parameter 'UTF8'

```

## Usage
```
server.exe [port] [debug mode] [use kirillica]
   [port]          port for server, default '21'
   [debug mode]    debug mode, default OFF, for ON set 'true'
   [use kirillica] use kirillica, for ON set 'true'

example: server.exe 21 false true
   launch server.exe on port 21, without debugging messages,
   but using Cyrillic in directory and file names
```
