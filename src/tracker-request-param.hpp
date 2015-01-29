
#ifndef TRACKER_REQUEST_HPP
#define TRACKER_REQUEST_HPP

#include "meta-info.hpp"
#include "http/url-encoding.hpp"

namespace sbt {

class TrackerRequestParam {
public:
    TrackerRequestParam(const sbt::MetaInfo& meta, string peer_protocol_port, string
            tracker_url) {
        _meta = meta;
        _tracker_url = tracker_url;
        _info_hash = get_info_hash();
        _peer_id = "ABCDEFGHIJKLMNOPQRST";
        _ip = "127.0.0.1";
        _port = peer_protocol_port;
        _uploaded = "0";
        _downloaded = "0";
        _left = "0";
        _event = "started";
    }

    ~TrackerRequestParam() {}

    string get_info_hash() {
        ConstBufferPtr p = _meta.getHash();
        uint8_t* hash = (uint8_t*)p->get();
        string info_hash = sbt::url::encode(hash, 20);
        return info_hash;
    }

    string get_params() {
        _params += _tracker_url;
        _params += "?info_hash=" + _info_hash;
        _params += "&peer_id=" + _peer_id;
        _params += "&ip=" + _ip;
        _params += "&port=" + _port;
        _params += "&uploaded=" + _uploaded;
        _params += "&downloaded=" + _downloaded;
        _params += "&left=" + _left;
        if (_event != "") {
            _params += "&event=" + _event;
        }
        return _params;
    }

    void set_event(string& event) {
        _event = event;
    }


private:
    sbt::MetaInfo _meta;
    string _tracker_url;
    // The parameter for the GET method, including info_hash, peer_id, etc.
    string _info_hash;
    string _peer_id;
    string _ip;
    string _port;
    string _uploaded;
    string _downloaded;
    string _left;
    string _event;
    string _params;
};

}

#endif // TRACKER_REQUEST_HPP
