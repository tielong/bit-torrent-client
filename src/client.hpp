/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California
 *
 * This file is part of Simple BT.
 * See AUTHORS.md for complete list of Simple BT authors and contributors.
 *
 * NSL is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NSL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NSL, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef SBT_CLIENT_HPP
#define SBT_CLIENT_HPP

#include "common.hpp"
#include "meta-info.hpp"
#include "tracker-request-param.hpp"
#include "http/http-request.hpp"
#include "tracker-response.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define _4KB 4096

namespace sbt {

class Client
{
public:
  Client(const std::string& port, const std::string& torrent)
  {
      _peer_protocol_port = port;
      // This could be any port
      _http_protocol_port = "11111";
      _torrent = torrent;
      decode_torrent();
      set_tracker_url();
      _response = new char[_4KB];
  }

  ~Client() {
      delete[] _response;
      _response = NULL;
  }

  // Decode the torrent file and save the meta-info
  void decode_torrent() {
      std::ifstream bt_stream;
      bt_stream.open(_torrent, std::ifstream::in);
      if (!bt_stream.is_open()) {
          perror("Cannot open torrent file: ");
          exit(1);
      }
      meta_info.wireDecode(bt_stream);
  }

  void set_tracker_url() {
      _tracker_url = meta_info.getAnnounce();
  }

  string get_tracker_url() {
      return _tracker_url;
  }

  // parse the tracker url to get its port number
  string get_tracker_port() {
      string delim_protocol = "://";
      string delim_host = ":";
      string delim_port = "/";
      string s = _tracker_url;
      size_t pos;
      pos = s.find(delim_protocol);
      s = s.substr(pos + delim_protocol.length());
      pos = s.find(delim_host);
      s = s.substr(pos + delim_host.length());
      pos = s.find(delim_port);
      string port = s.substr(0, pos);
      return port;
  }

  // connect to server
  int connect_tracker() {
      _sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (_sockfd < 0) {
          perror("Cannot open socket");
          return 1;
      }

      // create client address
      struct sockaddr_in client_addr;
      client_addr.sin_family = AF_INET;
      client_addr.sin_port = htons(atoi(_http_protocol_port.c_str()));
      client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
      memset(client_addr.sin_zero, '\0', sizeof(client_addr.sin_zero));

      // bind client to given port
      if (bind(_sockfd, (struct sockaddr*) &client_addr, sizeof(client_addr)) == -1) {
          perror("Error binding socket");
          return 2;
      }

      // initialize server(tracker) address
      struct sockaddr_in server_addr;
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(atoi(get_tracker_port().c_str()));
      server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
      memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

      // connect to tracker
      if (connect(_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
          perror("Error connecting to tracker");
          return 3;
      }
      return 0;
  }

  int send_request(bool is_first_request) {
      sbt::TrackerRequestParam tracker_request_param(meta_info, _peer_protocol_port,
              _tracker_url);

      if (!is_first_request) {
          string event = "";
          tracker_request_param.set_event(event);
      }
      string params = tracker_request_param.get_params();

      HttpRequest req;
      req.setMethod(HttpRequest::GET);
      req.setPath(params);
      req.setVersion("1.0");
      req.addHeader("Host", "localhost:12345");

      size_t len = req.getTotalLength();
      char* message = new char[len];
      req.formatRequest(message);

      if (send(_sockfd, message, len, 0) == -1) {
          perror("Error sending message to tracker");
          return 1;
      }


      memset(_response, '\0', _4KB);
      if (recv(_sockfd, _response, _4KB, 0) == -1) {
          perror("Error receiving response from tracker");
          return 2;
      }

      //cout << _response << endl;
      delete[] message;

      return 0;
  }

  int handle_http_response(bool is_first_request) {
      // no valid response available
      if (!_response) return -1;

      std::istringstream is(_response);
      string curr_line;
      while (curr_line[0] != 'd') {
        std::getline(is, curr_line);
      }

      // curr_line now contains the bencoded response, decode it
      std::istringstream iss(curr_line);
      sbt::bencoding::Dictionary dict;
      dict.wireDecode(iss);

      TrackerResponse tracker_res;
      tracker_res.decode(dict);

      // get the next interval and peer information
      _interval = tracker_res.getInterval();
      _peers = tracker_res.getPeers();
      if (is_first_request) {
          for (auto i = _peers.begin(); i != _peers.end(); i++) {
              cout << i->ip << ":" << i->port << endl;
          }
      }

      // wait for required interval, then send the next request
      //cout << "Sleeping " << _interval << " seconds" << endl;
      sleep(_interval);
      return 0;
  }

  void close_connection() {
      close(_sockfd);
  }

private:
  string _peer_protocol_port;
  string _http_protocol_port;
  string _torrent;
  string _tracker_url;
  sbt::MetaInfo meta_info;
  int _sockfd;
  char* _response;
  uint64_t _interval;
  vector<PeerInfo> _peers;
};

} // namespace sbt

#endif // SBT_CLIENT_HPP
