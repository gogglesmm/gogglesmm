/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2013-2021 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "ap_defs.h"
#include "ap_socket.h"
#include "ap_input_plugin.h"
#include "ap_utils.h"

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <poll.h>
#include <unistd.h> // for close()
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>
#ifndef OPENSSL_THREADS
#error "gap: requires openssl with threads support"
#endif
#elif defined(HAVE_GNUTLS)
#include <gnutls/gnutls.h>
#elif defined(HAVE_GCRYPT)
#include "gcrypt.h"
#endif

using namespace ap;

namespace ap {

#ifdef HAVE_OPENSSL
static FXMutex * ssl_locks = nullptr;
static SSL_CTX * ssl_context = nullptr;

#if OPENSSL_VERSION_NUMBER < 0x10100000
static void ap_ssl_locking_callback(int mode, int type, const char */*file*/, int /*line*/) {
  //GM_DEBUG_PRINT("ssl %s at %s:%d\n",(mode&CRYPTO_LOCK) ? "lock" : "unlock",file,line);
  if (mode&CRYPTO_LOCK)
    ssl_locks[type].lock();
  else
    ssl_locks[type].unlock();
  }


static void ap_ssl_threadid_callback(CRYPTO_THREADID *tid) {
#if defined(_WIN32)
  CRYPTO_THREADID_set_pointer(tid,FXThread::current());
#else
  CRYPTO_THREADID_set_numeric(tid,FXThread::current());
#endif
  }

#endif

#elif defined(HAVE_GNUTLS)
static gnutls_certificate_credentials_t ssl_credentials = nullptr;
#endif


FXbool ap_init_crypto() {
#ifdef HAVE_OPENSSL
  if (ssl_locks == nullptr) {
    ssl_locks = new FXMutex[CRYPTO_num_locks()];
#if OPENSSL_VERSION_NUMBER < 0x10100000
    CRYPTO_THREADID_set_callback(ap_ssl_threadid_callback);
    CRYPTO_set_locking_callback(ap_ssl_locking_callback);
#endif

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    ssl_context = SSL_CTX_new(SSLv23_client_method());
    SSL_CTX_set_options(ssl_context,SSL_OP_NO_SSLv3|SSL_OP_NO_SSLv2);
    SSL_CTX_set_default_verify_paths(ssl_context);

    }
#elif defined(HAVE_GNUTLS)
  if (ssl_credentials == nullptr) {
    gnutls_certificate_allocate_credentials(&ssl_credentials);
    gnutls_certificate_set_x509_system_trust(ssl_credentials);
    }
#elif defined(HAVE_GCRYPT)
  if (!gcry_check_version(GCRYPT_VERSION)) {
    fxwarning("gap: libgcrypt version mismatch");
    return false;
    }
  gcry_control(GCRYCTL_DISABLE_SECMEM,0);
  gcry_control(GCRYCTL_INITIALIZATION_FINISHED,0);
#endif
  return true;
  }


void ap_free_crypto() {
#ifdef HAVE_OPENSSL
  if (ssl_locks) {

    SSL_CTX_free(ssl_context);
    ssl_context = nullptr;

    EVP_cleanup();

#if OPENSSL_VERSION_NUMBER < 0x10100000
    CRYPTO_set_locking_callback(nullptr);
    CRYPTO_THREADID_set_callback(nullptr);
#endif

    delete [] ssl_locks;
    ssl_locks = nullptr;
    }
#elif defined(HAVE_GNUTLS)
  if (ssl_credentials == nullptr) {
    gnutls_certificate_free_credentials(ssl_credentials);
    ssl_credentials = nullptr;
    }
#endif
  }


FXbool Socket::setReceiveTimeout(FXTime time) {
#ifdef _WIN32
  FXuint value = time / NANOSECONDS_PER_MILLISECOND;
  if (setsockopt(sockethandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&value, sizeof(FXuint))!=0)
    return false;
#else
  struct timeval tv;
  tv.tv_sec  = time / NANOSECONDS_PER_SECOND;
  tv.tv_usec = (time % NANOSECONDS_PER_SECOND) / NANOSECONDS_PER_MICROSECOND;
  if (setsockopt(device, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval))!=0)
    return false;
#endif
  return true;
  }


FXbool Socket::setSendTimeout(FXTime time) {
#ifdef _WIN32
  FXuint value = time / NANOSECONDS_PER_MILLISECOND;
  if (setsockopt(sockethandle, SOL_SOCKET, SO_SNDTIMEO, (char*)&value, sizeof(FXuint))!=0)
    return false;
#else
  struct timeval tv;
  tv.tv_sec  = time / NANOSECONDS_PER_SECOND;
  tv.tv_usec = (time % NANOSECONDS_PER_SECOND) / NANOSECONDS_PER_MICROSECOND;
  if (setsockopt(device,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval))!=0)
    return false;
#endif
  return true;
  }


FXint Socket::getError() const {
#ifndef _WIN32
  int value = 0;
  socklen_t length=sizeof(value);
  if (getsockopt(device,SOL_SOCKET,SO_ERROR,&value,&length)==0)
    return value;
  else
    return -1;
#endif
  }


#ifdef _WIN32
// Return true if open
FXbool Socket::isOpen() const {
  return sockethandle!=0;
  }
#endif


FXbool Socket::close() {
#ifdef _WIN32
  if (isOpen()) {
    CloseSocket(sockethandle);
    sockethandle=0;
    return FXIODevice::close();
    }
#else
  if (isOpen()) {
    return FXIODevice::close();
    }
#endif
  return true;
  }


FXint Socket::eof() {
  return (access&EndOfStream) ? 1 : 0;
  }


FXbool Socket::shutdown() {
  GM_DEBUG_PRINT("[socket] shutdown()\n");
#ifndef _WIN32
  access&=~FXIO::ReadWrite;
  return ::shutdown(device,SHUT_RDWR)==0;
#endif
  }


FXbool Socket::create(FXint domain,FXint type,FXint protocol,FXuint mode) {
#ifdef _WIN32
  sockethandle = socket(domain,type,protocol);
  if (sockethandle==INVALID_SOCKET)
    return false;

  if (mode&FXIO::NonBlocking) {
    u_long blocking = 1;
    if (ioctlsocket(sockethandle, FIONBIO, &blocking)!=0){
      CloseSocket(sockethandle);
      sockethandle==INVALID_SOCKET;
      return false;
      }
    access|=FXIO::NonBlocking;
    }

  device = CreateEvent();
  if (device==BadHandle) {
    CloseSocket(sockethandle);
    return false;
    }
#else
  int options = 0;

#ifdef SOCK_CLOEXEC
  options|=SOCK_CLOEXEC;
#endif

#ifdef SOCK_NONBLOCK
  if (mode&FXIO::NonBlocking){
    access|=FXIO::NonBlocking;
    options|=SOCK_NONBLOCK;
    }
#endif
  device = socket(domain,type|options,protocol);
  if (device==BadHandle)
    return false;

#ifndef SOCK_CLOEXEC
  if (!ap_set_closeonexec(device)){
    ::close(device);
    device=BadHandle;
    return false;
    }
#endif

#ifndef SOCK_NONBLOCK
  if (mode&FXIO::NonBlocking){
    access|=FXIO::NonBlocking;
    if (!ap_set_nonblocking(device)){
      ::close(device);
      device=BadHandle;
      return false;
      }
    }
#endif
#endif
  access|=OwnHandle;
  return true;
  }


// Connect to address
FXint Socket::connect(const struct sockaddr * address,FXint address_length) {
#ifdef _WIN32

  if (::connect(sockethandle,address,address_length)==0) {
    access|=ReadWrite;
    pointer=0;
    return 0;
    }

  if (WSAGetLastError()==WSAEWOULDBLOCK) {
    pointer=0;
    return FXIO::Again;
    }

#else

  // Connect
  if (::connect(device,address,address_length)==0) {
    access|=FXIO::ReadWrite;
    pointer=0;
    return 0;
    }

  // Handle asynchronous completion or error
  switch(errno) {
    case EINTR      :
    case EINPROGRESS:
    case EWOULDBLOCK:
      {
        switch(wait(WaitMode::Connect)) {
          case WaitEvent::Input:
            if (getError()==0) {
              access|=FXIO::ReadWrite;
              pointer=0;
              return 0;
              }
            break;
          case WaitEvent::Signal: return 1; break;
          default:break;
          }
      }
    default: break;
    }
#endif
  return FXIO::Error;
  }


FXival Socket::writeBlock(const void* ptr,FXival count){
  if(__likely(device!=BadHandle) && __likely(access&WriteOnly)){
#ifdef _WIN32
#else
    FXival nwrote;
x:  nwrote=::send(device,ptr,count,MSG_NOSIGNAL);
    if(__unlikely(nwrote<0)){
      switch(errno) {
        case EINTR:
          goto x;
          break;
        case EAGAIN:
#if EAGAIN!=EWOULDBLOCK
        case EWOULDBLOCK:
#endif
          if ((access&FXIO::NonBlocking) && wait(WaitMode::Write)==WaitEvent::Input)
            goto x;

          // fallthrough - intentional no break
        default:
          access|=EndOfStream;
          return FXIO::Error;
          break;
        }
      }
    if (nwrote==0 && count>0)
      access|=EndOfStream;
#endif
    return nwrote;
    }
  return FXIO::Error;
  }


FXival Socket::readBlock(void* ptr,FXival count){
  if(__likely(device!=BadHandle) && __likely(access&ReadOnly)){
#ifdef _WIN32
#else
    FXival nwrote;
x:  nwrote=::recv(device,ptr,count,MSG_NOSIGNAL);
    if(__unlikely(nwrote<0)){
      switch(errno) {
        case EINTR:
          goto x;
          break;
        case EAGAIN:
#if EAGAIN!=EWOULDBLOCK
        case EWOULDBLOCK:
#endif
          if ((access&FXIO::NonBlocking) && wait(WaitMode::Read)==WaitEvent::Input)
            goto x;

          // fallthrough - intentional no break
        default:
          access|=EndOfStream;
          return FXIO::Error;
          break;
        }
      }
    if (nwrote==0 && count>0)
      access|=EndOfStream;
#endif
    return nwrote;
    }
  return FXIO::Error;
  }


WaitEvent Socket::wait(WaitMode mode) {
#if defined(_WIN32)
  if (mode==WaitRead)
    WSAEventSelect(sockethandle,device,FD_READ);
  else if (mode==WaitWrite)
    WSAEventSelect(sockethandle,device,FD_WRITE);
  else if (mode==WaitConnect)
    WSAEventSelect(sockethandle,device,FD_CONNECT);
  WaitForSingleObject(device,INFINITE);
  return WaitEvent::Input;
#else
  FXint n;
  struct pollfd handles;
  handles.fd     = device;
  handles.events = (mode==WaitMode::Read) ? POLLIN : POLLOUT;
x:n=poll(&handles,1,-1);
  if (__unlikely(n<0)) {
    if (errno==EAGAIN || errno==EINTR)
      goto x;
    return WaitEvent::Error;
    }
  return WaitEvent::Input;
#endif
  }



ThreadSocket::ThreadSocket(IOContext * ctx) : context(ctx) {
  }


WaitEvent ThreadSocket::wait(WaitMode mode) {
#ifdef _WIN32
  if (mode==WaitRead)
    WSAEventSelect(sockethandle,device,FD_READ);
  else if (mode==WaitWrite)
    WSAEventSelect(sockethandle,device,FD_WRITE);
  else if (mode==WaitConnect)
    WSAEventSelect(sockethandle,device,FD_CONNECT);
#endif
  WaitEvent event;
  do {
    event = context->signal().wait(device,mode,10_s);
    if (event==WaitEvent::Input) {
      return event;
      }
    else if (event==WaitEvent::Signal) {
      if (context->aborted()) return WaitEvent::Signal;
      }
    }
  while(event==WaitEvent::Signal);
  return event;
  }

#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)

SecureSocket::SecureSocket() {
  }


FXbool SecureSocket::create(FXint domain,FXint type,FXint protocol,FXuint mode) {
  if (Socket::create(domain,type,protocol,mode)) {

#if defined(HAVE_OPENSSL)

    GM_DEBUG_PRINT("[ssl] using openssl\n");

    // Create BIO for socket
    BIO * bio = BIO_new_socket(device, BIO_NOCLOSE);
    if (bio==nullptr) {
      close();
      return false;
      }

    // Create SSL Object
    ssl = SSL_new(ssl_context);
    if (ssl == nullptr) {
      BIO_free(bio);
      close();
      return false;
      }

    SSL_set_bio(ssl,bio,bio);

#elif defined(HAVE_GNUTLS)

    GM_DEBUG_PRINT("[ssl] using gnutls\n");

    if (gnutls_init(&session, GNUTLS_CLIENT)!=GNUTLS_E_SUCCESS)
      return false;

    if (gnutls_set_default_priority(session)!=GNUTLS_E_SUCCESS)
      return false;

    if (gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, ssl_credentials)!=GNUTLS_E_SUCCESS)
      return false;

    gnutls_session_set_verify_cert(session, nullptr, 0);
    gnutls_transport_set_int(session,device);
#endif

    return true;
    }
  GM_DEBUG_PRINT("[ssl] failed to create socket\n");
  return false;
  }

#if defined(HAVE_GNUTLS)
FXint SecureSocket::handshake() {
  FXint status;

x:status = gnutls_handshake(session);
  if (status == GNUTLS_E_SUCCESS) {
    return 0;
    }
  else if (status == GNUTLS_E_INTERRUPTED || status == GNUTLS_E_AGAIN) {

    WaitMode mode;
    WaitEvent event;

    if (gnutls_record_get_direction(session)==0)
      mode = WaitMode::Read;
    else
      mode = WaitMode::Write;

    if ((event=wait(mode))==WaitEvent::Input)
      goto x;

    return (event==WaitEvent::Signal) ? 1 : FXIO::Error;
    }
  else if (!gnutls_error_is_fatal(status)){
    goto x; // try again
    }
  GM_DEBUG_PRINT("[ssl] handshake failed with code %d\n",status);
  return FXIO::Error;
  }
#endif



FXbool SecureSocket::shutdown() {
  GM_DEBUG_PRINT("[securesocket] shutdown()\n");
  FXint status;
#if defined(HAVE_OPENSSL)
x:status = SSL_shutdown(ssl);
  if (status == 1)
    return Socket::shutdown();
  else if (status == 0)
    return Socket::shutdown();
    //goto x; // wait for reply using SSL_shutdown
  else if (status < 0) {

    WaitMode  mode;
    WaitEvent event;

    switch(SSL_get_error(ssl,status)) {
      case SSL_ERROR_WANT_READ :
        mode = WaitMode::Read;
        break;
      case SSL_ERROR_WANT_WRITE:
        mode = WaitMode::Write;
        break;
      default:
        Socket::shutdown();
        return false;
      }

    // Wait for input
    do {
      event=wait(mode);
      if (event==WaitEvent::Input) goto x;
      }
    while(event==WaitEvent::Signal);
    }
#elif defined(HAVE_GNUTLS)
x:status = gnutls_bye(session,GNUTLS_SHUT_WR);
  if (status == GNUTLS_E_SUCCESS) {
    return Socket::shutdown();
    }
  else if (status==GNUTLS_E_INTERRUPTED || status == GNUTLS_E_AGAIN) {

    WaitMode mode;
    WaitEvent event;

    if (gnutls_record_get_direction(session)==0)
      mode = WaitMode::Read;
    else
      mode = WaitMode::Write;

    do {
      event=wait(mode);
      if (event==WaitEvent::Input) goto x;
      }
    while(event==WaitEvent::Signal);
    }
#endif
  Socket::shutdown();
  return false;
  }


FXbool SecureSocket::close() {
#if defined(HAVE_OPENSSL)
  if (isOpen()) {
    if (ssl) {
      SSL_free(ssl);
      ssl=nullptr;
      }
    return Socket::close();
    }
#elif defined(HAVE_GNUTLS)
  if (isOpen()) {
    if (session) {
      gnutls_deinit(session);
      session=nullptr;
      }
    return Socket::close();
    }
#endif
  return true;
  }



// Connect to address
FXint SecureSocket::connect(const struct sockaddr * address,FXint address_length) {

  // Establish Connection
  FXint status = Socket::connect(address,address_length);
  if (status<0) return status;

#if defined(HAVE_OPENSSL)
  // Negotiate SSL
x:status = SSL_connect(ssl);
  if (status == 1) {

    // Check for peer certificate
    X509 * certificate = SSL_get_peer_certificate(ssl);
    if (certificate == nullptr) {
      GM_DEBUG_PRINT("[ssl] no peer certificate\n");
      close();
      return FXIO::Error;
      }
    X509_free(certificate);

    // Verify certificate
    status = SSL_get_verify_result(ssl);
    if (status!=X509_V_OK) {

      // todo: have some way to handle self-signed certificates
      switch(status) {
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
          GM_DEBUG_PRINT("[ssl] self-signed certificate\n");
          break;
        default: GM_DEBUG_PRINT("[ssl] verify certificate failed. code %d\n",status); break;
        }
      close();
      return FXIO::Error;
      }
    return 0;
    }
  else if (status < 0) {
    WaitMode mode;
    switch(SSL_get_error(ssl,status)) {
      case SSL_ERROR_WANT_READ :
        mode = WaitMode::Read;
        break;
      case SSL_ERROR_WANT_WRITE:
        mode = WaitMode::Write;
        break;
      default:
        close();
        return FXIO::Error;
      }
    if (wait(mode)==WaitEvent::Input)
      goto x;
    }
#elif defined(HAVE_GNUTLS)
  status = handshake();
  if (status!=0) close();
  return status;
#endif
  // Clean shutdown or error
  close();
  return FXIO::Error;
  }




// Read block of bytes, returning number of bytes read
FXival SecureSocket::readBlock(void* data,FXival count) {
  if(__likely(device!=BadHandle) && __likely(access&ReadOnly)){
#if defined(HAVE_OPENSSL)
    FXival n;
x:  n=SSL_read(ssl,data,count);
    if (__unlikely(n<0)) {
      WaitMode mode;
      switch(SSL_get_error(ssl,n)) {
        case SSL_ERROR_WANT_READ :
          mode = WaitMode::Read;
          break;
        case SSL_ERROR_WANT_WRITE:
          mode = WaitMode::Write;
          break;
        default:
          close();
          return FXIO::Error;
        }

      if ((access&FXIO::NonBlocking) && wait(mode)==WaitEvent::Input) {
        goto x;
        }
      return FXIO::Error;
      }

    if (n==0 && count>0)
      access|=EndOfStream;

    pointer+=n;
    return n;
#elif defined(HAVE_GNUTLS)
    FXival n;
x:  n=gnutls_record_recv(session,data,count);
    if(__unlikely(n<0)) {
      if (n == GNUTLS_E_INTERRUPTED || n == GNUTLS_E_AGAIN) {
        WaitMode mode;

        if (gnutls_record_get_direction(session)==0)
          mode = WaitMode::Read;
        else
          mode = WaitMode::Write;

        if (wait(mode)==WaitEvent::Input)
          goto x;
        }
      else if (n == GNUTLS_E_REHANDSHAKE) {
        if (gnutls_safe_renegotiation_status(session) && handshake()==0) {
          goto x;
          }
        return FXIO::Error;
        }
      else if (!gnutls_error_is_fatal(n)){
        goto x; // try again
        }
      else {
        return FXIO::Error;
        }
      }

    if (n==0 && count>0)
      access|=EndOfStream;

    pointer+=n;
    return n;
#endif
    }
  return FXIO::Error;
  }



// Read block of bytes, returning number of bytes read
FXival SecureSocket::writeBlock(const void* data,FXival count) {
  if(__likely(device!=BadHandle) && __likely(access&WriteOnly)){
#if defined(HAVE_OPENSSL)
    FXival n;
x:  n=SSL_write(ssl,data,count);
    if (__unlikely(n<0)) {
      WaitMode mode;
      switch(SSL_get_error(ssl,n)) {
        case SSL_ERROR_WANT_READ :
          mode = WaitMode::Read;
          break;
        case SSL_ERROR_WANT_WRITE:
          mode = WaitMode::Write;
          break;
        default:
          close();
          return FXIO::Error;
        }

      if ((access&FXIO::NonBlocking) && wait(mode)==WaitEvent::Input) {
        goto x;
        }
      return FXIO::Error;
      }

    if (n==0 && count>0)
      access|=EndOfStream;

    pointer+=n;
    return n;
#elif defined(HAVE_GNUTLS)
    FXival n;
x:  n=gnutls_record_send(session,data,count);
    if(__unlikely(n<0)) {
      if (n == GNUTLS_E_INTERRUPTED || n == GNUTLS_E_AGAIN) {
        WaitMode mode;

        if (gnutls_record_get_direction(session)==0)
          mode = WaitMode::Read;
        else
          mode = WaitMode::Write;

        if (wait(mode)==WaitEvent::Input)
          goto x;
        }
      else if (n == GNUTLS_E_REHANDSHAKE) {
        if (gnutls_safe_renegotiation_status(session) && handshake()==0) {
          goto x;
          }
        return FXIO::Error;
        }
      else if (!gnutls_error_is_fatal(n)){
        goto x; // try again
        }
      else {
        return FXIO::Error;
        }
      }

    if (n==0 && count>0)
      access|=EndOfStream;

    pointer+=n;
    return n;
#endif
    }
  return FXIO::Error;
  }


ThreadSecureSocket::ThreadSecureSocket(IOContext * ctx) : context(ctx) {
  }

WaitEvent ThreadSecureSocket::wait(WaitMode mode) {
#ifdef _WIN32
  if (mode==WaitRead)
    WSAEventSelect(sockethandle,device,FD_READ);
  else if (mode==WaitWrite)
    WSAEventSelect(sockethandle,device,FD_WRITE);
  else if (mode==WaitConnect)
    WSAEventSelect(sockethandle,device,FD_CONNECT);
#endif
  WaitEvent event;
  do {
    event = context->signal().wait(device,mode,10_s);
    if (event==WaitEvent::Input) {
      return event;
      }
    else if (event==WaitEvent::Signal) {
      if (context->aborted()) return WaitEvent::Signal;
      }
    }
  while(event==WaitEvent::Signal);
  return event;
  }


#endif

}
