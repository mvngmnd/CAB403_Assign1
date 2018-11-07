# CAB403_Assign1

To launch the project, download the repository and ensure the `pthreads` library is installed on your system.

The program comes with a make file, which is simply run by running make in the directory. This will output two files, with the following run instructions:

#### Client
- To run the client requires two parameters, the IP on which the server is
running, and the port on which it is running. This IP must be a local IP and
apart of the current subnet. `localhost` may be used instead, if necessary.
- Example `./client 192.168.0.2 1901`

#### Server
- To run the server requires one parameter, the port on which to run the
server. If this parameter is not supplied, the default port will be used, which is currently set to 1901. This port will be required to be given to all prospective users so that they can connect to the server.
- Example `./server 1588`
