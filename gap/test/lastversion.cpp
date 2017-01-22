#include <fx.h>

typedef FXArray<FXString> FXStringList;
#include <FXTextCodec.h>
#include <ap.h>


FXbool gm_parse_iso8601(const FXString & str, FXTime & timestamp) {
  /// 1 second expresed in nanoseconds
  const FXlong seconds = 1000000000;
  FXint year,month,day,hour,minute,second;
  if (str.scan("%d-%d-%dT%d:%d:%dZ",&year,&month,&day,&hour,&minute,&second)==6) {
    timestamp  = FXDate(year,month,day).getTime();
    timestamp += seconds * ((hour*3600)+(minute*60)+(second));
    return true;
    }
  return false;
  }


void check_lastversion() {
  FXString version;
  FXTime date;

  HttpClient client;
  client.setAcceptEncoding(HttpClient::AcceptEncodingGZip);
  if (client.basic("GET","https://api.github.com/repos/gogglesmm/gogglesmm/releases/latest","User-Agent: gogglesmm/1.1\r\n")) {
    FXString data = client.textBody();
    FXVariant info;
    FXJSON json(data.text(),data.length());
    if (json.load(info)==FXJSON::ErrOK) {
      version = info["tag_name"];
      if (!gm_parse_iso8601(info["published_at"],date))
        return;
      fxmessage("gogglesmm version: %s\ndate: %s\n%s\n",version.text(),info["published_at"].asChars(),FXSystem::localTime(date).text());
      }
    }
  }


int main(int /*argc*/,char ** /*argv*/) {
  ap_init_crypto();
  check_lastversion();
  ap_free_crypto();
  return 1;
  }
