#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_buffer.h"
#include "ap_input_plugin.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_thread_queue.h"
#include "ap_input_thread.h"
#include "ap_http.h"


#ifndef WIN32
#include <unistd.h> // for close()
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> // for getaddrinfo()
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

using namespace ap;

namespace ap {


FXint HttpStatus::type() const {
    if (code>=100 && code<200)
        return HTTP_RESPONSE_INFORMATIONAL;
    else if (code>=200 && code<300)
        return HTTP_RESPONSE_SUCCESS;
    else if (code>=300 && code<400)
        return HTTP_RESPONSE_REDIRECT;
    else if (code>=400 && code<500)
        return HTTP_RESPONSE_CLIENT_ERROR;
    else if (code>=500 && code<600)
        return HTTP_RESPONSE_SERVER_ERROR;
    else
        return HTTP_RESPONSE_FAILED;
    }


HttpResponse::HttpResponse() :
    flags(0),
    content_length(-1),
    chunk_remaining(-1) {
    }

HttpResponse::~HttpResponse() {
    clear();
    }

void HttpResponse::clear() {
    flags=0;
    content_length=-1;
    chunk_remaining=-1;
    clear_headers();
    }


// Read (at most) nbytes from buffer or source
FXival HttpResponse::read(FXchar*ptr,FXival nbytes) {
    FXival nread=0;
    while(nbytes) {
        if (buffer.size()) {
            FXival n = buffer.read(ptr,nbytes);
            nread+=n;
            nbytes-=n;
            ptr+=n;
            }
        else {
            FXival n = readBlock(ptr,nbytes);
            if (n<=0) return (nread>0) ? nread : n;
            nread+=n;
            nbytes-=n;
            ptr+=n;
            }
        }
    return nread;
    }

// Fill buffer with (at most) nbytes
FXival HttpResponse::fill(FXival nbytes) {
    buffer.reserve(nbytes);
    FXival nread = readBlock((FXchar*)buffer.ptr(),nbytes);
    if (nread>0) {
        buffer.wroteBytes(nread);
        }
    return nread;
    }


// Try parsing one HTTP header from buffer. If succesfull, returns
// headers. Mode can either be HEADER_SINGLE_LINE or HEADER_MULTIPLE_LINES,
// depending on whether headers may span multiple lines or not.
FXbool HttpResponse::parse_header(FXString & line,FXuint mode) {
    FXint len=0,i,j,l;
    for (i=0;i<buffer.size()-1;i++){
        if (buffer[i]=='\r' && buffer[i+1]=='\n') {

            // If header cannot span multiple lines, we're done here.
            if (mode==HEADER_SINGLE_LINE) {
                if (i>0) line.assign((FXchar*)buffer.data(),i);
                buffer.readBytes(i+2);
                return true;
                }

            // check if header continues on the next line
            if (len>0) {

                // need more bytes
                if ((i+2)>=buffer.size())
                    return false;

                /// header continues on next line
                if (buffer[i+2]==' ' || buffer[i+2]=='\t')
                    continue;
                }

            /// copy string
            line.length(len);
            for (j=0,l=0;l<len;j++){
                if (buffer[j]=='\r' || buffer[j]=='\n')
                    continue;
                line[l++]=buffer[j];
                }
            buffer.readBytes(i+2);
            return true;
            }
        len++;
        }
    return false;
    }

// Store parsed header into headers.
void HttpResponse::insert_header(const FXString & header) {
    FXString key   = header.before(':').lower();
    FXString value = header.after(':').trim();

    FXString * existing = (FXString*)headers.find(key.text());
    if (existing) {
        (*existing) += ", " + value;
        }
    else {
        FXString * v = new FXString;
        v->adopt(value);
        headers.replace(key.text(),v);
        }
//    fxmessage("%s : %s\n",key.text(),value.text());
    }

void HttpResponse::clear_headers() {
    for (FXint pos=headers.first();pos<=headers.last();pos=headers.next(pos)) {
        FXString * field = (FXString*) headers.data(pos);
        delete field;
        }
    headers.clear();
    }

void HttpResponse::check_headers() {
    FXString * field = NULL;

    field = (FXString*) headers.find("content-length");
    if (field)
        content_length = field->toInt();

    field = (FXString*) headers.find("transfer-encoding");
    if (field && field->contains("chunked") )
        flags|=ChunkedResponse;

    field = (FXString*) headers.find("connection");
    if (field && comparecase(*field,"close")==0)
        flags|=ConnectionClose;
    }




// Read a chunk header and set the chunksize
FXbool HttpResponse::read_chunk_header(FXint & chunksize) {
    FXString header;
    FXchar   clrf[2];

    // We've read a previous chunk
    if (chunksize==0) {
        if (read(clrf,2)!=2 || clrf[0]!='\r' || clrf[1]!='\n') {
           fxmessage("missing line feed: %c%c\n",clrf[0],clrf[2]);
           return false;
           }
        }

    if (read_header(header,HEADER_SINGLE_LINE)) {
        //fxmessage("chunk_header: \"%s\"\n",header.text());
        if (header.scan("%x",&chunksize)==1) {
            //fxmessage("chunk: %d\n",chunksize);
            return true;
            }
        }
    return false;
    }


// Read status line and set status.
FXbool HttpResponse::read_status() {
    FXString header;
    if (read_header(header,HEADER_SINGLE_LINE)) {
        if (header.scan("HTTP/%d.%d %d",&status.major,&status.minor,&status.code)==3){
            fxmessage("Code: %d \nVersion: %d.%d\n",status.code,status.major,status.minor);
            return true;
            }
        }
    return false;
    }

// Read header
FXbool HttpResponse::read_header(FXString & header,FXuint mode) {
    while(!parse_header(header,mode)) {
        if (fill()==-1)
            return false;
        }
    return true;
    }

// Read the full message body non-chunked
FXString HttpResponse::read_body() {
    FXString body;
    if (content_length==0) {
        return FXString::null;
        }
    else if (content_length>0) {
        body.length(content_length);
        FXival n = read(&body[0],content_length);
        if (n<content_length)  // Partial Transfer...
            return FXString::null;
        }
    else {
        const FXint BLOCK=4096;
        FXint size=0;
        FXint pos=0;
        FXival n=0;
        do {
            body.length(body.length()+BLOCK);
            n = read(&body[pos],BLOCK);
            if (n>0) pos+=n;
            }
        while(n==BLOCK);

        if (pos>0)
            body.length(pos);
        }
    return body;
    }

// Read the full message body chunked
FXString HttpResponse::read_body_chunked() {
    FXString body;
    FXint    pos=0;
    FXint    chunksize=-1;
    FXchar   clrf[2];
    if (read_chunk_header(chunksize)) {
        while(chunksize) {
            body.length(body.length()+chunksize);
            FXival n = read(&body[pos],chunksize);
            if (n<chunksize) return FXString::null;
            pos+=chunksize;
            chunksize=0;
            if (!read_chunk_header(chunksize)) {
                fxmessage("reading next chunk failed\n");
                return FXString::null;
                }
            }

        // Possible... there may be a trailer
        ///fxmessage("now trying to read trailers...\n");
        FXString header;
        while(read_header(header,HEADER_MULTIPLE_LINES)) {
            // empty header indicates end of headers
            if (header.empty())
                break;
            insert_header(header);
            }
        }
    return body;
    }




// Parse the response status and headers. Returns true if succesful
FXint HttpResponse::response() {
    FXString header;
    if (read_status()) {
        while(read_header(header,HEADER_MULTIPLE_LINES)) {
            // empty header indicates end of headers
            if (header.empty()) {
                check_headers();
                return status.type();
                }
            insert_header(header);
            }
        }
    return HTTP_RESPONSE_FAILED;
    }

// Read the full message body into string
FXString HttpResponse::body() {
    if (flags&ChunkedResponse)
        return read_body_chunked();
    else
        return read_body();
    }

FXival HttpResponse::read_body_chunked(void * ptr,FXival len) {
    FXival n,nread=0;
    FXchar * data = (FXchar*)ptr;

    while(len){
        if (chunk_remaining<=0) {

            if (!read_chunk_header(chunk_remaining))
                return -1;

            if (chunk_remaining==0) {
                FXString header;
                while(read_header(header,HEADER_MULTIPLE_LINES)) {
                    // empty header indicates end of headers
                    if (header.empty())
                        break;
                    insert_header(header);
                    }
                return nread;
                }
            }
        n = read(data,FXMIN(len,chunk_remaining));
        if (n<=0) return nread;
        data+=n;
        len-=n;
        nread+=n;
        chunk_remaining-=n;
        }
    return nread;
    }

FXival HttpResponse::read_body(void*ptr,FXival len) {
    if (content_length >= 0) {
        //return read(ptr,FXMIN(content_length_remaining,len);
        return 0;
        }
    else {
        return read((FXchar*)ptr,len);
        }
    }


FXival HttpResponse::readBody(void * ptr,FXival len) {
    if (flags&ChunkedResponse)
        return read_body_chunked(ptr,len);
    else
        return read_body(ptr,len);
    }


void HttpResponse::discard() {
    if (!(flags&ConnectionClose)) {
        FXchar buffer[1024];
        while(readBody(buffer,1024)==1024) ;
        }
    clear();
    }

FXString HttpResponse::getHeader(const FXString & key) const {
    FXString * value = (FXString*)headers.find(key.text());
    if (value)
        return (*value);
    else
        return FXString::null;
    }


























HttpClient::HttpClient(const FXString & hostname,FXint port) : device(BadHandle),servername(hostname),serverport(port) {
    }

void HttpClient::setServer(const FXString & hostname,FXint port) {
    if (hostname==servername && port==serverport) {
        discard();
        return;
        }
    else {
        close();
        servername = hostname;
        serverport = port;
        }
    }

HttpClient::~HttpClient() {
    close();
    }

void HttpClient::close() {
    if (device!=BadHandle) {
        shutdown(device,SHUT_RDWR);
        ::close(device);
        device=BadHandle;
        }
    }

void HttpClient::discard() {
    if (flags&ConnectionClose)
        close();
    else
        HttpResponse::discard();
    }


FXbool HttpClient::open_server() {
    struct addrinfo   hints;
    struct addrinfo * list=NULL;
    struct addrinfo * item=NULL;

    memset(&hints,0,sizeof(struct addrinfo));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags|=(AI_NUMERICSERV|AI_ADDRCONFIG);

    FXint result=getaddrinfo(servername.text(),FXString::value(serverport).text(),&hints,&list);
    if (result)
        return false;

    for (item=list;item;item=item->ai_next){

        device = socket(item->ai_family,item->ai_socktype,item->ai_protocol);
        if (device == BadHandle)
            continue;

        if (connect(device,item->ai_addr,item->ai_addrlen)==0){
            freeaddrinfo(list);
            return true;
            }

        ::close(device);
        device=BadHandle;
        }

    if (list)
        freeaddrinfo(list);
    return false;
    }


FXival HttpClient::writeBlock(const void * data,FXival count) {
    FXival nwritten=-1;
    do{
      nwritten=::write(device,data,count);
      }
    while(nwritten<0 && errno==EINTR);
    return nwritten;
    }

FXival HttpClient::readBlock(void * data,FXival count) {
    FXival nread=-1;
    do{
      nread=::read(device,data,count);
      }
    while(nread<0 && errno==EINTR);
    return nread;
    }


FXbool HttpClient::send(const FXchar * data,FXint len) {
   do {
        FXival n = writeBlock(data,len);
        if (n<=0) return false;
        data += n;
        len -= n;
        }
    while(len);
    return true;
    }


FXbool HttpClient::request(const FXString & cmd) {
    if (device==BadHandle && !open_server())
        return false;
    else
        return send(cmd.text(),cmd.length());
    }


HttpClient * HttpClient::getUrl(const FXString & url) {
    FXString host  = FXURL::host(url);
    FXString path  = FXURL::host(url);
    FXString query = FXURL::query(url);
    FXint    port  = FXURL::port(url);
    if (port==0) port=80;
    if (!query.empty())
        path+="?"+query;


    FXString cmd;

    HttpClient * client = new HttpClient(host,port);
    do {
        cmd = "GET " + path + " HTTP/1.1\r\nHost: " + host +  "\r\n\r\n";
        if (!client->request(cmd))
            goto failed;

        switch(client->response()) {
            case HTTP_RESPONSE_FAILED   : goto failed; break;
            case HTTP_RESPONSE_REDIRECT :
                {
                    FXString location = client->getHeader("location");
                    fxmessage("redirect: %s\n",location.text());
                    host  = FXURL::host(location);
                    path  = FXURL::path(location);
                    query = FXURL::query(location);
                    port  = FXURL::port(location);
                    if (!query.empty())
                        path+="?"+query;
                    if (port==0) port=80;

                    client->setServer(host,port);
                    continue;

                } break;

            default: break;
            }
        return client;
        }
    while(1);

failed:
    delete client;
    return NULL;
    }



}





