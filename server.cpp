#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <string.h>
#include <iostream>
#include "threadpool/ThreadPool.h"
#define PORT 5000

void respond (int sock);
std::string ROOT;

int main( int argc, char *argv[] ) {
  
  int sockfd, newsockfd, portno = PORT;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  clilen = sizeof(cli_addr);
  ROOT = getenv("PWD");

  /* First call to socket() function */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);


  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }

  // port reusable
  int tr = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }

  /* Initialize socket structure */
  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* DONE : Now bind the host address using bind() call.*/
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))== -1){
      perror("binderr");
      exit(1);
  }

  /* DONE : listen on socket you created */
  if(listen(sockfd,1000) == -1){
      perror("listenerr");
      exit(1);
  }


  printf("Server is running on port %d\n", portno);

  socklen_t servlen = sizeof(serv_addr);
  ThreadPool threadpool(10);
  while (1) {
    /* DONE : accept connection */
      newsockfd = accept(sockfd, (struct sockaddr *) &serv_addr, &servlen);
      if(newsockfd == -1){
        perror("accepterr");
        exit(1);
      }
    // DONE : implement processing. there are three ways - single threaded, multi threaded, thread pooled
    threadpool.enqueue([](int sock){respond(sock);}, newsockfd);
    respond(newsockfd);
  }

  return 0;
}

void respond(int sock) {
  int n;
  char buffer[9999];
  char abs_path[256];

  std::ifstream file;
  std::stringstream text;
  std::stringstream stream;
  
  bzero(buffer,9999);
  n = recv(sock,buffer,9999, 0);
  if (n < 0) {
    printf("recv() error\n");
    return;
  } else if (n == 0) {
    printf("Client disconnected unexpectedly\n");
    return;
  } else {
    char* request = strtok(buffer, "\r\n\r\n"); 
    std::string method(request);

  //  std::cout<<(method)<<"_______";
    printf(request);
   
    if (method.substr(0,3)!="GET") {
      shutdown(sock, SHUT_RDWR);
      close(sock);
    }

    std::string filename = " ";
    std::string req = method.substr(4,method.length()-4);
    if(req.find(" ")!= std::string::npos){
        filename = req.substr(0, req.find(" "));
    }

    if (filename == "/") {
        filename = "/index.html";
    }

    if (filename.find(".jpg") == std::string::npos) {

        file.open(ROOT + filename.c_str());
        if(file){
            text << file.rdbuf();
            file.close();
        }else{
            std::cout<<" File not found ";
            shutdown(sock, SHUT_RDWR);
            close(sock);
        }
        
        stream<<"HTTP/1.1 200 OK\r\n"
            <<"Content-Type: text/html; charset=UTF-8\r\n"
            <<"Content-Encoding: UTF-8\r\n"
            <<"Content-Length: "<<text.str().length()
            <<"\r\n\r\n"
            <<text.str();
            send(sock, stream.str().c_str(), stream.str().length(),0);

        stream.str(std::string());
        text.str(std::string());
    }
    else{

        file.open(ROOT + filename.c_str());

        if ( file ){
            text << file.rdbuf();
            file.close();
        }else{
            std::cout<<" File not found ";
            shutdown(sock, SHUT_RDWR);
            close(sock);
        }

        stream<<"HTTP/1.1 200 OK\r\n"
            <<"Content-Type: image/jpeg\r\n"
            <<"Connection: keep-alive\r\n"
            <<"Content-Length: "<<text.str().length()
            <<"\r\n\r\n"
            <<text.str();
            send(sock, stream.str().c_str(), stream.str().length(),0);

        stream.str(std::string());
        text.str(std::string());
    
    }
    // DONE : parse received message from client
    // make proper response and send it to client
    

  }

  shutdown(sock, SHUT_RDWR);
  close(sock);

}
