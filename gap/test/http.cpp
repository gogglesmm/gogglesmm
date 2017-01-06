#include <fx.h>

typedef FXArray<FXString> FXStringList;
#include <FXTextCodec.h>
#include <ap.h>

int main(int argc,char * argv[]) {
  if (argc==2) {

    ap_init_crypto();

    HttpClient client;

    client.setAcceptEncoding(HttpClient::AcceptEncodingGZip);

    if (client.basic("GET",argv[1])) {
      FXString page = client.textBody();
      printf("%d\n",page.length());
      }

    ap_free_crypto();
    return 0;
    }

  return 1;
  }
